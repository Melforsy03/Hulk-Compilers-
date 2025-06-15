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


Node* parser(LR1Table* table, Symbol** input, int toks, ActionEntryLR1**  acts, int* actc){

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
                printf("%s",p->right_len);
            
                printf("REDUCE: %s ->", p->left->name);
                for (int i = 0; i < p->right_len; i++) {
                    printf(" %s", p->right[i]->name);
                }
                printf("\n");
                int N = p->right_len;
                Node* children[N];
                for (int i = N-1; i >= 0; --i)
                    children[i] = ast[--top];

                Node* node = build_ast_node(p, children);

                if (!node) {
                    fprintf(stderr, "Error: Failed to build AST node for production %s\n", p->left->name);
                    clear_stack(st);
                    free(*acts);
                    return NULL;
                }

                printf("Created node: %s, type %d, children %d\n", 
                p->left->name, node ? node->tipo : -1, N);
                ast[top++] = node;

                // actualizar pila de estados
                for (int i = 0; i < N; ++i) stack_pop(&st);
                int nt  = nonterm_index(table->grammar, p->left);
                int gto = table->goto_table[stack_top(st)][nt];
                stack_push(&st, gto);
                printf("GOTO state %d\n", gto);
                break;
            }
            case ACTION_ACCEPT: {
                printf("ACCEPT - Building final Program Node\n");

                // Verificar los nodos antes de construir el ProgramNode
                printf("Debug: Type_function_list type = %d, addr = %p\n", 
                       ast[0] ? ast[0]->tipo : -1, ast[0]);
                printf("Debug: Expr_item_list type = %d, addr = %p\n", 
                       ast[1] ? ast[1]->tipo : -1, ast[1]);
                
                ProgramNode* program_node = ast_make_program(
                    (DeclarationNode**)ast[0], 
                    ast[0] ? ast[0]->child_count : 0,
                    (ExpressionNode*)ast[1],
                    0, 0
                );

                if (!program_node) {
                    fprintf(stderr, "Error: Failed to allocate ProgramNode\n");
                    clear_stack(st);
                    return NULL;
                }

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