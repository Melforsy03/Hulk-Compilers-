#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "grammar.h"
#include "symbol.h"
#include "production.h"

#define MAX_LINE 1024
#define MAX_TOKENS 100

typedef enum {
    NONE,
    NOTERMINALS,
    TERMINALS,
    START,
    PRODUCTIONS
} Section;

void trim(char* str) {
    // Elimina espacios iniciales
    while (isspace((unsigned char)*str)) str++;

    // Elimina espacios finales
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = 0;
}

void load_grammar_from_file(Grammar* grammar, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("No se pudo abrir el archivo");
        exit(1);
    }

    char line[MAX_LINE];
    Section section = NONE;

    while (fgets(line, sizeof(line), file)) {
        trim(line);

        if (strlen(line) == 0) continue;  

        if (strncasecmp(line, "NoTerminals:", 13) == 0) {
            section = NOTERMINALS;
            continue;
        }
        else if (strncasecmp(line, "Terminals:", 10) == 0) {
            section = TERMINALS;
            continue;
        }
        else if (strncasecmp(line, "StartSymbol:", 12) == 0) {
            section = START;
            char* name = line + 12;
            trim(name);

            Symbol* s = find_symbol(grammar, name);
            if (!s) {
                printf("Error: símbolo de inicio '%s' no encontrado.\n", name);
                exit(1);
            }
            grammar->start_symbol = s;
            continue;
        }
        else if (strncasecmp(line, "Productions:", 12) == 0) {
            section = PRODUCTIONS;
            continue;
        }

        if (section == NOTERMINALS || section == TERMINALS) {
            char* token = strtok(line, " ");
            while (token) {
                trim(token);
                SymbolType type = (section == NOTERMINALS) ? NON_TERMINAL : TERMINAL;
                add_symbol(grammar, token, type);
                token = strtok(NULL, " ");
            }
        } 
        else if (section == PRODUCTIONS) {
            char* arrow = strstr(line, "->");
            if (!arrow) {
                printf("Error: producción sin '->': %s\n", line);
                exit(1);
            }

            *arrow = '\0';          // ← Corta la línea justo donde inicia '->'
            char* lhs = line;
            char* rhs = arrow + 2;  // ← Salta los dos caracteres '->'

            // Limpia ambos lados
            trim(lhs);
            trim(rhs);

            // Depuración opcional
            printf("LHS = '%s'\n", lhs);
            printf("RHS = '%s'\n\n", rhs);

            Symbol* left = find_symbol(grammar, lhs);
            if (!left) {
                printf("Error: símbolo izquierdo no encontrado: %s\n", lhs);
                exit(1);
            }

            Symbol* rhs_symbols[MAX_TOKENS];
            int count = 0;

            char* token = strtok(rhs, " ");
            while (token && count < MAX_TOKENS) {
                trim(token);
                if (strcmp(token, "epsilon") == 0) {
                    count = 0; // producción vacía
                    break;
                }
                Symbol* sym = find_symbol(grammar, token);
                
                if (!sym) {
                    printf("Error: símbolo en RHS no encontrado: %s\n", token);
                    exit(1);
                }
                rhs_symbols[count++] = sym;
                token = strtok(NULL, " ");
            }

            add_production(grammar, left, rhs_symbols, count);
        }
    }

    fclose(file);
}