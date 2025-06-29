#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "grammar.h"
#include "symbol.h"
#include "production.h"
#include "load_grammar.h"

#define MAX_LINE_LENGTH 256
#define MAX_TOKENS 32

Grammar* load_grammar_from_file(const char* filename) {
    Grammar* g = create_grammar();
    if (!g) {
        fprintf(stderr, "Error: No se pudo crear la gramática\n");
        return NULL;
    }

    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: No se pudo abrir el archivo %s\n", filename);
        free_grammar(g);
        return NULL;
    }

    char line[MAX_LINE_LENGTH];
    int section = 0; // 0 = nada, 1 = terminales, 2 = no terminales, 3 = producciones
    Symbol* start_symbol = NULL;

    while (fgets(line, sizeof(line), file)) {
        // Eliminar comentarios (líneas que empiezan con #)
        if (line[0] == '#' || line[0] == '\n') continue;

        // Detectar secciones
        if (strstr(line, "TERMINALS:")) {
            section = 1;
            continue;
        } else if (strstr(line, "NON_TERMINALS:")) {
            section = 2;
            continue;
        } else if (strstr(line, "PRODUCTIONS:")) {
            section = 3;
            continue;
        } else if (strstr(line, "START_SYMBOL:")) {
            char* start_sym = strchr(line, ':');
            if (start_sym) {
                start_sym += 1;
                while (isspace(*start_sym)) start_sym++;
                start_sym[strcspn(start_sym, " \t\n")] = '\0';
                start_symbol = find_symbol(g, start_sym);
                if (!start_symbol) {
                    fprintf(stderr, "Error: Símbolo inicial '%s' no encontrado\n", start_sym);
                    fclose(file);
                    free_grammar(g);
                    return NULL;
                }
                g->start_symbol = start_symbol;
            }
            continue;
        }

        // Procesar según la sección actual
        switch (section) {
            case 1: { // Terminales
                char* token = strtok(line, " \t\n");
                while (token) {
                    if (strcmp(token, "epsilon") == 0) {
                        token = strtok(NULL, " \t\n");
                        continue;
                    }
                    add_symbol(g, token, TERMINAL);
                    token = strtok(NULL, " \t\n");
                }
                break;
            }
            case 2: { // No terminales
                char* token = strtok(line, " \t\n");
                while (token) {
                    add_symbol(g, token, NON_TERMINAL);
                    token = strtok(NULL, " \t\n");
                }
                break;
            }
            case 3: { // Producciones
                char* arrow = strstr(line, "->");
                if (!arrow) continue;

                *arrow = '\0';
                char* left = line;
                while (isspace(*left)) left++;
                left[strcspn(left, " \t")] = '\0';

                Symbol* left_sym = find_symbol(g, left);
                if (!left_sym) {
                    fprintf(stderr, "Error: No terminal '%s' no definido\n", left);
                    fclose(file);
                    free_grammar(g);
                    return NULL;
                }

                char* right_part = arrow + 2;
                char* production = strtok(right_part, "|");
                while (production) {
                    Symbol* right_symbols[MAX_TOKENS];
                    int right_len = 0;

                    char* token = strtok(production, " \t\n");
                    while (token && right_len < MAX_TOKENS) {
                        if (strcmp(token, "epsilon") == 0) {
                            right_symbols[right_len++] = g->epsilon;
                        } else {
                            Symbol* sym = find_symbol(g, token);
                            if (!sym) {
                                fprintf(stderr, "Error: Símbolo '%s' no definido\n", token);
                                fclose(file);
                                free_grammar(g);
                                return NULL;
                            }
                            right_symbols[right_len++] = sym;
                        }
                        token = strtok(NULL, " \t\n");
                    }

                    if (right_len > 0) {
                        add_production(g, left_sym, right_symbols, right_len);
                    }
                    production = strtok(NULL, "|");
                }
                break;
            }
        }
    }

    fclose(file);

    // Validación final
    if (!g->start_symbol) {
        fprintf(stderr, "Error: Símbolo inicial no definido\n");
        free_grammar(g);
        return NULL;
    }

    return g;
}
