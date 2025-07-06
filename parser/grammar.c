#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "grammar.h"

void init_grammar(Grammar* g) {
    g->num_terminals = 0;
    g->num_non_terminals = 0;
    g->num_productions = 0;
    strcpy(g->start_symbol, "Program"); // Establece el símbolo inicial
}

int is_symbol_present(const Grammar* g, const char* symbol) {
    for (int i = 0; i < g->num_terminals; i++) {
        if (strcmp(symbol, g->terminals[i]) == 0) return 1;
    }
    for (int i = 0; i < g->num_non_terminals; i++) {
        if (strcmp(symbol, g->non_terminals[i]) == 0) return 1;
    }
    return 0;
}

void add_terminal(Grammar* g, const char* symbol) {
    if (g->num_terminals >= MAX_TERMINALS) {
        fprintf(stderr, "Error: Demasiados terminales\n");
        exit(EXIT_FAILURE);
    }
    if (!is_symbol_present(g, symbol)) {
        strncpy(g->terminals[g->num_terminals], symbol, MAX_SYMBOL_LEN-1);
        g->terminals[g->num_terminals][MAX_SYMBOL_LEN-1] = '\0'; // Asegurar null-termination
        g->num_terminals++;
    }
}

void add_non_terminal(Grammar* g, const char* symbol) {
    if (g->num_non_terminals >= MAX_NON_TERMINALS) {
        fprintf(stderr, "Error: Demasiados no terminales\n");
        exit(EXIT_FAILURE);
    }
    if (!is_symbol_present(g, symbol)) {
        strncpy(g->non_terminals[g->num_non_terminals], symbol, MAX_SYMBOL_LEN-1);
        g->non_terminals[g->num_non_terminals][MAX_SYMBOL_LEN-1] = '\0'; // Asegurar null-termination
        g->num_non_terminals++;
    }
}

void add_production(Grammar* g, const char* lhs, const char** rhs, int rhs_len) {
    if (g->num_productions >= MAX_PRODUCTIONS) {
        fprintf(stderr, "Error: Demasiadas producciones\n");
        exit(EXIT_FAILURE);
    }

    Production* p = &g->productions[g->num_productions];
    strncpy(p->lhs, lhs, MAX_SYMBOL_LEN-1);
    p->lhs[MAX_SYMBOL_LEN-1] = '\0'; // Asegurar null-termination

    p->rhs_len = rhs_len;
    for (int i = 0; i < rhs_len; ++i) {
        strncpy(p->rhs[i], rhs[i], MAX_SYMBOL_LEN-1);
        p->rhs[i][MAX_SYMBOL_LEN-1] = '\0'; // Asegurar null-termination
    }
    g->num_productions++;
}

void load_grammar_from_file( Grammar* g, const char* filename) {
    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        perror("Error al abrir el archivo de gramática");
        exit(EXIT_FAILURE);
    }

    char line[512];
    char* symbol;
    char* saveptr1; // Para strtok_r externo
    char* saveptr2; // Para strtok_r interno

    char* symbols[MAX_RHS_SYMBOLS]; // Array temporal para símbolos RHS
    char symbol_storage[MAX_RHS_SYMBOLS][MAX_SYMBOL_LEN]; // Almacenamiento real de las cadenas

    while (fgets(line, sizeof(line), f) != NULL) {
        // Eliminar salto de línea si existe
        line[strcspn(line, "\n")] = 0;

        // Saltar líneas vacías o comentarios
        if (strlen(line) == 0 || line[0] == '#') continue;

        char* line_copy = strdup(line); // Crear una copia modificable de la línea

        // Parsear LHS (lado izquierdo)
        char* lhs = strtok_r(line_copy, " ", &saveptr1);
        if (lhs == NULL) {
            free(line_copy);
            continue;
        }

        add_non_terminal(g, lhs); // Asegurarse de que el LHS es un no terminal

        // Saltar "::="
        char* op = strtok_r(NULL, " ", &saveptr1);
        if (op == NULL || strcmp(op, "::=") != 0) {
            fprintf(stderr, "Error de formato de gramática: se esperaba '::=' en la línea '%s'\n", line);
            free(line_copy);
            continue;
        }

        char* alt = strtok_r(NULL, "|", &saveptr1);
        while (alt != NULL) {
            char* trimmed_alt = alt;
            // Eliminar espacios al principio y al final
            while (isspace((unsigned char)*trimmed_alt)) trimmed_alt++;
            char* end = trimmed_alt + strlen(trimmed_alt) - 1;
            while (end > trimmed_alt && isspace((unsigned char)*end)) end--;
            *(end + 1) = '\0';

            if (strlen(trimmed_alt) == 0) { // Si la alternativa está vacía (ej. "A ::= | B")
                alt = strtok_r(NULL, "|", &saveptr1);
                continue;
            }

            int rhs_len = 0;
            char* current_alt_copy = strdup(trimmed_alt); // Copia para strtok_r anidado
            symbol = strtok_r(current_alt_copy, " ", &saveptr2);

            while (symbol != NULL) {
                // Copiar el símbolo a un buffer temporal para manipulación
                char symbol_buffer[MAX_SYMBOL_LEN];
                strncpy(symbol_buffer, symbol, MAX_SYMBOL_LEN - 1);
                symbol_buffer[MAX_SYMBOL_LEN - 1] = '\0'; // Asegurar null-termination

                if (symbol_buffer[0] == '\'') { // Es un terminal (entre comillas simples)
                    // Eliminar comillas para almacenar
                    char* unquoted_symbol = symbol_buffer + 1; // Apuntar después de la primera comilla
                    unquoted_symbol[strlen(unquoted_symbol) - 1] = '\0'; // Cortar la última comilla

                    strcpy(symbol_storage[rhs_len], unquoted_symbol);
                    add_terminal(g, symbol_storage[rhs_len]);
                }
                // --- INICIO DE LA CORRECCIÓN PARA ÉPSILON ---
                else if (strcmp(symbol_buffer, "ε") == 0 || strcmp(symbol_buffer, "╬╡") == 0) {
                    // Es el símbolo épsilon, tratarlo como terminal especial.
                    // Estandarizar a "ε" internamente para una consistencia.
                    strcpy(symbol_storage[rhs_len], "ε");
                    add_terminal(g, "ε"); // Asegúrate de que "ε" se añada como terminal.
                }
                // --- FIN DE LA CORRECCIÓN PARA ÉPSILON ---
                else { // Si no es un terminal entre comillas ni épsilon, asumimos que es un no terminal
                    strcpy(symbol_storage[rhs_len], symbol_buffer);
                    add_non_terminal(g, symbol_storage[rhs_len]);
                }

                symbols[rhs_len] = symbol_storage[rhs_len];
                rhs_len++;

                symbol = strtok_r(NULL, " ", &saveptr2);
            }

            if (rhs_len > 0) {
                add_production(g, lhs, (const char**)symbols, rhs_len);
            }
            free(current_alt_copy); // Liberar la copia de la alternativa

            alt = strtok_r(NULL, "|", &saveptr1);
        }
        free(line_copy); // Liberar la copia de la línea original
    }

    fclose(f);
}

void print_grammar(const Grammar* g) {
    printf("Start Symbol: %s\n", g->start_symbol);
    
    printf("\nTerminals (%d):\n", g->num_terminals);
    for (int i = 0; i < g->num_terminals; ++i) {
        printf("'%s' ", g->terminals[i]);
        if ((i+1) % 10 == 0) printf("\n");
    }
    
    printf("\n\nNon-terminals (%d):\n", g->num_non_terminals);
    for (int i = 0; i < g->num_non_terminals; ++i) {
        printf("%s ", g->non_terminals[i]);
        if ((i+1) % 10 == 0) printf("\n");
    }
    
    printf("\n\nProductions (%d):\n", g->num_productions);
    for (int i = 0; i < g->num_productions; ++i) {
        printf("%s ::= ", g->productions[i].lhs);
        for (int j = 0; j < g->productions[i].rhs_len; ++j) {
            printf("%s ", g->productions[i].rhs[j]);
        }
        printf("\n");
    }
}