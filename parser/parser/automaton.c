
#include "automaton.h"
#include "grammar.h"
#include "first_follow.h"
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
        
        if (compare_items((*items)[i], new_item) == 1) return false;
    }
    *items = realloc(*items, (*count + 1) * sizeof(Item*));
    (*items)[*count] = new_item;
    (*count)++;
    return true;
}

// ==== Clausura LR(1) - Versión Segura ====
Item** closure(Item** items, int* count, Grammar* grammar, ContainerSet** firsts) {
    if (!items || !count || !grammar || !firsts) {
        fprintf(stderr, "Error: Parámetros inválidos para closure\n");
        return NULL;
    }

    Item** closure_items = NULL;
    int total = 0;

    // Copiar items iniciales con verificación de NULL
    for (int i = 0; i < *count; i++) {
        if (!items[i]) {
            fprintf(stderr, "Error: Item nulo en posición %d\n", i);
            continue;
        }
        
        // Crear copia segura del item
        ContainerSet* lookaheads_copy = NULL;
        if (items[i]->lookaheads) {
            lookaheads_copy = copy_containerset(items[i]->lookaheads);
            if (!lookaheads_copy) {
                fprintf(stderr, "Error: No se pudieron copiar lookaheads\n");
                continue;
            }
        } else {
            lookaheads_copy = create_containerset();
            if (!lookaheads_copy) {
                fprintf(stderr, "Error: No se pudo crear conjunto de lookaheads\n");
                continue;
            }
        }

        Item* new_item = create_item(items[i]->production, items[i]->pos, lookaheads_copy);
        if (!new_item) {
            fprintf(stderr, "Error: No se pudo crear item\n");
            free_containerset(lookaheads_copy);
            continue;
        }

        if (!add_unique_item(&closure_items, &total, new_item)) {
            free_item(new_item);
        }
    }
    
    printf("\n=== Entra en DO WHILE ===\n");
    bool changed;
    do {
        changed = false;
        for (int i = 0; i < total; i++) {
            if (!closure_items[i]) continue;
            
            Symbol* next = get_next_symbol(closure_items[i]);
            if (next && next->type == NON_TERMINAL) {
                // Cálculo seguro de FIRST(beta)
                Symbol** beta = closure_items[i]->production->right + closure_items[i]->pos + 1;
                int beta_len = closure_items[i]->production->right_len - closure_items[i]->pos - 1;
                if (beta_len < 0) beta_len = 0;

                ContainerSet* first_beta = compute_local_first(grammar, firsts, beta, beta_len);

// Impresión detallada de first_beta
printf("\n=== DEBUG first_beta ===\n");
printf("Dirección de memoria: %p\n", (void*)first_beta);

if (first_beta == NULL) {
    printf("first_beta es NULL\n");
} else {
    printf("first_beta no es NULL\n");
    printf("capacity: %d\n", first_beta->capacity);
    printf("symbols array en: %p\n", (void*)first_beta->symbols);
    printf("contains_epsilon: %d\n", first_beta->contains_epsilon);
    printf("size: %d\n", first_beta->size);
    
    if (first_beta->symbols == NULL) {
        printf("symbols es NULL\n");
    } else {
        printf("Símbolos contenidos (%d):\n", first_beta->size);
        for (int i = 0; i < first_beta->size; i++) {
            if (first_beta->symbols[i] == NULL) {
                printf("  [%d] NULL\n", i);
            } else {
                printf("  [%d] %s (type: ", i, first_beta->symbols[i]->name);
                switch (first_beta->symbols[i]->type) {
                    case TERMINAL: printf("TERMINAL"); break;
                    case NON_TERMINAL: printf("NON_TERMINAL"); break;
                    case EPSILON: printf("EPSILON"); break;
                    case EOF_SYM: printf("EOF"); break;
                    default: printf("UNKNOWN"); break;
                }
                printf(")\n");
            }
        }
    }
    
    // Verificación de integridad
    if (first_beta->size > first_beta->capacity) {
        printf("¡ADVERTENCIA! size > capacity (%d > %d)\n", 
              first_beta->size, first_beta->capacity);
    }
    if (first_beta->size < 0) {
        printf("¡ADVERTENCIA! size negativo (%d)\n", first_beta->size);
    }
}
printf("=== FIN DEBUG first_beta ===\n\n");
                
                // Verificación de estructura first_beta
                bool first_beta_valid = (first_beta->symbols != NULL && first_beta->capacity > 0 && first_beta->size >= 0);

                bool should_update = false;
                if (beta_len == 0) {
                    should_update = true;
                }
                else if (first_beta_valid) {
                    should_update = first_beta->contains_epsilon;
                }

                printf("\n=== Entra en should_update ===\n");
                if (should_update) {
                    if (!closure_items[i]->lookaheads) {
                        fprintf(stderr, "Error: lookaheads es NULL para el item %d\n", i);
                        free_containerset(first_beta);
                        continue;
                    }
                    printf("\n=== Entra en containerset_update ===\n");
                    if (!containerset_update(first_beta, closure_items[i]->lookaheads)) {
                        fprintf(stderr, "Error: Fallo al actualizar lookaheads\n");
                        free_containerset(first_beta);
                        continue;
                    }

                    printf("\n=== sale de containerset_update ===\n");
                }
                
                // Añadir producciones de next con estos lookaheads
                for (int j = 0; j < grammar->production_count; j++) {
                    if (grammar->productions[j] && grammar->productions[j]->left == next) {
                        Item* new_item = create_item(grammar->productions[j], 0, first_beta);
                        if (add_unique_item(&closure_items, &total, new_item)) {
                            changed = true;
                        } else {
                            free_item(new_item);
                        }
                    }
                }
                
                // No liberamos first_beta aquí porque ahora pertenece al nuevo item
            }
        }
    } while (changed);

    *count = total;
    return closure_items;
}


// ==== Verifica si un estado ya fue creado ====
State* find_existing_lr1_state(State** states, int state_count, Item** items, int item_count) {
    for (int i = 0; i < state_count; ++i) {
        State* candidate = states[i];
        if (candidate->item_count != item_count) continue;
        
        bool match = true;
        for (int j = 0; j < item_count; ++j) {
            if (!compare_lr1_items(candidate->items[j], items[j])) {
                match = false;
                break;
            }
        }
        if (match) return candidate;
    }
    return NULL;
}

int compare_lr1_items(Item* a, Item* b) {
    if (!compare_items(a, b)) return 0;
    return containerset_equals(a->lookaheads, b->lookaheads);
}

Symbol** get_symbols_after_dot(State* state) {
    Symbol** symbols = NULL;
    int count = 0;
    
    for (int i = 0; i < state->item_count; ++i) {
        Item* item = state->items[i];
        if (is_reduce_item(item)) continue;
        
        Symbol* sym = item->production->right[item->pos];
        if (!sym) continue;
        
        // Verificar si ya está en la lista
        bool found = false;
        for (int j = 0; j < count; ++j) {
            if (symbol_equals(symbols[j], sym)) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            Symbol** temp = realloc(symbols, (count + 1) * sizeof(Symbol*));
            if (!temp) {
                free(symbols);
                return NULL;
            }
            symbols = temp;
            symbols[count++] = sym;
        }
    }
    
    // Añadir marcador de fin
    Symbol** temp = realloc(symbols, (count + 1) * sizeof(Symbol*));
    if (!temp) {
        free(symbols);
        return NULL;
    }
    symbols = temp;
    symbols[count] = NULL;
    
    return symbols;
}

// ==== Modificación completa de build_LR1_automaton ====
State* build_LR1_automaton(Grammar* grammar, ContainerSet** firsts) {
    if (!grammar || !firsts) {
        fprintf(stderr, "Error: Gramática o conjuntos FIRST nulos\n");
        return NULL;
    }

    if (grammar->production_count == 0) {
        fprintf(stderr, "Error: Gramática sin producciones\n");
        return NULL;
    }

    // Item inicial con lookahead $
    Production* start_prod = grammar->productions[0];
    if (!start_prod || !start_prod->left || !start_prod->right) {
        fprintf(stderr, "Error: Producción inicial inválida\n");
        return NULL;
    }
    
    Item* start_item = create_item(start_prod, 0, create_containerset());
    if (!start_item || !start_item->lookaheads) {
    fprintf(stderr, "Error: No se pudo crear item inicial\n");
    return NULL;
    }

    add_symbol_to_set(start_item->lookaheads, grammar->eof);
    
    // Clausura inicial LR(1)
    int initial_count = 1;
    Item** initial_items = malloc(sizeof(Item*));
    if (!initial_items) {
        free_item(start_item);
        return NULL;
    }
    initial_items[0] = start_item;

    printf("\n=== Calculando clausura LR(1) inicial ===\n");
    initial_items = closure(initial_items, &initial_count, grammar, firsts);
    printf("=== Clausura inicial completada con %d items ===\n", initial_count);
                                    
    if (initial_count == 0) {
        fprintf(stderr, "Error: Clausura inicial vacía\n");
        free(initial_items);
        free_item(start_item);
        return NULL;
    }

    // Estado inicial
    State* start_state = create_state(initial_items, initial_count);
    if (!start_state) {
        fprintf(stderr, "Error: No se pudo crear estado inicial\n");
        free(initial_items);
        return NULL;
    }

    // Configuración para BFS
    int visited_capacity = 1000;
    State** visited_states = malloc(sizeof(State*) * visited_capacity);
    if (!visited_states) {
        free_state(start_state);
        return NULL;
    }
    
    int visited_count = 0;
    visited_states[visited_count++] = start_state;

    Node* pending = NULL;
    push(&pending, start_state);
    printf("\n=== Construyendo autómata LR(1) ===\n");

    while (pending) {
        State* current = pop(&pending);
        if (!current) continue;

        printf("\nProcesando estado %d con %d items:\n", current->id, current->item_count);
        
        // Verificar conflictos y marcar estados finales
        for (int i = 0; i < current->item_count; ++i) {
            Item* item = current->items[i];
            print_item(item);
            
            if (is_reduce_item(item)) {
                current->is_final = 1;
                printf("  (Estado de reducción para producción %d)\n", item->production->number);
            }
        }

        // Procesar cada símbolo que sigue al punto
        Symbol** symbols_after_dot = get_symbols_after_dot(current);
        int symbol_count = 0;
        while (symbols_after_dot && symbols_after_dot[symbol_count]) {
            symbol_count++;
        }

        for (int s = 0; s < symbol_count; s++) {
            Symbol* sym = symbols_after_dot[s];
            if (!sym) continue;

            printf("\nProcesando transición con '%s'\n", sym->name);

    

            // Mover items con este símbolo
            Item** moved_items = NULL;
            int moved_count = 0;
            
            for (int i = 0; i < current->item_count; ++i) {
                Item* item = current->items[i];
                if (!is_reduce_item(item) && item->pos < item->production->right_len &&
                    symbol_equals(item->production->right[item->pos], sym)) {
                    
                    Item* moved = next_item(item);
                    if (moved) {
                        Item** temp = realloc(moved_items, (moved_count + 1) * sizeof(Item*));
                        if (!temp) {
                            fprintf(stderr, "Error de memoria al mover items\n");
                            for (int k = 0; k < moved_count; ++k) free_item(moved_items[k]);
                            free(moved_items);
                            goto cleanup;
                        }
                        moved_items = temp;
                        moved_items[moved_count++] = moved;
                    }
                }
            }

            // Calcular clausura LR(1) de los items movidos
            moved_items = closure(moved_items, &moved_count, grammar, firsts);
            
            if (!moved_items || moved_count == 0) {
                printf("Advertencia: Clausura vacía para símbolo '%s'\n", sym->name);
                if (moved_items) free(moved_items);
                continue;
            }

            // Buscar estado existente con los mismos items y lookaheads
            State* next_state = find_existing_lr1_state(visited_states, visited_count, moved_items, moved_count);
            
            if (!next_state) {
                printf("Creando nuevo estado para '%s'\n", sym->name);
                next_state = create_state(moved_items, moved_count);
                if (!next_state) {
                    fprintf(stderr, "Error al crear nuevo estado\n");
                    for (int k = 0; k < moved_count; ++k) free_item(moved_items[k]);
                    free(moved_items);
                    continue;
                }

                // Redimensionar array si es necesario
                if (visited_count >= visited_capacity) {
                    visited_capacity *= 2;
                    State** temp = realloc(visited_states, visited_capacity * sizeof(State*));
                    if (!temp) {
                        free_state(next_state);
                        fprintf(stderr, "Error al redimensionar estados visitados\n");
                        goto cleanup;
                    }
                    visited_states = temp;
                }

                visited_states[visited_count++] = next_state;
                push(&pending, next_state);
                printf("Nuevo estado creado: %d\n", next_state->id);
            } else {
                printf("Estado existente %d encontrado para '%s'\n", next_state->id, sym->name);
                for (int k = 0; k < moved_count; ++k) free_item(moved_items[k]);
                free(moved_items);
            }

            // Añadir transición si no existe
            if (!get_transition(current, sym)) {
                printf("Añadiendo transición de %d a %d con '%s'\n", 
                      current->id, next_state->id, sym->name);
                add_transition(current, sym, next_state);
            }
        }
        
        free(symbols_after_dot);
    }

    printf("\n=== Autómata LR(1) completado con %d estados ===\n", visited_count);
    free(visited_states);
    return start_state;

cleanup:
    free(visited_states);
    while (pending) {
        State* s = pop(&pending);
        free_state(s);
    }
    return NULL;
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
        print_state(state);

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
