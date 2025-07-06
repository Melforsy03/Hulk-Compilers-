#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "first_follow.h" // Incluye la declaración de is_symbol_terminal
#include "grammar.h" // Asegúrate de que grammar.h esté incluido para la estructura Grammar


// add_to_set (ya no static, se declara en first_follow.h)
void add_to_set(char set[MAX_SET][32], int* size, const char* symbol) {
    for (int i = 0; i < *size; ++i) {
        if (strcmp(set[i], symbol) == 0) return; // ya está
    }
    if (*size < MAX_SET) { // Prevenir desbordamiento del buffer
        strcpy(set[(*size)++], symbol);
    } else {
        fprintf(stderr, "Advertencia: Conjunto excede MAX_SET. No se pudo añadir '%s'\n", symbol);
    }
}

// is_non_terminal permanece static ya que solo se usa aquí
static int is_non_terminal(const Grammar* g, const char* symbol) {
    for (int i = 0; i < g->num_non_terminals; ++i)
        if (strcmp(g->non_terminals[i], symbol) == 0) return 1;
    return 0;
}

// Implementación de la función unificada is_symbol_terminal
int is_symbol_terminal(const Grammar* g, const char* symbol) {
    if (symbol == NULL) {
        fprintf(stderr, "Error: is_symbol_terminal recibió symbol = NULL.\n");
        exit(1);
    }

    for (int i = 0; i < g->num_terminals; ++i) {
        if (strcmp(g->terminals[i], symbol) == 0) return 1;
    }
    if (strcmp(symbol, "ε") == 0) return 1;

    return 0;
}



void init_first_follow_table(FirstFollowTable* table, const Grammar* g) {
    table->num_entries = 0;
    for (int i = 0; i < g->num_non_terminals; ++i) {
        strcpy(table->entries[i].non_terminal, g->non_terminals[i]);
        table->entries[i].num_first = 0;
        table->entries[i].num_follow = 0;
    }
    table->num_entries = g->num_non_terminals;
}

void compute_first(const Grammar* g, FirstFollowTable* table) {
    int changed;
    do {
        changed = 0;

        for (int p = 0; p < g->num_productions; ++p) {
            const char* A = g->productions[p].lhs;
            
            FirstFollowEntry* entry_A = NULL;
            for (int i = 0; i < table->num_entries; ++i)
                if (strcmp(table->entries[i].non_terminal, A) == 0)
                    entry_A = &table->entries[i];

            int rhs_derives_epsilon = 1; 
            for (int i = 0; i < g->productions[p].rhs_len; ++i) {
                const char* Xi = g->productions[p].rhs[i];

                if (is_symbol_terminal(g, Xi)) { 
                    int old_size = entry_A->num_first;
                    add_to_set(entry_A->first, &entry_A->num_first, Xi);
                    if (entry_A->num_first > old_size) changed = 1;
                    rhs_derives_epsilon = 0; 
                    break;
                } else if (is_non_terminal(g, Xi)) { 
                    FirstFollowEntry* entry_Xi = NULL;
                    for (int e = 0; e < table->num_entries; ++e)
                        if (strcmp(table->entries[e].non_terminal, Xi) == 0)
                            entry_Xi = &table->entries[e];

                    int Xi_has_epsilon = 0;
                    for (int j = 0; j < entry_Xi->num_first; ++j) {
                        if (strcmp(entry_Xi->first[j], "ε") == 0) {
                            Xi_has_epsilon = 1;
                        } else {
                            int old_size = entry_A->num_first;
                            add_to_set(entry_A->first, &entry_A->num_first, entry_Xi->first[j]);
                            if (entry_A->num_first > old_size) changed = 1;
                        }
                    }
                    if (!Xi_has_epsilon) {
                        rhs_derives_epsilon = 0;
                        break;
                    }
                }
            }
            if (rhs_derives_epsilon) {
                int old_size = entry_A->num_first;
                add_to_set(entry_A->first, &entry_A->num_first, "ε");
                if (entry_A->num_first > old_size) changed = 1;
            }
        }
    } while (changed);
}
void compute_follow(const Grammar* g, FirstFollowTable* table) {
    int changed;

    // 1️⃣ Añadir $ al FOLLOW del símbolo inicial
    FirstFollowEntry* start_entry = NULL;
    for (int i = 0; i < table->num_entries; ++i) {
        if (strcmp(table->entries[i].non_terminal, g->start_symbol) == 0) {
            start_entry = &table->entries[i];
            break;
        }
    }
    if (start_entry) {
        add_to_set(start_entry->follow, &start_entry->num_follow, "$");
    } else {
        fprintf(stderr, "Error: Símbolo inicial '%s' no encontrado en la tabla FIRST/FOLLOW.\n", g->start_symbol);
        exit(1);
    }

    do {
        changed = 0;

        for (int p = 0; p < g->num_productions; ++p) {
            const Production prod = g->productions[p];
            const char* A = prod.lhs;

            for (int i = 0; i < prod.rhs_len; ++i) {
                const char* B = prod.rhs[i];

                if (is_non_terminal(g, B)) {
                    // Busca entrada FOLLOW(B)
                    FirstFollowEntry* entry_B = NULL;
                    for (int e = 0; e < table->num_entries; ++e) {
                        if (strcmp(table->entries[e].non_terminal, B) == 0) {
                            entry_B = &table->entries[e];
                            break;
                        }
                    }
                    if (!entry_B) {
                        fprintf(stderr, "Error: No terminal '%s' no encontrado en FIRST/FOLLOW.\n", B);
                        exit(1);
                    }

                    int beta_derives_epsilon_seq = 1;

                    // 2️⃣ Procesa beta (símbolos a la derecha de B)
                    for (int j = i + 1; j < prod.rhs_len; ++j) {
                        const char* Yj = prod.rhs[j];

                        if (is_symbol_terminal(g, Yj)) {
                            int old_size = entry_B->num_follow;
                            add_to_set(entry_B->follow, &entry_B->num_follow, Yj);
                            if (entry_B->num_follow > old_size) changed = 1;

                            beta_derives_epsilon_seq = 0;
                            break; // Parar: terminal no deriva ε
                        }
                        else if (is_non_terminal(g, Yj)) {
                            FirstFollowEntry* entry_Yj = NULL;
                            for (int e = 0; e < table->num_entries; ++e) {
                                if (strcmp(table->entries[e].non_terminal, Yj) == 0) {
                                    entry_Yj = &table->entries[e];
                                    break;
                                }
                            }

                            int Yj_has_epsilon = 0;
                            for (int k = 0; k < entry_Yj->num_first; ++k) {
                                const char* first_sym = entry_Yj->first[k];
                                if (strcmp(first_sym, "ε") == 0) {
                                    Yj_has_epsilon = 1;
                                } else {
                                    int old_size = entry_B->num_follow;
                                    add_to_set(entry_B->follow, &entry_B->num_follow, first_sym);
                                    if (entry_B->num_follow > old_size) changed = 1;
                                }
                            }

                            if (!Yj_has_epsilon) {
                                beta_derives_epsilon_seq = 0;
                                break; // Parar: FIRST(Yj) no contiene ε
                            }
                        }
                        else {
                            fprintf(stderr, "Advertencia: Símbolo '%s' no reconocido en RHS de '%s'.\n", Yj, A);
                            beta_derives_epsilon_seq = 0;
                            break;
                        }
                    }

                    // 3️⃣ Si beta deriva ε o beta está vacío: FOLLOW(A) += FOLLOW(B)
                    if (beta_derives_epsilon_seq) {
                        FirstFollowEntry* entry_A = NULL;
                        for (int e = 0; e < table->num_entries; ++e) {
                            if (strcmp(table->entries[e].non_terminal, A) == 0) {
                                entry_A = &table->entries[e];
                                break;
                            }
                        }
                        if (!entry_A) {
                            fprintf(stderr, "Error: LHS '%s' no encontrado en tabla FOLLOW.\n", A);
                            exit(1);
                        }

                        for (int j = 0; j < entry_A->num_follow; ++j) {
                            const char* follow_sym = entry_A->follow[j];
                            int old_size = entry_B->num_follow;
                            add_to_set(entry_B->follow, &entry_B->num_follow, follow_sym);
                            if (entry_B->num_follow > old_size) changed = 1;
                        }
                    }
                }
            }
        }
    } while (changed);

    // ✅ 4️⃣ FILTRO FINAL para evitar FIRST ∩ FOLLOW
    for (int i = 0; i < table->num_entries; ++i) {
        FirstFollowEntry* entry = &table->entries[i];
        for (int j = 0; j < entry->num_first; ++j) {
            const char* first_sym = entry->first[j];
            if (strcmp(first_sym, "ε") != 0) {
                for (int k = 0; k < entry->num_follow; ++k) {
                    if (strcmp(entry->follow[k], first_sym) == 0) {
                        // Remueve el duplicado
                        for (int m = k; m < entry->num_follow - 1; ++m) {
                            strcpy(entry->follow[m], entry->follow[m+1]);
                        }
                        entry->num_follow--;
                        break; // solo una ocurrencia
                    }
                }
            }
        }
    }
}

void print_first_follow(const FirstFollowTable* table) {
    printf("=== FIRST & FOLLOW ===\n");
    for (int i = 0; i < table->num_entries; ++i) {
        printf("Non-terminal: %s\n", table->entries[i].non_terminal);

        printf("  FIRST: { ");
        for (int j = 0; j < table->entries[i].num_first; ++j)
            printf("%s ", table->entries[i].first[j]);
        printf("}\n");

        printf("  FOLLOW: { ");
        for (int j = 0; j < table->entries[i].num_follow; ++j)
            printf("%s ", table->entries[i].follow[j]);
        printf("}\n\n");
    }
}

FirstFollowEntry* get_entry(FirstFollowTable* table, const char* non_terminal) {
    for (int i = 0; i < table->num_entries; ++i) {
        if (strcmp(table->entries[i].non_terminal, non_terminal) == 0) {
            return &table->entries[i];
        }
    }
    return NULL;
}