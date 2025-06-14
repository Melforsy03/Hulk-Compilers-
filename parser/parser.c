#include "parser.h"
#include "../grammar/grammar.h"
#include "lr1_table.h"
#include "ast_builder.h"
#include "ast_nodes.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define AST_STACK_SIZE 1000

typedef struct StackNode {
    int state;
    struct StackNode*next;
} StackNode;

static void stack_push(StackNode**s,int st){
    StackNode*n = malloc(sizeof* n);
    n->state=st;
    n->next=*s;
    *s=n;
}
static int stack_top(StackNode*s){
    return s ? s->state: -1;
}
static void stack_pop(StackNode**s){
    if(!*s)return;
    StackNode*t = *s;
    *s = t->next;
    free(t);
}  
static void clear_stack(StackNode* s){
    while(s){StackNode*t=s->next;free(s);s=t;}
}

// Índices
static int symbol_index(Grammar* g,Symbol* s){
    for(int i = 0; i < g->terminals_count; i++) 
    {
        if(strcmp(g->terminals[i]->name, s->name) == 0) return i;
    }
        
    return -1;
}
static int nonterm_index(Grammar*g,Symbol*s){
    for(int i=0;i<g->nonterminals_count;i++) if(g->nonterminals[i]==s)return i;
    return -1;
}
// obtener producción
static Production* get_production_by_number(Grammar*g,int num){
    return (num >= 0 && num < g->production_count)? g->productions[num] : NULL;
}

Node* parser(LR1Table* table, Symbol** input, int toks, ActionEntryLR1**  acts,int* actc){

    if(!table||!input||toks<1||!acts||!actc) 
    {
        fprintf(stderr, "Error: Parámetros inválidos para parse\n");
        return NULL;
    }

    StackNode* st=NULL;
    stack_push(&st,0);

    Node* ast[AST_STACK_SIZE];
    int top = 0;

    int pos = 0; 
    Symbol* look = input[pos];
    *actc = 0; 
    *acts = malloc(table->state_count*sizeof(**acts));
    if (!*acts) return NULL;

    while(1){
        int s = stack_top(st);
        if (s < 0 || s >= table->state_count) {
            printf("es aqui 1 %d, este es el s %d este era el table state count", s, table->state_count);
            fprintf(stderr, "Error: Estado %d fuera de rango\n", s);
            clear_stack(st);
            free(*acts);
            return NULL;
        }

        int tidx = symbol_index(table->grammar,look);
        if (tidx == -1) {
            fprintf(stderr, "Error: Símbolo '%s' no encontrado en terminales\n", look->name);
            clear_stack(st);
            free(*acts);
            return NULL;
        }

        // Verificar límites de la tabla ACTION
        if (tidx >= table->terminal_count) {
            printf("es aqui 2");
            fprintf(stderr, "Error: Índice de símbolo %d fuera de rango\n", tidx);
            clear_stack(st);
            free(*acts);
            return NULL;
        }

        ActionEntryLR1 a = table->action[s][tidx]; 
        (*acts)[(*actc)++]=a;

        printf("Estado %d, %d, Símbolo '%s':\n ", s, tidx, look->name);

        switch(a.action){
            case ACTION_SHIFT:{
                Node* leaf;
                if      (strcmp(look->name, "NUMBER") == 0)
                    leaf = (Node*)ast_make_literal(NODE_NUMBER, look->name, 0, 0);
                else if (strcmp(look->name, "STRING") == 0)
                    leaf = (Node*)ast_make_literal(NODE_STRING, look->name, 0, 0);
                else if (strcmp(look->name, "TRUE") == 0 || strcmp(look->name, "FALSE") == 0)
                    leaf = (Node*)ast_make_literal(NODE_BOOLEAN, look->name, 0, 0);
                else if (strcmp(look->name, "IDENTIFIER") == 0)
                    leaf = (Node*)ast_make_literal(NODE_VAR, look->name, 0, 0);
                else
                    leaf = create_node(look, look->name, 0, NULL);

                ast[top++] = leaf;
                stack_push(&st, a.value);
                look = (++pos < toks) ? input[pos] : table->grammar->eof;
                break;
            }
    
            case ACTION_REDUCE:{
                Production* p = get_production_by_number(table->grammar, a.value);
                printf("REDUCE: %s ->", p->left->name);
                for (int i = 0; i < p->right_len; i++) {
                    printf(" %s", p->right[i]->name);
                }
                printf("\n");
                int N = p->right_len;
                Node* children[N];
                for (int i = N-1; i >= 0; --i)
                    children[i] = ast[--top];

                Node* node = NULL;
               
                // —— Program -> Type_function_list Expr_item_list EOF ——
                if (strcmp(p->left->name, "Program") == 0) {
                    node = (Node*)ast_make_program((DeclarationNode**)children[0], children[0]->child_count,(ExpressionNode*)children[1],
                        0, 0
                    );
                    printf("Created node: %s, type %d, children %d\n", 
                    p->left->name, node ? node->tipo : -1, N);
                }

                // —— Let_expr -> LET Assignment IN Expr ——
                else if (strcmp(p->left->name, "Let_expr") == 0) {
                    node = (Node*)ast_make_let_in(
                        (VarDeclarationNode**)children[1],
                        children[1]->child_count,
                        (ExpressionNode*)children[3],
                        children[0]->row, children[0]->column
                    );
                    printf("Created node: %s, type %d, children %d\n", 
                    p->left->name, node ? node->tipo : -1, N);
                }

                // —— Conditional -> IF LPAREN Expr RPAREN THEN Expr Cond_other_case ——
                else if (strcmp(p->left->name, "Conditional") == 0) {
                    ExpressionNode* conditions[] = { (ExpressionNode*)children[2] };
                    ExpressionNode* expressions[] = { (ExpressionNode*)children[5] };

                    node = (Node*)ast_make_conditional(
                        conditions,
                        expressions,
                        1,
                        (ExpressionNode*)children[6], // else case
                        children[0]->row, children[0]->column
                    );
                    printf("Created node: %s, type %d, children %d\n", 
                    p->left->name, node ? node->tipo : -1, N);
                }

                // —— Func -> function Full_form ——
                else if (strcmp(p->left->name, "Func") == 0 && N == 2) {
                    MethodDeclarationNode* method = (MethodDeclarationNode*)children[1];
                    node = (Node*)ast_make_function(
                        method->name,
                        (DeclarationNode**)method->params,
                        method->param_counter,
                        (ExpressionNode*)method->body,
                        method->returnType,
                        children[0]->row, children[0]->column
                    );
                    printf("Created node: %s, type %d, children %d\n", 
                    p->left->name, node ? node->tipo : -1, N);
                }

                // —— Type -> type Type_dec Type_block ——
                else if (strcmp(p->left->name, "Type") == 0 && N == 3) {
                    TypeDeclarationNode* type_node = malloc(sizeof(TypeDeclarationNode));
                    type_node->base.base.row = children[0]->row;
                    type_node->base.base.column = children[0]->column;
                    type_node->base.base.tipo = NODE_TYPE_DECLARATION;
                    type_node->name = strdup(children[1]->lexeme);
                    type_node->parent = strdup("Object");
                    type_node->attributes = children[2]; // Type_member_list
                    node = (Node*)type_node;
                }

                // —— Call_func -> IDENTIFIER LPAREN Arguments RPAREN ——
                else if (strcmp(p->left->name, "Call_func") == 0 && N == 4) {
                    CallFuncNode* call = malloc(sizeof(CallFuncNode));
                    call->base.base.base.row = children[0]->row;
                    call->base.base.base.column = children[0]->column;
                    call->base.base.base.tipo = NODE_CALL_FUNC;
                    call->name = strdup(children[0]->lexeme);
                    call->arguments = children[2]; // Arguments node
                    node = (Node*)call;
                    printf("Created node: %s, type %d, children %d\n", 
                    p->left->name, node ? node->tipo : -1, N);
                }
    
                // —— Atom → '(' Expr ')' | TRUE | FALSE | IDENTIFIER | NUMBER | STRING | ID —— 
                else if (strcmp(p->left->name, "Atom")==0) {
                    if (N==3) {
                        node = children[1];
                    } else {
                        // el shift ya apiló el literal/var correspondiente
                        node = children[0];
                    }
                    printf("Created node: %s, type %d, children %d\n", 
                    p->left->name, node ? node->tipo : -1, N);
                }

                // —— Factor → NOT Atom | Atom —— 
                else if (strcmp(p->left->name, "Factor")==0) {
                    if (N==2) {
                        node = (Node*)ast_make_unary(
                            NODE_NOT,
                            (ExpressionNode*)children[1],
                            children[1]->row, children[1]->column
                        );
                    } 
                    else {
                        node = children[0];
                    }
                    printf("Created node: %s, type %d, children %d\n", 
                    p->left->name, node ? node->tipo : -1, N);
                }

                // —— Sign → '+' Factor | '-' Factor | Factor —— 
                else if (strcmp(p->left->name, "Sign")==0 && N==2) {
                    NodeType t = (strcmp(children[0]->lexeme, "+")==0)
                                 ? NODE_POSITIVE : NODE_NEGATIVE;
                    node = (Node*)ast_make_unary(
                        t,
                        (ExpressionNode*)children[1],
                        children[1]->row, children[1]->column
                    );
                    printf("Created node: %s, type %d, children %d\n", 
                    p->left->name, node ? node->tipo : -1, N);
                }
                else if (strcmp(p->left->name, "Sign")==0) {
                    node = children[0];
                    printf("Created node: %s, type %d, children %d\n", 
                    p->left->name, node ? node->tipo : -1, N);
                }

                // —— Pow → Sign ('POWER'|'DSTAR') Pow | Sign —— 
                else if (strcmp(p->left->name, "Pow")==0 && N==3) {
                    char* op = children[1]->lexeme;
                    node = (Node*)ast_make_binary(
                        (strcmp(op,"POWER")==0 ? NODE_POW : NODE_POW),
                        (ExpressionNode*)children[0],
                        (ExpressionNode*)children[2],
                        op,
                        children[0]->row, children[0]->column
                    );
                    printf("Created node: %s, type %d, children %d\n", 
                    p->left->name, node ? node->tipo : -1, N);
                }
                else if (strcmp(p->left->name, "Pow")==0) {
                    node = children[0];
                    printf("Created node: %s, type %d, children %d\n", 
                    p->left->name, node ? node->tipo : -1, N);
                }
                // —— Term → Term ('STAR'|'SLASH'|'MODULO') Pow | Pow —— 
                else if (strcmp(p->left->name, "Term")==0 && N==3) {
                    char* op = children[1]->lexeme;
                    NodeType t = (strcmp(op,"STAR")==0  ? NODE_MULT :
                                  strcmp(op,"SLASH")==0 ? NODE_DIV  :
                                                          NODE_MOD);
                    node = (Node*)ast_make_binary(
                        t,
                        (ExpressionNode*)children[0],
                        (ExpressionNode*)children[2],
                        op,
                        children[0]->row, children[0]->column
                    );
                    printf("Created node: %s, type %d, children %d\n", 
                    p->left->name, node ? node->tipo : -1, N);
                }
                else if (strcmp(p->left->name, "Term")==0) {
                    node = children[0];
                    printf("Created node: %s, type %d, children %d\n", 
                    p->left->name, node ? node->tipo : -1, N);
                }

                // —— Arithmetic → Arithmetic ('PLUS'|'MINUS') Term | Term —— 
                else if (strcmp(p->left->name, "Arithmetic")==0 && N==3) {
                    char* op = children[1]->lexeme;
                    NodeType t = (strcmp(op,"PLUS")==0 ? NODE_PLUS : NODE_MINUS);
                    node = (Node*)ast_make_binary(
                        t,
                        (ExpressionNode*)children[0],
                        (ExpressionNode*)children[2],
                        op,
                        children[0]->row, children[0]->column
                    );
                    printf("Created node: %s, type %d, children %d\n", 
                    p->left->name, node ? node->tipo : -1, N);
                }
                else if (strcmp(p->left->name, "Arithmetic")==0) {
                    node = children[0];
                    printf("Created node: %s, type %d, children %d\n", 
                    p->left->name, node ? node->tipo : -1, N);
                }

                // —— Concat → Concat ('AT'|'AT_AT') Arithmetic | Arithmetic —— 
                else if (strcmp(p->left->name, "Concat")==0 && N==3) {
                    char* op = children[1]->lexeme;
                    NodeType t = (strcmp(op,"AT_AT")==0 ? NODE_DOUBLE_CONCAT : NODE_CONCAT);
                    node = (Node*)ast_make_binary(
                        t,
                        (ExpressionNode*)children[0],
                        (ExpressionNode*)children[2],
                        op,
                        children[0]->row, children[0]->column
                    );
                    printf("Created node: %s, type %d, children %d\n", 
                    p->left->name, node ? node->tipo : -1, N);
                }
                else if (strcmp(p->left->name, "Concat")==0) {
                    node = children[0];
                }

                // —— Aritm_comp → Aritm_comp (EQUAL_EQUAL|NOT_EQUAL|GREATER|GREATER_EQUAL|LESS|LESS_EQUAL) Concat | Concat —— 
                else if (strcmp(p->left->name, "Aritm_comp")==0 && N==3) {
                    char* op = children[1]->lexeme;
                    NodeType t = (strcmp(op,"EQUAL_EQUAL")==0     ? NODE_EQUAL :
                                  strcmp(op,"NOT_EQUAL")==0       ? NODE_NOT_EQUAL :
                                  strcmp(op,"GREATER")==0         ? NODE_GREATER :
                                  strcmp(op,"GREATER_EQUAL")==0   ? NODE_GREATER_EQUAL :
                                  strcmp(op,"LESS")==0            ? NODE_LESS :
                                                                   NODE_LESS_EQUAL);
                    node = (Node*)ast_make_binary(
                        t,
                        (ExpressionNode*)children[0],
                        (ExpressionNode*)children[2],
                        op,
                        children[0]->row, children[0]->column
                    );
                    printf("Created node: %s, type %d, children %d\n", 
                    p->left->name, node ? node->tipo : -1, N);
                }
                else if (strcmp(p->left->name, "Aritm_comp")==0) {
                    node = children[0];
                    printf("Created node: %s, type %d, children %d\n", 
                    p->left->name, node ? node->tipo : -1, N);
                }

                // —— Check_type → Check_type 'IS' IDENTIFIER | Aritm_comp —— 
                else if (strcmp(p->left->name, "Check_type")==0 && N==3) {
                    node = (Node*)ast_make_binary(
                        NODE_CHECK_TYPE,
                        (ExpressionNode*)children[0],
                        (ExpressionNode*)children[2],
                        "IS",
                        children[0]->row, children[0]->column
                    );
                }
                else if (strcmp(p->left->name, "Check_type")==0) {
                    node = children[0];
                }
                // —— And_expr → And_expr 'AND' Check_type | Check_type —— 
                else if (strcmp(p->left->name, "And_expr")==0 && N==3) {
                    node = (Node*)ast_make_binary(
                        NODE_AND,
                        (ExpressionNode*)children[0],
                        (ExpressionNode*)children[2],
                        "AND",
                        children[0]->row, children[0]->column
                    );
                }
                else if (strcmp(p->left->name, "And_expr")==0) {
                    node = children[0];
                    printf("Created node: %s, type %d, children %d\n", 
                    p->left->name, node ? node->tipo : -1, N);
                }

                // —— Or_expr → Or_expr 'OR' And_expr | And_expr —— 
                else if (strcmp(p->left->name, "Or_expr")==0 && N==3) {
                    node = (Node*)ast_make_binary(
                        NODE_OR,
                        (ExpressionNode*)children[0],
                        (ExpressionNode*)children[2],
                        "OR",
                        children[0]->row, children[0]->column
                    );
                }
                else if (strcmp(p->left->name, "Or_expr")==0) {
                    node = children[0];
                    printf("Created node: %s, type %d, children %d\n", 
                    p->left->name, node ? node->tipo : -1, N);
                }

                // —— Fallback genérico —— 
                if (!node) {
                    node = create_node(p->left, NULL, N, children);
                }

                ast[top++] = node;

                // actualizar pila de estados
                for (int i = 0; i < N; ++i) stack_pop(&st);
                int nt  = nonterm_index(table->grammar, p->left);
                int gto = table->goto_table[stack_top(st)][nt];
                stack_push(&st, gto);
                break;
        
            }

            case ACTION_ACCEPT: {
                printf("ACCEPT - Building final Program Node\n");

                // Verificar los nodos antes de construir el ProgramNode
                printf("Debug: Type_function_list type = %d, addr = %p\n", 
                       ast[0] ? ast[0]->tipo : -1, ast[0]);
                printf("Debug: Expr_item_list type = %d, addr = %p\n", 
                       ast[1] ? ast[1]->tipo : -1, ast[1]);
                
                // Crear nodo Program correctamente estructurado
                ProgramNode* program_node = malloc(sizeof(ProgramNode));
                if (!program_node) {
                    fprintf(stderr, "Error: Failed to allocate ProgramNode\n");
                    clear_stack(st);
                    return NULL;
                }

                // Inicializar estructura base
                program_node->base.row = 0;
                program_node->base.column = 0;
                program_node->base.tipo = NODE_PROGRAM;
                program_node->base.symbol = create_symbol("Program", NON_TERMINAL);
                program_node->base.lexeme = NULL;
                program_node->base.child_count = 0;
                program_node->base.children = NULL;

                // Asignar declaraciones (convertir a lista terminada en NULL)
                DeclarationNode** decls = malloc(2 * sizeof(DeclarationNode*));
                if (!decls) {
                    free(program_node);
                    clear_stack(st);
                    return NULL;
                }
                decls[0] = (DeclarationNode*)ast[0];
                decls[1] = NULL;
                program_node->declarations = decls;

                // Asignar expresión principal
                program_node->expression = (ExpressionNode*)ast[1];

                // Verificar estructura antes de imprimir
                printf("Debug: ProgramNode structure:\n");
                printf("  Declarations[0] = %p\n", decls[0]);
                printf("  Expression = %p\n", program_node->expression);

                print_ast_root((Node*)program_node);
                clear_stack(st);
                return (Node*)program_node;
            }
            case ACTION_ERROR:
            default:
                fprintf(stderr,"Error sintáctico en estado %d\n",s);
                fprintf(stderr, "Posibles acciones en este estado:\n");
                for (int k = 0; k < table->terminal_count; k++) {
                    if (table->action[s][k].action != ACTION_ERROR) {
                        fprintf(stderr, "  %s: %s %d\n", 
                            table->grammar->terminals[k]->name,
                            table->action[s][k].action == ACTION_SHIFT ? "SHIFT" : "REDUCE",
                            table->action[s][k].value);
                    }
                }
                clear_stack(st);
                free(*acts);
                return NULL;
        }
    }
}