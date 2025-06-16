#include "parser.h"
#include "grammar/grammar.h"
#include "lr1_table.h"
#include "ast_nodes/ast_optimize.h"
#include "ast_nodes/ast_builder.h"
#include "ast_nodes/ast_nodes.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define AST_STACK_SIZE 1000
#define MAX_CHILDREN 10 

// Funciones auxiliares para casos específicos
static NodeType get_node_type_for_operator(const char* op) {
    if (strcmp(op, "+") == 0) return NODE_PLUS;
    if (strcmp(op, "-") == 0) return NODE_MINUS;
    if (strcmp(op, "*") == 0) return NODE_MULT;
    if (strcmp(op, "/") == 0) return NODE_DIV;
    if (strcmp(op, "&&") == 0) return NODE_AND;
    if (strcmp(op, "||") == 0) return NODE_OR;
    // ... otros operadores
    return NODE_BINARY;
}

static Node* handle_function_declaration(Node* inline_form) {
    // Asume que inline_form es un nodo MethodDeclarationNode
    MethodDeclarationNode* method = (MethodDeclarationNode*)inline_form;
    return (Node*)ast_make_function(
        method->name,
        (DeclarationNode**)method->params,
        method->param_counter,
        (ExpressionNode*)method->body,
        method->returnType,
        method->base.base.row,
        method->base.base.column
    );
}

static Node* handle_conditional_statement(Node** children, int count) {
    // Implementación simplificada - adaptar según necesidades
    ExpressionNode* condition = (ExpressionNode*)children[2];
    ExpressionNode* then_expr = (ExpressionNode*)children[4];
    ExpressionNode* else_expr = count > 5 ? (ExpressionNode*)children[6] : NULL;
    
    ExpressionNode* conditions[] = {condition};
    ExpressionNode* exprs[] = {then_expr};
    
    return (Node*)ast_make_conditional(
        conditions,
        exprs,
        1,
        else_expr,
        condition->base.row,
        condition->base.column
    );
}

static SemanticStack* create_semantic_stack(int capacity) {
    SemanticStack* s = malloc(sizeof(SemanticStack));
    s->items = malloc(capacity * sizeof(SemanticEntry));
    s->top = -1;
    s->capacity = capacity;
    return s;
}

static void semantic_push(SemanticStack* s, Node* node, Symbol* symbol, int is_node) {
    if (s->top == s->capacity - 1) {
        // Redimensionar si es necesario
        s->capacity *= 2;
        s->items = realloc(s->items, s->capacity * sizeof(SemanticEntry));
    }
    s->top++;
    s->items[s->top].node = node;
    s->items[s->top].symbol = symbol;
    s->items[s->top].is_node = is_node;
}

static SemanticEntry semantic_pop(SemanticStack* s) {
    if (s->top == -1) {
        return (SemanticEntry){NULL, NULL, -1};
    }
    return s->items[s->top--];
}

static void free_semantic_stack(SemanticStack* s) {
    free(s->items);
    free(s);
}
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
    
    SemanticStack* sem_stack = create_semantic_stack(32);
    
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
                ast[top++] = create_node(lookahead,lookahead->name,0,NULL);  //////////////////////////////////

                if(should_build_node_for_symbol(lookahead)) {
                    semantic_push(sem_stack, NULL, lookahead, 0);
                }
                
                stack_push(&st, a.value);
                lookahead = (++pos < toks) ? input[pos] : table->grammar->eof;
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
                    // if(should_build_node_for_symbol(p->right[i])) {
                    //     SemanticEntry entry = semantic_pop(sem_stack);
                    //     if(entry.is_node != 1) {
                    //         fprintf(stderr, "Error: Se esperaba nodo AST para %s\n", p->right[i]->name);
                    //     }
                    //     children[actual_children++] = entry.node;
                    // }
                }

                Node* node = create_node(p->left,NULL,N,children);
                ast[top++]=node;

                for(int i=0; i < N;i++) 
                    stack_pop(&st);
                
                if (a.value != 0) {  // No es producción augmentada
                    int nt = nonterm_index(table->grammar, p->left);
                    int gto = table->goto_table[stack_top(st)][nt];
                    stack_push(&st, gto);
                }

                break;

                //Node* new_node = build_ast_node(p, children);
                // if(!new_node) {
                //     fprintf(stderr, "Error en build_ast_node para producción %d\n", p->number);
                // }
                
                // // 3. Manejar pila de estados (sin cambios)
                // for(int i = 0; i < p->right_len; i++) {
                //     stack_pop(&st);
                // }
                // int new_state = table->goto_table[stack_top(st)][nonterm_index(table->grammar, p->left)];
                // stack_push(&st, new_state);

                // // 4. Pushear el nuevo nodo
                // semantic_push(sem_stack, new_node, NULL, 1);
                //break;
        
            }
            case ACTION_ACCEPT:{
                printf("ACCEPT\n");
                
                Node* children[2];
                children[1] = ast[1];
                children[0] = ast[0];
                Node* root = create_node(create_symbol("Program", NON_TERMINAL), "Program", 2, children);
                root = optimize_ast(root);

                print_ast_root(root);
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
