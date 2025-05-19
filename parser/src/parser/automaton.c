
#include "automaton.h"
#include "grammar.h"
#include "item.h"
#include "state.h"
#include "symbol.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

// ==== Estructura interna para pila de estados ====
typedef struct Node {
    State* state;
    struct Node* next;
} Node;

static void push(Node** stack, State* state) {
    Node* node = malloc(sizeof(Node));
    node->state = state;
    node->next = *stack;
    *stack = node;
}

static State* pop(Node** stack) {
    if (!*stack) return NULL;
    Node* node = *stack;
    *stack = node->next;
    State* s = node->state;
    free(node);
    return s;
}

// ==== Agrega un item si no está repetido ====
bool add_unique_item(Item*** items, int* count, Item* new_item) {
    for (int i = 0; i < *count; ++i) {
        if (compare_items((*items)[i], new_item) == 0) return false;
    }
    *items = realloc(*items, (*count + 1) * sizeof(Item*));
    (*items)[*count] = new_item;
    (*count)++;
    return true;
}

// ==== Clausura LR(0) ====
Item** closure(Item** items, int* count, Grammar* grammar) {
    Item** closure_items = malloc((*count) * sizeof(Item*));
    memcpy(closure_items, items, (*count) * sizeof(Item*));
    int total = *count;
    

    bool changed;
    do {
        changed = false;
        for (int i = 0; i < total; ++i) {
            Item* item = closure_items[i];
            Symbol* next = get_next_symbol(item);
            if (next && next->type == NON_TERMINAL) {
                for (int j = 0; j < grammar->production_count; ++j) {
                    Production* prod = grammar->productions[j];
                    if (prod->left == next) {
                        Item* new_item = create_item(prod, 0);
                        printf("Agregando clausura: %s -> . ...\n", new_item->production->left->name);
                        if (add_unique_item(&closure_items, &total, new_item)) {
                            changed = true;
                        }
                    }
                }
            }
        }
    } while (changed);

    *count = total;
    return closure_items;
}

// ==== Verifica si un estado ya fue creado ====
State* find_existing_state(State** states, int state_count, Item** items, int item_count) {
    for (int i = 0; i < state_count; ++i) {
        State* candidate = states[i];
        if (candidate->item_count != item_count) continue;
        int match = 1;
        for (int j = 0; j < item_count; ++j) {
            if (!compare_items(candidate->items[j], items[j])) {
                match = 0;
                break;
            }
        }
        if (match) return candidate;
    }
    return NULL;
}

// ==== Construcción del autómata LR(0) con clausura ====
State* build_LR0_automaton(Grammar* grammar) {
    Production* start_prod = grammar->productions[0];
    Item* start_item = create_item(start_prod, 0);

    Item** initial_items = malloc(sizeof(Item*));
    initial_items[0] = start_item;
    int initial_count = 1;

    initial_items = closure(initial_items, &initial_count, grammar);
    State* start_state = create_state(initial_items, initial_count);

    Node* pending = NULL;
    push(&pending, start_state);

    State* visited_states[1000];
    int visited_count = 0;
    visited_states[visited_count++] = start_state;

    while (pending) {
        State* current = pop(&pending);

        for (int i = 0; i < current->item_count; ++i) {
            Item* item = current->items[i];
            if (is_reduce_item(item)) continue;

            Symbol* next_symbol = item->production->right[item->pos];

            Item** moved_items = NULL;
            int moved_count = 0;
            for (int j = 0; j < current->item_count; ++j) {
                Item* it = current->items[j];
                if (is_reduce_item(it)) continue;
                if (it->production->right[it->pos] == next_symbol) {
                    Item* moved = next_item(it);
                    moved_items = realloc(moved_items, (moved_count + 1) * sizeof(Item*));
                    moved_items[moved_count++] = moved;
                }
            }

            moved_items = closure(moved_items, &moved_count, grammar);

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


void print_automaton(State* start) {
    if (!start) {
        printf("Autómata vacío.\\n");
        return;
    }

    printf("=== Autómata LR(0) ===\\n");
    State** visited = malloc(1000 * sizeof(State*));
    int visited_count = 0;

    Node* pending = NULL;
    push(&pending, start);
    visited[visited_count++] = start;

    while (pending) {
        State* state = pop(&pending);
        print_state(state);  // esta función debe estar en state.c

        for (Transition* t = state->transitions; t; t = t->next) {
            int found = 0;
            for (int i = 0; i < visited_count; ++i) {
                if (visited[i] == t->next_state) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                visited[visited_count++] = t->next_state;
                push(&pending, t->next_state);
            }
        }
    }

    free(visited);
}
