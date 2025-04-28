#include "parser.h"
#include <stdlib.h>
#include <stdio.h>

// Estructura para la pila
typedef struct StackNode 
{
    int state;                // Estado en la pila
    struct StackNode* next;
} StackNode;

// Funciones auxiliares para manejar la pila
void push(StackNode** stack, int state) 
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

void pop(StackNode** stack) 
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

// Función principal de parsing
int parse(SLR1Table* table, Symbol** input_tokens, int token_count) 
{
    StackNode* stack = NULL;
    push(&stack, 0);  // Estado inicial 0

    int input_pos = 0;
    Symbol* lookahead = input_tokens[input_pos];

    while (1) 
    {
        int state = top(stack);

        // Encontrar índice del símbolo en la tabla
        int symbol_idx = -1;
        for (int i = 0; i < table->terminal_count; ++i) 
        {
            if (strcmp(table->grammar->terminals[i]->name, lookahead->name) == 0) 
            {
                symbol_idx = i;
                break;
            }
        }

        if (symbol_idx == -1) 
        {
            printf("Error: símbolo '%s' no reconocido en la tabla.\n", lookahead->name);
            clear_stack(stack);
            return 0;
        }

        ActionEntrySLR action = table->action[state][symbol_idx];

        printf("Estado actual: %d, símbolo: '%s' -> ", state, lookahead->name);

        if (action.action == ACTION_SHIFT) 
        {
            printf("Acción: SHIFT a estado %d\n", action.value);
            push(&stack, action.value);
            input_pos++;
            if (input_pos < token_count)
                lookahead = input_tokens[input_pos];
            else
                lookahead = table->grammar->eof;  // Al final agregamos EOF
        } 
        else if (action.action == ACTION_REDUCE) 
        {
            printf("Acción: REDUCE usando producción %d\n", action.value);

            // Buscar producción a partir del número
            Production* prod = NULL;
            for (int i = 0; i < table->grammar->productions_count; ++i) 
            {
                if (table->grammar->productions[i]->number == action.value) 
                {
                    prod = table->grammar->productions[i];
                    break;
                }
            }

            if (!prod) 
            {
                printf("Error: producción %d no encontrada.\n", action.value);
                clear_stack(stack);
                return 0;
            }

            // Hacer POP según la longitud de la derecha de la producción
            for (int i = 0; i < prod->right_len; ++i) 
            {
                pop(&stack);
            }

            // GOTO
            state = top(stack);

            int nonterm_idx = -1;
            for (int i = 0; i < table->nonterminal_count; ++i) 
            {
                if (strcmp(table->grammar->nonterminals[i]->name, prod->left->name) == 0) 
                {
                    nonterm_idx = i;
                    break;
                }
            }

            if (nonterm_idx == -1) 
            {
                printf("Error: no se encontró el no terminal '%s'.\n", prod->left->name);
                clear_stack(stack);
                return 0;
            }

            int next_state = table->goto_table[state][nonterm_idx];
            printf("GOTO a estado %d por '%s'\n", next_state, prod->left->name);
            push(&stack, next_state);
        } 
        else if (action.action == ACTION_ACCEPT) 
        {
            printf("Acción: ACCEPT\n");
            clear_stack(stack);
            return 1;  // Cadena aceptada
        } 
        else 
        {
            printf("Acción: ERROR\n");
            clear_stack(stack);
            return 0;  // Cadena rechazada
        }
    }
}
