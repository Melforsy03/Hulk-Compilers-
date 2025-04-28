#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "grammar.h"   
#include "symbol.h"
#include "production.h"

#define MAX_LINE 1024
#define MAX_TOKENS 100

typedef enum { NONE, NONTERMINALS, TERMINALS, START, PRODUCTIONS } Section;

// -----------------------------
// FUNCION PRINCIPAL PARA CARGAR
// -----------------------------
void load_grammar_from_file(Grammar* grammar, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error al abrir el archivo: %s\n", filename);
        exit(1);
    }

    char line[MAX_LINE];
    Section current_section = NONE;

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "NonTerminals:", 13) == 0) {
            current_section = NONTERMINALS;
            continue;
        } else if (strncmp(line, "Terminals:", 10) == 0) {
            current_section = TERMINALS;
            continue;
        } else if (strncmp(line, "StartSymbol:", 12) == 0) {
            current_section = START;
            char* start = line + 13;
            start[strcspn(start, "\n")] = 0; // Quitar el salto de línea
            Symbol* start_sym = add_symbol(grammar, start, NON_TERMINAL);
            grammar->start_symbol = start_sym;
            continue;
        } else if (strncmp(line, "Productions:", 12) == 0) {
            current_section = PRODUCCIONES;
            continue;
        }

        if (current_section == NONTERMINALS || current_section == TERMINALS) {
            char* token = strtok(line, ", \n");
            while (token) {
                SymbolType type = (current_section == NONTERMINALS) ? NON_TERMINAL : TERMINAL;
                add_symbol(grammar, token, type);
                token = strtok(NULL, ", \n");
            }
        } else if (current_section == PRODUCCIONES) {
            char* lhs = strtok(line, "->\n");
            char* rhs = strtok(NULL, "\n");

            if (!lhs || !rhs) continue;

            while (*lhs == ' ') lhs++; // Quitar espacios iniciales

            Symbol* left_sym = find_symbol(grammar, lhs);
            if (!left_sym) {
                printf("Error: símbolo no encontrado: %s\n", lhs);
                exit(1);
            }

            Symbol* rhs_symbols[MAX_TOKENS];
            int rhs_len = 0;

            char* token = strtok(rhs, " \t\n");
            while (token && rhs_len < MAX_TOKENS) {
                if (strcmp(token, "ε") == 0) {
                    // Si aparece epsilon explícitamente, no agregamos nada a la derecha
                    rhs_len = 0;
                    break;
                }

                Symbol* sym = find_symbol(grammar, token);
                if (!sym) {
                    printf("Error: símbolo no encontrado: %s\n", token);
                    exit(1);
                }

                rhs_symbols[rhs_len++] = sym;
                token = strtok(NULL, " \t\n");
            }

            add_production(grammar, left_sym, rhs_symbols, rhs_len);
        }
    }

    fclose(file);
}

// -----------------------------
// MAIN PRINCIPAL
// -----------------------------
int main() {
    Grammar* grammar = create_grammar();

    load_grammar_from_file(grammar, "producciones.txt");

    print_grammar(grammar);

    return 0;
}
