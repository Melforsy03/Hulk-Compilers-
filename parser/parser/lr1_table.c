#include "lr1_table.h"
#include "grammar.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Función auxiliar para recolectar todos los estados del autómata
void collect_states_lr1(State* start, State** states, int* state_count) {
    int visited_capacity = 1000;
    int visited_count = 0;
    states[visited_count++] = start;

    for (int i = 0; i < visited_count; ++i) {
        State* current = states[i];
        Transition* t = current->transitions;
        while (t) {
            int found = 0;
            for (int j = 0; j < visited_count; ++j) {
                if (states[j] == t->next_state) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                if (visited_count >= visited_capacity) {
                    fprintf(stderr, "Error: Demasiados estados en el autómata\n");
                    return;
                }
                states[visited_count++] = t->next_state;
            }
            t = t->next;
        }
    }

    *state_count = visited_count;
}

int index_of_symbol(Symbol** list, int count, Symbol* s) {
    if (!s) return -1;
    for (int i = 0; i < count; ++i) {
        if (list[i] && strcmp(list[i]->name, s->name) == 0) {
            return i;
        }
    }
    return -1;
}

// Función para registrar conflictos
void log_conflict(const char* type, int state, const char* symbol_name, int production1, int production2) {
    FILE* log_file = fopen("conflicts.log", "a");
    if (!log_file) {
        perror("Error al abrir conflicts.log");
        return;
    }

    time_t now = time(NULL);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(log_file, "[%s] Conflicto %s:\n", timestamp, type);
    fprintf(log_file, "  - Estado: %d\n", state);
    fprintf(log_file, "  - Símbolo: '%s'\n", symbol_name);
    fprintf(log_file, "  - Producciones en conflicto: %d vs %d\n\n", production1, production2);
    fclose(log_file);
}

// Función para verificar si un símbolo es operador
int is_operator(Symbol* sym) {
    if (!sym || sym->type != TERMINAL) return 0;
    const char* ops[] = {"+", "-", "*", "/", "^", "==", "!=", ">", "<", ">=", "<=", ":=", "%", "@"};
    for (size_t i = 0; i < sizeof(ops)/sizeof(ops[0]); i++) {
        if (strcmp(sym->name, ops[i]) == 0) return 1;
    }
    return 0;
}

LR1Table* build_lr1_table(State* start, Grammar* grammar) {
    // Validaciones iniciales
    if (!start || !grammar) {
        fprintf(stderr, "Error: Parámetros inválidos para construir tabla\n");
        return NULL;
    }

    // Recolectar todos los estados del autómata
    State* states[1000];
    int state_count = 0;
    collect_states_lr1(start, states, &state_count);
    
    printf("=== Construyendo tabla LR(1) con %d estados === \n", state_count);

    // Inicializar tabla
    LR1Table* table = malloc(sizeof(LR1Table));
    if (!table) return NULL;

    table->state_count = state_count;
    table->terminal_count = grammar->terminals_count;
    table->nonterminal_count = grammar->nonterminals_count;
    table->grammar = grammar;

    // Inicializar ACTION table
    table->action = malloc(sizeof(ActionEntryLR1*) * state_count);
    if (!table->action) {
        printf("=== Construyendo tabla LR(1) (!table->action)  === \n");
        free(table);
        return NULL;
    }

    for (int i = 0; i < state_count; ++i) {
        table->action[i] = malloc(sizeof(ActionEntryLR1) * grammar->terminals_count);
        if (!table->action[i]) {
            for (int j = 0; j < i; ++j) free(table->action[j]);
            free(table->action);
            free(table);
            return NULL;
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Inicializar con ERROR
        for (int j = 0; j < grammar->terminals_count; ++j) {
            table->action[i][j].action = ACTION_ERROR;
            table->action[i][j].value = -1;
        }
    }

    // Inicializar GOTO table
    table->goto_table = malloc(sizeof(int*) * state_count);
    if (!table->goto_table) {
        for (int i = 0; i < state_count; ++i) free(table->action[i]);
        free(table->action);
        free(table);
        return NULL;
    }

    for (int i = 0; i < state_count; ++i) {
        table->goto_table[i] = malloc(sizeof(int) * grammar->nonterminals_count);
        if (!table->goto_table[i]) {
            for (int j = 0; j < i; ++j) free(table->goto_table[j]);
            for (int j = 0; j < state_count; ++j) free(table->action[j]);
            free(table->action);
            free(table->goto_table);
            free(table);
            return NULL;
        }
        
        // Inicializar con -1
        for (int j = 0; j < grammar->nonterminals_count; ++j) {
            table->goto_table[i][j] = -1;
        }
    }

    // Llenar ACTION y GOTO
    for (int i = 0; i < state_count; ++i) {
        State* current = states[i];
        if (!current) continue;

        // REDUCE/ACCEPT (para estados finales)
        if (current->is_final) {
            for (int j = 0; j < current->item_count; ++j) {
                Item* item = current->items[j];
                
                if (!is_reduce_item(item)) {
                    continue;
                }

                // ACCEPT para producción augmentada
                if (item->production->number == 0) {
                    int eof_idx = index_of_symbol(grammar->terminals, grammar->terminals_count, grammar->eof);
                    if (eof_idx >= 0) {
                        table->action[i][eof_idx].action = ACTION_ACCEPT;
                        table->action[i][eof_idx].value = 0;
                    }
                } 
                // REDUCE para otras producciones
                else {
                    if (item->lookaheads->size == 0) {
                        printf("Item con producción %d no tiene lookaheads\n", item->production->number);
                    }
                    // Usar los lookaheads específicos del item LR(1)
                    for (int k = 0; k < item->lookaheads->size; ++k) {
                        Symbol* lookahead = item->lookaheads->symbols[k];
                        int symbol_idx = index_of_symbol(grammar->terminals, grammar->terminals_count, lookahead);
                        if (symbol_idx >= 0) {
                            // Verificar conflictos
                            //////////////////////////////////////////////////////////////////////////////////////////////////////
                            if (table->action[i][symbol_idx].action != ACTION_ERROR) {
                                printf("Viendo conflictos: table->action[i][symbol_idx].action != ACTION_ERROR \n");
                                const char* conflict_type = (table->action[i][symbol_idx].action == ACTION_SHIFT)? "SHIFT/REDUCE" : "REDUCE/REDUCE";
                                
                                log_conflict(
                                    conflict_type,
                                    i,
                                    lookahead->name,
                                    item->production->number,
                                    (table->action[i][symbol_idx].action == ACTION_REDUCE) 
                                        ? table->action[i][symbol_idx].value : -1
                                );
                                
                                if (strcmp(conflict_type, "SHIFT/REDUCE") == 0) {
                                    printf("Conflicto shift/reduce en estado %d con símbolo %s\n", i, lookahead->name);
    
                                    // Preferir shift para operadores
                                    if (is_operator(lookahead)) {
                                        printf("Resolviendo a favor de SHIFT para operador %s\n", lookahead->name);
                                        continue;
                                    }
                                }
                                // Preferir reduce para símbolos que terminan expresiones
                                if (strcmp(lookahead->name, ";") == 0 || strcmp(lookahead->name, "$") == 0) {
                                    printf("Resolviendo a favor de REDUCE para terminador %s\n", lookahead->name);
                                    table->action[i][symbol_idx].action = ACTION_REDUCE;
                                    table->action[i][symbol_idx].value = item->production->number;
                                }
                            } 
                            else {
                                table->action[i][symbol_idx].action = ACTION_REDUCE;
                                table->action[i][symbol_idx].value = item->production->number;
                            }
                        }
                    }
                }
            }
        }

        // SHIFT y GOTO (para transiciones)
        for (Transition* t = current->transitions; t; t = t->next) {
            int next_idx = -1;
            for (int k = 0; k < state_count; ++k) {
                if (states[k] == t->next_state) {
                    next_idx = k;
                    break;
                }
            }
            if (next_idx == -1) continue;

            if (t->symbol->type == TERMINAL) {
                int symbol_idx = index_of_symbol(grammar->terminals, grammar->terminals_count, t->symbol);
                if (symbol_idx >= 0) {
                    // Verificar conflictos
                    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////         
                    if (table->action[i][symbol_idx].action != ACTION_ERROR) {
                        const char* conflict_type = (table->action[i][symbol_idx].action == ACTION_SHIFT) 
                            ? "SHIFT/SHIFT" : "SHIFT/REDUCE";
                        
                        log_conflict(
                            conflict_type,
                            i,
                            t->symbol->name,
                            next_idx,
                            (table->action[i][symbol_idx].action == ACTION_REDUCE)? table->action[i][symbol_idx].value : -1
                        );
                        
                        // Resolver conflicto según preferencia
                        if (strcmp(conflict_type, "SHIFT/REDUCE") == 0) {
                            printf("Resolviendo conflicto shift/reduce a favor de shift\n");
                            table->action[i][symbol_idx].action = ACTION_SHIFT;
                            table->action[i][symbol_idx].value = next_idx;
                        }
                    } 
                    else {
                        table->action[i][symbol_idx].action = ACTION_SHIFT;
                        table->action[i][symbol_idx].value = next_idx;
                    }
                }
            } 
            else if (t->symbol->type == NON_TERMINAL) {
                int symbol_idx = index_of_symbol(grammar->nonterminals, grammar->nonterminals_count, t->symbol);
                if (symbol_idx >= 0) {
                    table->goto_table[i][symbol_idx] = next_idx;
                }
            }
        }
    }

    return table;
}


void print_lr1_table(LR1Table* table) 
{
    printf("\nLR(1) ACTION Table:\n");
    for (int i = 0; i < table->state_count; ++i) 
    {
        printf("State %d:\n", i);
        for (int j = 0; j < table->terminal_count; ++j) 
        {
            if (table->action[i][j].action == ACTION_SHIFT)
                printf("  shift %d with '%s'\n", table->action[i][j].value, table->grammar->terminals[j]->name);
            else if (table->action[i][j].action == ACTION_REDUCE)
                printf("  reduce %d with '%s'\n", table->action[i][j].value, table->grammar->terminals[j]->name);
            else if (table->action[i][j].action == ACTION_ACCEPT)
                printf("  accept with '%s'\n", table->grammar->terminals[j]->name);
        }
    }

    printf("\nLR(1) GOTO Table:\n");
    for (int i = 0; i < table->state_count; ++i) 
    {
        printf("State %d:\n", i);
        for (int j = 0; j < table->nonterminal_count; ++j) 
        {
            if (table->goto_table[i][j] != -1)
                printf("  goto %d with '%s'\n", table->goto_table[i][j], table->grammar->nonterminals[j]->name);
        }
    }
}

void free_lr1_table(LR1Table* table) 
{
    if (!table) return;

    for (int i = 0; i < table->state_count; ++i) 
    {
        free(table->action[i]);
        free(table->goto_table[i]);
    }
    free(table->action);
    free(table->goto_table);
    free(table);
}