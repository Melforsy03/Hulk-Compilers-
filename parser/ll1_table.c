// Archivo: ll1_table.c
#include <stdio.h>
#include <string.h>
#include "ll1_table.h"
#include "first_follow.h" // Se necesita para is_symbol_terminal y add_to_set
#include <stdlib.h>  
// La definición de is_symbol_terminal se elimina de aquí.
// Se usará la función is_symbol_terminal de first_follow.c
/*
int is_symbol_terminal(const Grammar* g, const char* symbol) { // <--- ELIMINADO
    for (int i = 0; i < g->num_terminals; ++i)
        if (strcmp(g->terminals[i], symbol) == 0) return 1;
    if (strcmp(symbol, "ε") == 0) return 1;
    return 0;
}
*/


static void add_entry(LL1Table* table, const char* A, const char* t, Production* p) {
    // Evita duplicados
    for (int i = 0; i < table->num_entries; ++i) {
        if (strcmp(table->entries[i].non_terminal, A) == 0 &&
            strcmp(table->entries[i].terminal, t) == 0) {
            // Manejo de conflictos: si ya existe una entrada y es diferente, esto es un conflicto LL(1)
            if (table->entries[i].production != p) {
                fprintf(stderr, "⚠️ Conflicto LL(1): M[%s, %s] ya definido con una producción diferente. (%s ::= ... vs %s ::= ...)\n", A, t, table->entries[i].production->lhs, p->lhs);
                // Si hay un conflicto, puedes decidir cómo manejarlo, por ahora sobrescribimos o reportamos un error fatal.
                // Para depuración, es útil saber cuándo ocurre un conflicto.
                exit(1); // Error fatal en caso de conflicto
            }
            return; // Si es la misma producción, no hacemos nada (duplicado exacto)
        }
    }
    // Asegurarse de que no desbordamos el límite de entradas
    if (table->num_entries >= MAX_CELLS) { // MAX_CELLS está definido en ll1_table.h
        fprintf(stderr, "Error: Tabla LL(1) excede MAX_CELLS. No se pudo añadir la entrada M[%s, %s].\n", A, t);
        exit(1);
    }
    strcpy(table->entries[table->num_entries].non_terminal, A);
    strcpy(table->entries[table->num_entries].terminal, t);
    table->entries[table->num_entries].production = p;
    table->num_entries++;
}

void init_ll1_table(LL1Table* table) {
    table->num_entries = 0;
}

void generate_ll1_table(const Grammar* g, const FirstFollowTable* ff_table, LL1Table* table) {
    for (int p = 0; p < g->num_productions; ++p) {
        Production* prod = &g->productions[p];
        const char* A = prod->lhs;

        // Calcula FIRST(α) para la producción actual A ::= α
        char first_alpha[MAX_SET][32];
        int num_first_alpha = 0;
        int alpha_derives_epsilon = 1; // Asumimos que RHS deriva epsilon hasta que se demuestre lo contrario

        if (prod->rhs_len == 1 && strcmp(prod->rhs[0], "ε") == 0) { // Si la producción es directamente A ::= ε
            strcpy(first_alpha[num_first_alpha++], "ε");
            alpha_derives_epsilon = 1;
        } else {
            for (int i = 0; i < prod->rhs_len; ++i) {
                const char* Xi = prod->rhs[i];
                if (is_symbol_terminal(g, Xi)) { // Si Xi es un terminal
                    add_to_set(first_alpha, &num_first_alpha, Xi);
                    alpha_derives_epsilon = 0; // Un terminal no deriva epsilon
                    break; // Parar de procesar el RHS para FIRST
                } else { // Si Xi es un no terminal
                    FirstFollowEntry* entry_Xi = get_entry((FirstFollowTable*)ff_table, Xi);
                    if (!entry_Xi) {
                        fprintf(stderr, "Error: No terminal '%s' no encontrado en la tabla FIRST/FOLLOW al calcular FIRST(alpha).\n", Xi);
                        exit(1);
                    }

                    int Xi_has_epsilon = 0;
                    for (int j = 0; j < entry_Xi->num_first; ++j) {
                        if (strcmp(entry_Xi->first[j], "ε") == 0) {
                            Xi_has_epsilon = 1;
                        } else {
                            add_to_set(first_alpha, &num_first_alpha, entry_Xi->first[j]);
                        }
                    }
                    if (!Xi_has_epsilon) {
                        alpha_derives_epsilon = 0; // Xi no deriva epsilon, por lo tanto alpha tampoco
                        break; // Parar de procesar el RHS para FIRST
                    }
                    // Si Xi deriva epsilon, continuar con el siguiente símbolo en RHS para calcular FIRST(alpha)
                }
            }
        }
        
        // Regla 1: Para cada terminal 't' en FIRST(α) - {ε}, añade M[A, t] = A ::= α
        for (int i = 0; i < num_first_alpha; ++i) {
            if (strcmp(first_alpha[i], "ε") != 0) {
                add_entry(table, A, first_alpha[i], prod);
            }
        }

        // Regla 2: Si ε ∈ FIRST(α), entonces para cada terminal 'b' en FOLLOW(A), añade M[A, b] = A ::= α
        if (alpha_derives_epsilon) {
            FirstFollowEntry* entry_A_ff = get_entry((FirstFollowTable*)ff_table, A);
            if (!entry_A_ff) {
                fprintf(stderr, "Error: No terminal '%s' no encontrado en la tabla FIRST/FOLLOW para FOLLOW.\\n", A);
                exit(1);
            }
            for (int j = 0; j < entry_A_ff->num_follow; ++j) {
                add_entry(table, A, entry_A_ff->follow[j], prod);
            }
        }
    }
}

void print_ll1_table(const LL1Table* table) {
    printf("\n=== LL(1) PARSING TABLE ===\n");
    // Esto es una impresión básica, considera una matriz para una mejor visualización.
    for (int i = 0; i < table->num_entries; ++i) {
        printf("M[%s, %s] = %s ::= ", 
               table->entries[i].non_terminal, 
               table->entries[i].terminal, 
               table->entries[i].production->lhs);
        for (int j = 0; j < table->entries[i].production->rhs_len; ++j) {
            printf("%s ", table->entries[i].production->rhs[j]);
        }
        printf("\n");
    }
    printf("Total entries: %d\n", table->num_entries);
}