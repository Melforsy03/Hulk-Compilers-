#include "parser.h"
#include "../grammar/grammar.h"
#include "lr1_table.h"
#include "../ast_nodes/ast_optimize.h"
#include "../ast_nodes/ast_builder.h"
#include "../ast_nodes/ast_build.h"
#include "../ast_nodes/ast_nodes.h"
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
   // print_tokens(input, toks);

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

                break;
            }
            case ACTION_REDUCE:{
                printf("REDUCE por producción %d\n", a.value);
                Production*p = get_production_by_number(table->grammar,a.value);
                int N = p->right_len;
                
                if(strcmp(p->left->name, "Atom") == 0 && p->right_len == 3){
                    int N = 1;
                }
                Node* children[N];
                for(int i = N-1;i>=0;i--) 
                    children[i]=ast[--top];

                Node* node = create_node(p->left,NULL,N,children);
                ast[top++]=node;

                for(int i=0; i < N;i++) 
                    stack_pop(&st);

                int nt = nonterm_index(table->grammar,p->left);
                int gto = table->goto_table[stack_top(st)][nt];
                stack_push(&st,gto);
                break;
            }
            case ACTION_ACCEPT:{
                
                Node* children[2];
                children[1] = ast[1];
                children[0] = ast[0];

                printf("ACCEPT\n");
                Node* root = create_node(create_symbol("Program", NON_TERMINAL), "Program", 2, children);
                //root =  create_node(create_symbol("Expr", NON_TERMINAL), "Expr", 2, node[0]);
                //root = optimize_ast(root);

                printf("ACCEPT\n");
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