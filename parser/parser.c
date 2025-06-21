#include "parser.h"
#include "grammar/grammar.h"
#include "lr1_table.h"
#include "ast_nodes/ast_optimize.h"
#include "ast_nodes/ast_builder.h"
#include "ast_nodes/ast_build.h"
#include "ast_nodes/ast_nodes.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define AST_STACK_SIZE 1000
#define MAX_CHILDREN 10 

static int should_build_node_for_symbol(Symbol* sym) {
    // Símbolos terminales que no generan nodos directos
    const char* non_node_symbols[] = {
        "SEMICOLON", "LPAREN", "RPAREN", "LBRACE", "RBRACE", 
        "COMMA", "COLON", "ASSIGN", "ARROW", "EOF"
    };
    
    for (size_t i = 0; i < sizeof(non_node_symbols)/sizeof(non_node_symbols[0]); i++) {
        if (strcmp(sym->name, non_node_symbols[i]) == 0) {
            return 0;
        }
    }
    return 1;
}


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

static Production* get_production_by_number(Grammar*g,int num){
    return (num >= 0 && num < g->production_count)? g->productions[num] : NULL;
}

Node* parser(LR1Table* table, Symbol** input, int toks, ActionEntryLR1**  acts,int* actc){

    if(!table||!input||toks<1||!acts||!actc) 
    {
        fprintf(stderr, "Error: Parámetros inválidos para parse\n");
        return NULL;
    }

    StackNode* st = NULL;
    stack_push(&st, 0);
    
    TypedStack* typed_stack = create_typed_stack(32);
    print_tokens(input, toks);

    int pos = 0;
    Symbol* lookahead = input[pos];
    Node* result = NULL;

    Node* ast[AST_STACK_SIZE];
    int top = 0;

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
        printf("\nToken actual: [%d] %s\n", pos, lookahead->name);
        int tidx = symbol_index(table->grammar,lookahead);
        if (tidx == -1) {
            fprintf(stderr, "Error: Símbolo '%s' no encontrado en terminales\n", lookahead->name);
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

        printf("Estado %d, %d, Símbolo '%s': ", s, tidx, lookahead->name);

        switch(a.action){
            case ACTION_SHIFT:{
                printf("SHIFT a %d\n", a.value);
                ast[top++] = create_node(lookahead,lookahead->name,0,NULL); \
                stack_push(&st, a.value);
                lookahead = (++pos < toks) ? input[pos] : table->grammar->eof;
              
                if(should_build_node_for_symbol(lookahead)) {
                    // Crear nodo hoja genérico
                    Node* leaf = create_node(lookahead, lookahead->name, 0, NULL);
                    //semantic_push(sem_stack, leaf, NULL, 1);
                    
                    // Crear nodo tipado según el tipo de token
                    TypedNode typed_leaf;
                    NodeType leaf_type;
                    
                    if (strcmp(lookahead->name, "NUMBER") == 0) {
                        leaf_type = NODE_NUMBER;
                        typed_leaf.literal = ast_make_literal(leaf_type, lookahead->name, lookahead->row, lookahead->colum);
                    } 
                    else if (strcmp(lookahead->name, "IDENTIFIER") == 0) {
                        leaf_type = NODE_VAR;
                        typed_leaf.literal = ast_make_literal(leaf_type, lookahead->name, lookahead->row, lookahead->colum);
                    }
                    else if (strcmp(lookahead->name, "TRUE") == 0 || strcmp(lookahead->name, "FALSE") == 0) {
                        leaf_type = NODE_BOOLEAN;
                        typed_leaf.literal = ast_make_literal(leaf_type, lookahead->name, lookahead->row, lookahead->colum);
                    }
                    else if (strcmp(lookahead->name, "STRING") == 0) {
                        leaf_type = NODE_STRING;
                        typed_leaf.literal = ast_make_literal(leaf_type, lookahead->name, lookahead->row, lookahead->colum);
                    }
                    else {
                        // Para operadores y otros, usamos el nodo genérico
                        leaf_type = NODE_ATOMIC;
                        typed_leaf.any = leaf;
                    }
                    
                    typed_push(typed_stack, typed_leaf, leaf_type);
                }

                break;
            }
            case ACTION_REDUCE:{
                printf("REDUCE por producción %d\n", a.value);
                Production*p = get_production_by_number(table->grammar,a.value);
                int N = p->right_len;
                Node* children[N];
                int actual_children = 0;

                for(int i = N-1; i >= 0; i--) 
                {
                    children[i]=ast[--top];
                }

                Node* node = create_node(p->left,NULL,N,children);
                ast[top++]=node;

                for(int i=0; i < N;i++) 
                    stack_pop(&st);
                
                if (a.value != 0) {  // No es producción augmentada, goto
                    int nt = nonterm_index(table->grammar, p->left);
                    int gto = table->goto_table[stack_top(st)][nt];
                    stack_push(&st, gto);
                }
                
                
                // Procesar pila tipada
                TypedNode typed_children[MAX_CHILDREN];
                NodeType child_types[MAX_CHILDREN];
                int typed_child_count = 0;
                for(int i = p->right_len - 1; i >= 0; i--) {
                    if(should_build_node_for_symbol(p->right[i])) {
                        typed_children[typed_child_count] = typed_pop(typed_stack);
                        child_types[typed_child_count] = typed_peek_type(typed_stack);
                        typed_child_count++;
                    }
                }
                
                // Construir nodo según la producción
                Node* new_node = NULL;
                TypedNode typed_node;
                
                if (strcmp(p->left->name, "Func") == 0) {
                    printf("\n====================Entro al Function===================\n");
                    // new_node = ast_make_function_decl((IdentifierNode*)children[1], (ParamListNode*)children[3], (ExpressionNode*)children[6]);
                    // if (new_node) {
                    //     typed_node.arith_unary = (ArithmeticUnaryNode*)new_node;
                    // }
                } 
                else if (strcmp(p->left->name, "Conditional") == 0) {
                    printf("\n====================Entro al Conditional===================\n");
                    new_node = ast_make_conditional((ExpressionNode*)children[2], (ExpressionNode*)children[4], 0, (ExpressionNode*)children[6], lookahead->row, lookahead->colum);
                    if (new_node) {
                        typed_node.conditional = (ConditionalNode*)new_node;
                    }
                } 
                else if (strcmp(p->left->name, "While_loop") == 0) {
                    printf("\n====================Entro al While_loop===================\n");
                    new_node = ast_make_while((ExpressionNode*)children[2], (ExpressionNode*)children[4], 0 , 0);
                    if (new_node) {
                        typed_node.while_ = (WhileNode*)new_node;
                    }
                } 
                else if (strcmp(p->left->name, "For_loop") == 0) {
                    printf("\n====================Entro al For_loop===================\n");
                    new_node = ast_make_for(children[2]->lexeme, (ExpressionNode*)children[4], (ExpressionNode*)children[6], lookahead->row, lookahead->colum);
                    if (new_node) {
                        typed_node.for_ = (ForNode*)new_node;
                    }
                } 
                else if (strcmp(p->left->name, "Let_expr") == 0) {
                    printf("\n====================Entro al Let_expr===================\n");
                    new_node = ast_make_let_in((ExpressionNode*)children[1], (ExpressionNode*)children[1]->child_count, (ExpressionNode*)children[3], lookahead->row, lookahead->colum);
                    if (new_node) {
                        typed_node.let = (LetInNode*)new_node;
                    }
                }
                else if (strcmp(p->left->name, "Or_expr") == 0 && p->right_len == 3) {
                    printf("\n====================Entro Or===================\n");
                    new_node = build_binary_operation(p, children, p->right[1]->name);
                    if (new_node) {
                        typed_node.bool_binary = (BooleanBinaryNode*)new_node;
                    }
                } 
                else if (strcmp(p->left->name, "And_expr") == 0 && p->right_len == 3) {
                    printf("\n====================Entro And===================\n");
                    new_node = build_binary_operation(p, children, p->right[1]->name);
                    if (new_node) {
                        typed_node.bool_binary = (BooleanBinaryNode*)new_node;
                    }
                }  
                else if (strcmp(p->left->name, "Aritm_comp") == 0 && p->right_len == 3) {
                    printf("\n====================Entro Aritm_c======================\n");
                    new_node = build_binary_operation(p, children, p->right[1]->name);
                    if (new_node) {
                        typed_node.comp_binary = (ComparisonBinaryNode*)new_node;
                    }
                } 
                else if (strcmp(p->left->name, "Concat") == 0 && p->right_len == 3) {
                    printf("\n====================Entro Concat===================\n");
                    new_node = build_binary_operation(p, children, p->right[0]->name);
                    if (new_node) {
                        typed_node.string = (StringBinaryNode*)new_node;
                    } 
                }  
                else if (strcmp(p->left->name, "Arithmetic") == 0 && p->right_len == 3) {
                    printf("\n====================Entro Aritmet======================\n");
                    new_node = build_binary_operation(p, children, p->right[1]->name);
                    if (new_node) {
                        typed_node.arith_binary = (ArithmeticBinaryNode*)new_node;
                    }
                } 
                else if (strcmp(p->left->name, "Term") == 0 && p->right_len == 3) {
                    printf("\n====================Entro Termino===================\n");
                    new_node = build_binary_operation(p, children, p->right[1]->name);
                    if (new_node) {
                        typed_node.arith_binary = (ArithmeticBinaryNode*)new_node;
                    }
                } 
                else if (strcmp(p->left->name, "Pow") == 0 && p->right_len == 3) {
                    printf("\n====================Entro Pow===================\n");
                    new_node = build_binary_operation(p, children, p->right[1]->name);
                    if (new_node) {
                        typed_node.arith_binary = (ArithmeticBinaryNode*)new_node;
                    }
                } 
                else if (strcmp(p->left->name, "Sign") == 0 && p->right_len == 2) {
                    printf("\n====================Entro Sign===================\n");
                    new_node = build_unary_operation(p, children, p->right[0]->name);
                    if (new_node) {
                        typed_node.arith_unary = (ArithmeticUnaryNode*)new_node;
                    }
                }
                else if (strcmp(p->left->name, "Expr_block") == 0) {
                    printf("\n====================Entro al Expr_block===================\n");
                    new_node = ast_make_expression_block(children[1], lookahead->row, lookahead->colum, 0);
                    if (new_node) {
                        typed_node.exprB = (ExpressionBlockNode*)new_node;
                    }
                } 
                else if (strcmp(p->left->name, "Assignment") == 0 ){
                    printf("\n====================Entro al Assignment===================\n");
                    new_node = ast_make_var_decl(children[0]->lexeme, (ExpressionNode*)children[2], children[2]->tipo, lookahead->row, lookahead->colum);
                    if (new_node) {
                        typed_node.varD = (VarDeclarationNode*)new_node;
                    } 
                }  
                else if (strcmp(p->left->name, "Call_func") == 0){
                    printf("\n====================Entro al Call_func===================\n");
                    new_node = ast_make_call_func(children[0]->lexeme, (ExpressionNode*)children[2], children[2]->child_count, lookahead->row, lookahead->colum);
                    if (new_node) {
                        typed_node.call = (CallFuncNode*)new_node;
                    }
                }
                else if (strcmp(p->left->name, "Print") == 0){
                    printf("\n====================Entro al Print===================\n");
                    new_node = ast_make_call_func(children[0]->lexeme, (ExpressionNode*)children[2], children[2]->child_count, lookahead->row, lookahead->colum);
                    if (new_node) {
                        typed_node.call = (CallFuncNode*)new_node;
                    }
                }
                // else{

                //     printf("\n====================Entro al %s ===================\n", p->left->name);
                // }

                //else if (strcmp(p->left->name, "Method_decl") == 0) {
                //     new_node = ast_make_method_decl((IdentifierNode*)children[1], (ParamListNode*)children[3], (ExpressionNode*)children[6]);
                // } else if (strcmp(p->left->name, "Type_decl") == 0) {
                //     new_node = ast_make_type_decl((IdentifierNode*)children[1], (ParamListNode*)children[3], (TypeAttributeList*)children[7], (MethodDeclList*)children[9]);
                // } else if (strcmp(p->left->name, "Protocol_decl") == 0) {
                //     new_node = ast_make_protocol_decl((IdentifierNode*)children[1], (MethodSigList*)children[3]);
                // }


                // llamadas
                
                // else if (strcmp(p->left->name, "Call_method") == 0) {
                //     new_node = ast_make_call_method((ExpressionNode*)children[0], p->right[2]->name, (ExprList*)children[3]);
                // } 
                // else if (strcmp(p->left->name, "Call_attr") == 0) {
                //     new_node = ast_make_call_attr((ExpressionNode*)children[0], p->right[2]->name);
                // } 
                // else if (strcmp(p->left->name, "Type_instantiation") == 0) {
                //     new_node = ast_make_type_inst((IdentifierNode*)children[1], (ExprList*)children[3]);
                // }

                // Destructuring y return
                // else if (strcmp(p->left->name, "Destructuring") == 0) {
                //     new_node = ast_make_destruct((IdentifierNode*)children[0], (ExpressionNode*)children[2]);
                // } else if (strcmp(p->left->name, "Return") == 0) {
                //     new_node = ast_make_return((ExpressionNode*)children[1]);
                // }

                //  // Cast y check type
                // else if (strcmp(p->left->name, "Cast_type")==0) {
                //     new_node = ast_make_cast((ExpressionNode*)children[0], p->right[2]->name);
                // } else if (strcmp(p->left->name, "Check_type")==0) {
                //     new_node = ast_make_check_type(NODE_CAST_TYPE, (ExpressionNode*)children[0], (ExpressionNode*)children[2]);
                // }
            
                // Genérico
                // else {
                //     new_node = ast_make_generic(p->left->name, (Node**)children, N);
                // }

                typed_push(typed_stack, typed_node, new_node ? new_node->tipo : NODE_ATOMIC);
                //print_typed_stack(typed_stack);
                break;
            }
            case ACTION_ACCEPT:{
                printf("ACCEPT\n");

                //TypedNode typed_root = typed_pop(typed_stack);
                //print_typed_stack(typed_stack);
                
                Node* children[2];
                children[1] = ast[1];
                children[0] = ast[0];
                Node* root = create_node(create_symbol("Program", NON_TERMINAL), "Program", 2, children);
                //root = optimize_ast(root);

                //print_ast_root(root);
                clear_stack(st);
                return root;
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


void print_typed_stack(TypedStack* s) {
    printf("=== Typed Stack (%d items) ===\n", s->top + 1);
    for (int i = 0; i <= s->top; i++) {
        printf("[%d] Type: %d - ", i, s->types[i]);
        
        Node* node = (Node*)s->items[i].any;
        if (!node) {
            printf("NULL\n");
            continue;
        }
        
        switch(s->types[i]) {
            case NODE_NUMBER:
                printf("Number: %s\n", node->lexeme);
                break;
            case NODE_VAR:
                printf("Variable: %s\n", node->lexeme);
                break;
            case NODE_PLUS:
                printf("ArithmeticOp: %s\n", ((ArithmeticBinaryNode*)node)->base.operator);
                break;
            case NODE_MINUS:
            printf("ArithmeticOp: %s\n", ((ArithmeticBinaryNode*)node)->base.operator);
                break;
            case NODE_MULT:
            printf("ArithmeticOp: %s\n", ((ArithmeticBinaryNode*)node)->base.operator);
                break;
            case NODE_DIV:
                printf("ArithmeticOp: %s\n", ((ArithmeticBinaryNode*)node)->base.operator);
                break;
            case NODE_OR:
            case NODE_AND:
                printf("BooleanOp: %s\n", ((BooleanBinaryNode*)node)->base.operator);
                break;
            default:
                printf("Unhandled type (name: %s)\n", node->symbol ? node->symbol->name : "no symbol");
        }
    }
    printf("=====================\n");
}


void print_tokens(Symbol** tokens, int count) {
    printf("=== Tokens de entrada ===\n");
    for (int i = 0; i < count; i++) {
        if (tokens[i]) {
            printf("[%d] %s (%s)\n", i, tokens[i]->name, 
                   tokens[i]->type == TERMINAL ? "TERMINAL" : "NON_TERMINAL");
        } else {
            printf("[%d] NULL\n", i);
        }
    }
    printf("=======================\n");
}