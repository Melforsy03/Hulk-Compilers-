#include "automaton.h"
#include "grammar.h"
#include <stdlib.h>
#include <stdio.h>

// Estructura para una lista de estados pendientes (pila simple)
typedef struct Node 
{
    State* state;
    struct Node* next;
} Node;

// Función auxiliar para empujar un estado a la pila
void push(Node** stack, State* state) 
{
    Node* node = (Node*)malloc(sizeof(Node));
    node->state = state;
    node->next = *stack;
    *stack = node;
}

// Función auxiliar para sacar un estado de la pila
State* pop(Node** stack) 
{
    if (!*stack) return NULL;
    Node* node = *stack;
    State* s = node->state;
    *stack = node->next;
    free(node);
    return s;
}

// Función auxiliar para saber si un estado ya existe en la lista de visitados (por items)
State* find_existing_state(State** states, int state_count, Item** items, int item_count) 
{
    for (int i = 0; i < state_count; ++i) 
    {
        State* candidate = states[i];
        if (candidate->item_count != item_count) continue;

        int match = 1;
        for (int j = 0; j < item_count; ++j) 
        {
            if (!compare_items(candidate->items[j], items[j])) 
            {
                match = 0;
                break;
            }
        }

        if (match) 
            return candidate;
    }
    return NULL;
}

State* build_LR0_automaton(Grammar* grammar) 
{
    Production* start_prod = grammar->productions[0];

    Item* start_item = create_item(start_prod, 0);

    Item* initial_items[] = { start_item };
    State* start_state = create_state(initial_items, 1);

    Node* pending = NULL;
    push(&pending, start_state);

    State* visited_states[1000]; // Puedes hacer esto dinámico si quieres
    int visited_count = 0;
    visited_states[visited_count++] = start_state;

    while (pending) {
        State* current = pop(&pending);

        // Para cada posible símbolo después del punto
        for (int i = 0; i < current->item_count; ++i) {
            Item* item = current->items[i];
            if (is_reduce_item(item)) continue;

            Symbol* next_symbol = item->production->right[item->pos];

            // Construir nuevos items para el nuevo estado
            Item* moved_items[1000];
            int moved_count = 0;

            for (int j = 0; j < current->item_count; ++j) {
                Item* it = current->items[j];
                if (is_reduce_item(it)) continue;
                if (it->production->right[it->pos] == next_symbol) {
                    moved_items[moved_count++] = next_item(it);
                }
            }

            State* existing = find_existing_state(visited_states, visited_count, moved_items, moved_count);

            State* next_state;
            if (existing) {
                next_state = existing;
            } else {
                next_state = create_state(moved_items, moved_count);
                visited_states[visited_count++] = next_state;
                push(&pending, next_state);
            }

            add_transition(current, next_symbol, next_state);
        }
    }

    return start_state;
}

// Función para imprimir todo el autómata
void print_automaton(State* start) {
    Node* visited = NULL;
    push(&visited, start);

    while (visited) {
        State* state = pop(&visited);
        print_state(state);

        Transition* t = state->transitions;
        while (t) {
            push(&visited, t->next_state);
            t = t->next;
        }
    }
}
