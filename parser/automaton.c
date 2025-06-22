#include "automaton.h"
#include "../grammar/grammar.h"
#include "first_follow.h"
#include "item.h"
#include "lr1_table.h"
#include "state.h"
#include "../grammar/symbol.h"
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

// ==== Clausura LR(1) ====
Item** closure(Item** items, int* count, Grammar* grammar, ContainerSet** firsts) {
    if (!items || !count || !grammar || !firsts) {
        //fprintf(stderr, "Error: Parámetros inválidos para closure\n");
        return NULL;
    }

    Item** closure_items = NULL;
    int total = 0;

    // Copiar items iniciales
    for (int i = 0; i < *count; i++) {
        if (!items[i]) continue;
        
        ContainerSet* lookaheads_copy = copy_containerset(items[i]->lookaheads);
        if (!lookaheads_copy) continue;

        Item* new_item = create_item(items[i]->production, items[i]->pos, lookaheads_copy);
        if (!new_item) {
            free_containerset(lookaheads_copy);
            continue;
        }

        if (!add_unique_item(&closure_items, &total, new_item)) {
            free_item(new_item);
        }
    }

   // printf("\n=== INICIO CLAUSURA ===\n");
    for (int i = 0; i < *count; i++) {
        if (items[i]) {
           // printf("Item inicial %d: ", i);
            //print_item(items[i]);
           // printf("Lookaheads: ");
            //print_containerset(items[i]->lookaheads, "Lookaheads originales");
        }
    }

    bool changed;
    do {
        changed = false;
        Item* padre = items[0];
        for (int i = 0; i < total; i++) {
            Item* item = closure_items[i];
            Symbol* next = get_next_symbol(item);

            if (!next) continue;
            if( next->type == NON_TERMINAL) {
                // Calcular FIRST(beta a)
               // printf("\n=== CÁLCULO DE BETA ===\n");
                Symbol** beta = item->production->right + item->pos + 1;
                int beta_len = item->production->right_len - item->pos - 1;

               // printf("\nLongitud de beta: %d\n", beta_len);
                ContainerSet* first_beta = compute_local_first(grammar, firsts, beta, beta_len);
                if (!first_beta) continue;
                
                if (beta_len <= 0) {
                   // printf("ε (epsilon)");
                }
                else{
                   // printf("Beta :");
                    for (int k = 0; k < beta_len; k++) {
                       // printf(" %s", beta[k]->name);
                    }
                }

               // printf("\nProcesando item:");
                //print_item(item);
               // printf("Símbolo después del punto: %s\n", next->name);
               // printf("FIRST(beta): ");
                //print_containerset(first_beta, "FIRST(beta) calculado");


                // Si beta puede derivar epsilon, añadir los lookaheads del item original
                // if (beta_len == 0 || (first_beta && first_beta->contains_epsilon)) {
                //     if (padre->lookaheads) {
                //         first_beta = padre->lookaheads;
                //     }
                //     printf("Beta deriva epsilon, añadiendo lookaheads originales\n");
                //     printf("Lookaheads antes de añadir: ");
                //     print_containerset(first_beta, "first_beta");
                //     printf("Lookaheads a añadir: ");
                //     print_containerset(padre->lookaheads, "item->lookaheads");
                // }
                
                ContainerSet* alt_lookaheads = copy_containerset(first_beta);
                if (beta_len == 0 || (first_beta && first_beta->contains_epsilon)) {
                    containerset_update(alt_lookaheads, padre->lookaheads);
                }
                first_beta = alt_lookaheads;

                if(beta_len > 0)
                if(first_beta->symbols[0]->type == TERMINAL)   // Si lo q esta en lookahead ahora es  un operador.
                for (int j = 0; j < total; j++) 
                {
                    if (symbol_equals(grammar->productions[j]->left, next)) {

                       // printf("\n=== CÁLCULO DE BETA DE LA COINCIDENCIA ===\n");
                        //print_production(grammar->productions[j]);
                        ContainerSet* alt_lookaheads = copy_containerset(first_beta);
                        Symbol** beta_ = grammar->productions[j]->right + item->pos + 1;
                        int beta_len_ = grammar->productions[j]->right_len - item->pos - 1;
                        
                        ContainerSet* first_betaC = compute_local_first(grammar, firsts, beta_, beta_len_);
                        //print_containerset(first_betaC, "FIRST(beta) coincidencia calculado");
                        if (beta_len_ <= 0) {
                           // printf("ε (epsilon)");
                        }
                        else{
                            if(beta_[0]->type == NON_TERMINAL) continue;
                           // printf("Beta_ :");
                            for (int k = 0; k < beta_len_; k++) {
                               // printf(" %s", beta_[k]->name);
                            }
                        }
                        // Si beta puede derivar epsilon, añadir los lookaheads del item original
                        if (beta_len_ == 0 || (first_betaC && first_betaC->contains_epsilon)) {
                            if (padre->lookaheads) {
                               // printf("first_betaC = padre->lookaheads;");
                                first_betaC = padre->lookaheads;
                            }
                        }

                        containerset_update(alt_lookaheads,first_betaC);
                        first_beta = alt_lookaheads;
                        // if (item->pos+1 < grammar->productions[j]->right_len && grammar->productions[j]->right_len > 0)
                        // {
                        //     if (is_operator(grammar->productions[j]->right[item->pos + 1])){
                        //         add_symbol_to_set(alt_lookaheads, grammar->productions[j]->right[item->pos+1]);
                        //     }

                            
                        //     Item* new_item = create_item(grammar->productions[j], 0, alt_lookaheads);
                        //     if (add_unique_item(&closure_items, &total, new_item)) {
                        //     changed = true;
                        //     } else {
                        //     free_item(new_item);
                        //     }
                        // }
                    }
                }

                containerset_update(padre->lookaheads, first_beta);
                //padre->lookaheads = first_beta;

                //containerset_update(item->lookaheads, item->lookaheads);
                // Añadir producciones del no terminal con estos lookaheads
                for(int j = 0; j < grammar->production_count; j++) {
                    if(grammar->productions[j]->left == next) {
                        Item* new_item = create_item(grammar->productions[j], 0, first_beta);

                        if(add_unique_item(&closure_items, &total, new_item)) {
                            changed = true;
                        }
                        else {
                           free_item(new_item);
                        }
                        containerset_update(item->lookaheads, new_item->lookaheads);
                    }
                }
                
            }
        }
    } while (changed);

   // printf("\n=== FIN CLAUSURA ===\n");
   // printf("Items resultantes (%d):\n", total);
    for (int i = 0; i < total; i++) {
        Item* item = closure_items[i];
       // printf("Item %d: ", i);
        //print_item(item);
        //print_containerset(item->lookaheads, "Lookaheads finales");
    }

    *count = total;
    return closure_items;
}

int compare_lr1_items(Item* a, Item* b) {
    if (!a || !b) return 0;
    if (!compare_items(a, b)) return 0;
    
    // Ambos tienen lookaheads NULL
    if (!a->lookaheads && !b->lookaheads) return 1;
    
    // Solo uno tiene lookaheads NULL
    if (!a->lookaheads || !b->lookaheads) return 0;
    
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

State* build_LR1_automaton(Grammar* grammar, ContainerSet** firsts) {
    if (!grammar || !firsts) {
        //fprintf(stderr, "Error: Gramática o conjuntos FIRST nulos\n");
        return NULL;
    }

    if (grammar->production_count == 0) {
        //fprintf(stderr, "Error: Gramática sin producciones\n");
        return NULL;
    }

    // Item inicial con lookahead $
    Production* start_prod = grammar->productions[0];
    if (!start_prod || !start_prod->left || !start_prod->right) {
        //fprintf(stderr, "Error: Producción inicial inválida\n");
        return NULL;
    }
    
    Item* start_item = create_item(start_prod, 0, create_containerset());
    if (!start_item || !start_item->lookaheads) {
    //fprintf(stderr, "Error: No se pudo crear item inicial\n");
    return NULL;
    }

   // printf("\n=== ITEM INICIAL ===\n");
    //print_item(start_item);
    //print_containerset(start_item->lookaheads, "Lookaheads iniciales");

    add_symbol_to_set(start_item->lookaheads, grammar->eof);

    // Clausura inicial LR(1)
    int initial_count = 1;
    Item** initial_items = malloc(sizeof(Item*));
    if (!initial_items) {
        free_item(start_item);
        return NULL;
    }
    initial_items[0] = start_item;

   // printf("\n=== CLAUSURA INICIAL ===\n");
    for (int i = 0; i < initial_count; i++) {
       // printf("Item %d: ", i);
        //print_item(initial_items[i]);
        //print_containerset(initial_items[i]->lookaheads, "Lookaheads");
    }

   // printf("\n=== Calculando clausura LR(1) inicial ===\n");
    initial_items = closure(initial_items, &initial_count, grammar, firsts);
   // printf("=== Clausura inicial completada con %d items ===\n", initial_count);
                        
    if (initial_count == 0) {
        //fprintf(stderr, "Error: Clausura inicial vacía\n");
        free(initial_items);
        free_item(start_item);
        return NULL;
    }

    // Estado inicial
    State* start_state = create_state(initial_items, initial_count);
    if (!start_state) {
        //fprintf(stderr, "Error: No se pudo crear estado inicial\n");
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
   // printf("\n=== Construyendo autómata LR(1) ===\n");

    while (pending) {
        State* current = pop(&pending);
        if (!current) continue;

       // printf("\n=== PROCESANDO ESTADO %d ===\n", current->id);
        
        for (int i = 0; i < current->item_count; ++i) {
           // printf("Item %d: ", i);
            //print_item(current->items[i]);
            //print_containerset(current->items[i]->lookaheads, "Lookaheads");
            
            if (is_reduce_item(current->items[i])) {
               // printf("  (REDUCE usando producción %d)\n", current->items[i]->production->number);
            }
        }
        
        // Verificar conflictos y marcar estados finales
        for (int i = 0; i < current->item_count; ++i) {
            Item* item = current->items[i];
            //print_item(item);
            
            if (is_reduce_item(item)) {
                current->is_final = 1;
               // printf("  (Estado de reducción para producción %d)\n", item->production->number);
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

           // printf("\nProcesando transición con '%s'\n", sym->name);

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
                            //fprintf(stderr, "Error de memoria al mover items\n");
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
               // printf("Advertencia: Clausura vacía para símbolo '%s'\n", sym->name);
                if (moved_items) free(moved_items);
                continue;
            }

            // Buscar estado existente con los mismos items y lookaheads
            State* next_state = find_existing_lalr_state(visited_states, visited_count, moved_items, moved_count);
            
            if (!next_state) {
               // printf("Creando nuevo estado para '%s'\n", sym->name);
                next_state = create_state(moved_items, moved_count);
                if (!next_state) {
                    //fprintf(stderr, "Error al crear nuevo estado\n");
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
                        //fprintf(stderr, "Error al redimensionar estados visitados\n");
                        goto cleanup;
                    }
                    visited_states = temp;
                }

                visited_states[visited_count++] = next_state;
                push(&pending, next_state);
               // printf("Nuevo estado creado: %d\n", next_state->id);
            } 
            else {
               // printf("Estado existente %d encontrado para '%s'\n", next_state->id, sym->name);
                for (int k = 0; k < moved_count; ++k) free_item(moved_items[k]);
                free(moved_items);
            }

            // Añadir transición si no existe
            if (!get_transition(current, sym)) {
               // printf("Añadiendo transición de %d a %d con '%s'\n", current->id, next_state->id, sym->name);
                add_transition(current, sym, next_state);
            }
        }
        
        free(symbols_after_dot);
    }
    State** states = visited_states;    // tu array de estados
    int n_states = visited_count;       // total de estados generados

    for (int i = 0; i < n_states; ++i) {
        if (i <= 26)
        {
            //print_state_lookaheads(states[i], grammar);
           
        }
    }
   // printf("\n=== Autómata LR(1) completado con %d estados ===\n", visited_count);
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
       // printf("Autómata vacío.\\n");
        return;
    }

   // printf("=== Autómata LR(0) ===\\n");
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
