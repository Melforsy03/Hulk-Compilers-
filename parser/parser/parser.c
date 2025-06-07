#include "parser.h"
#include "grammar.h"
#include "lr1_table.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Estructura para la pila
typedef struct StackNode 
{
    int state;                // Estado en la pila
    struct StackNode* next;
} StackNode;

// Funciones auxiliares para manejar la pila
void stack_push(StackNode** stack, int state) 
{
    StackNode* node = (StackNode*)malloc(sizeof(StackNode));
    node->state = state;
    node->next = *stack;
    *stack = node;
}

int top(StackNode* stack) 
{
    if (!stack) return -1;
    return stack->state;
}

void stack_pop(StackNode** stack) 
{
    if (!*stack) return;
    StackNode* node = *stack;
    *stack = node->next;
    free(node);
}

void clear_stack(StackNode* stack) 
{
    while (stack) 
    {
        StackNode* next = stack->next;
        free(stack);
        stack = next;
    }
}

int parser(LR1Table* table, Symbol** input_tokens, int token_count, ActionEntryLR1** actions, int* action_count) {
    // Validaciones iniciales
    if (!table || !input_tokens || token_count <= 0 || !actions || !action_count) {
        fprintf(stderr, "Error: Parámetros inválidos para parse\n");
        return 0;
    }

    StackNode* stack = NULL;
    stack_push(&stack, 0); // Estado inicial

    int input_pos = 0;
    Symbol* lookahead = input_tokens[input_pos];
    *action_count = 0;
    *actions = malloc(sizeof(ActionEntryLR1) * 1000); // Tamaño inicial
    if (!*actions) return 0;

    while (1) {
        int state = top(stack);
        if (state < 0 || state >= table->state_count) {
            fprintf(stderr, "Error: Estado %d fuera de rango\n", state);
            clear_stack(stack);
            free(*actions);
            return 0;
        }

        // Buscar símbolo en terminales
        int symbol_idx = -1;
        for (int i = 0; i < table->terminal_count; ++i) {
            if (strcmp(table->grammar->terminals[i]->name, lookahead->name) == 0) {
                symbol_idx = i;
                break;
            }
        }

        if (symbol_idx == -1) {
            fprintf(stderr, "Error: Símbolo '%s' no encontrado en terminales\n", lookahead->name);
            clear_stack(stack);
            free(*actions);
            return 0;
        }

        // Verificar límites de la tabla ACTION
        if (symbol_idx >= table->terminal_count) {
            fprintf(stderr, "Error: Índice de símbolo %d fuera de rango\n", symbol_idx);
            clear_stack(stack);
            free(*actions);
            return 0;
        }

        ActionEntryLR1 action = table->action[state][symbol_idx];
        (*actions)[(*action_count)++] = action;

        printf("Estado %d, %d, Símbolo '%s': ", state, symbol_idx, lookahead->name);
        
        switch (action.action) {
            case ACTION_SHIFT:
                printf("SHIFT a %d\n", action.value);
                if (action.value < 0 || action.value >= table->state_count) {
                    fprintf(stderr, "Error: Estado destino %d inválido\n", action.value);
                    clear_stack(stack);
                    free(*actions);
                    return 0;
                }
                stack_push(&stack, action.value);
                input_pos++;
                lookahead = input_pos < token_count ? input_tokens[input_pos] : table->grammar->eof;
                break;
                
            case ACTION_REDUCE: {
                printf("REDUCE por producción %d\n", action.value);
                Production* prod = NULL;
                    
                // Verificación más robusta
                if (action.value < 0 || action.value >= table->grammar->production_count) {
                    fprintf(stderr, "Error: Número de producción %d inválido\n", action.value);
                    clear_stack(stack);
                    free(*actions);
                    return 0;
                }
    
                for (int i = 0; i < table->grammar->production_count; ++i) {
                    if (table->grammar->productions[i]->number == action.value) {
                        prod = table->grammar->productions[i];
                        break;
                    }
                }

                if (!prod) {
                    fprintf(stderr, "Error: Producción %d no encontrada\n", action.value);
                    clear_stack(stack);
                    free(*actions);
                    return 0;
                }
                
                for (int i = 0; i < prod->right_len; ++i) {
                    if (!stack) {
                        fprintf(stderr, "Error: Stack underflow\n");
                        return 0;
                    }
                    stack_pop(&stack);
                }

                state = top(stack);
                int nonterm_idx = index_of_symbol(table->grammar->nonterminals, table->grammar->nonterminals_count, prod->left);
                
                if (nonterm_idx == -1) {
                    fprintf(stderr, "Error: No terminal %s no encontrado\n", prod->left->name);
                    clear_stack(stack);
                    free(*actions);
                    return 0;
                }

                int goto_state = table->goto_table[state][nonterm_idx];
                if (goto_state == -1) {
                    fprintf(stderr, "Error: Entrada GOTO vacía para %s\n", prod->left->name);
                    clear_stack(stack);
                    free(*actions);
                    return 0;
                }
                
                stack_push(&stack, goto_state);
                break;
            }
                
            case ACTION_ACCEPT:
                printf("ACCEPT\n");
                clear_stack(stack);
                return 1;
                
            case ACTION_ERROR:
            default:
                fprintf(stderr, "Error: Acción no válida\n");
                // Imprimir información de depuración
                fprintf(stderr, "Posibles acciones en este estado:\n");
                for (int k = 0; k < table->terminal_count; k++) {
                    if (table->action[state][k].action != ACTION_ERROR) {
                        fprintf(stderr, "  %s: %s %d\n", 
                            table->grammar->terminals[k]->name,
                            table->action[state][k].action == ACTION_SHIFT ? "SHIFT" : "REDUCE",
                            table->action[state][k].value);
                    }
                }
                clear_stack(stack);
                free(*actions);
                *actions = NULL;  
                return 0;
        }
    }
}