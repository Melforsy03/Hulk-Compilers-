// generar_lexer.c (corregido: transiciones válidas y cierre correcto)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "regex_parser.h"
#include "utils.h"
#include "regex_to_dfa.h"
#include <ctype.h>

#define MAX_TOKENS 128

typedef struct {
    char nombre[32];
    char* regex;
    DFA dfa;
} EntradaToken;

EntradaToken tokens[MAX_TOKENS];
int num_tokens = 0;
void leer_tokens(const char* archivo) {
    FILE* f = fopen(archivo, "r");
    if (!f) {
        perror("No se pudo abrir tokens.def");
        exit(1);
    }

    char linea[256];
    int linea_num = 0;
    num_tokens = 0;

    printf("\n--- Depuración: Leyendo tokens.def ---\n");

    while (fgets(linea, sizeof(linea), f)) {
        linea_num++;
        printf("Línea %d: %s", linea_num, linea);  // Mostrar la línea original

        // Ignora líneas vacías o comentarios
        if (linea[0] == '#' || linea[0] == '\n' || linea[0] == '\r') {
            printf("Ignorado (comentario/vacío)\n");
            continue;
        }

        char nombre[32];
        int offset = 0;

        // Extrae el nombre (primera palabra)
        if (sscanf(linea, "%31s%n", nombre, &offset) != 1) {
            fprintf(stderr, "❌ Error en línea %d: No se pudo leer el nombre del token\n", linea_num);
            exit(1);
        }

        // Apunta al inicio de la regex (después del nombre)
        char* regex_inicio = linea + offset;
        while (*regex_inicio && isspace(*regex_inicio)) {
            regex_inicio++;
        }

        // Verifica si la regex está vacía
        if (*regex_inicio == '\0') {
            fprintf(stderr, "❌ Token '%s' (línea %d): Expresión regular vacía\n", nombre, linea_num);
            exit(1);
        }

        // Elimina cualquier carácter de fin de línea o espacios finales
        char* end = regex_inicio + strlen(regex_inicio) - 1;
        while (end > regex_inicio && (isspace(*end) || *end == '\r' || *end == '\n')) {
            *end = '\0';
            end--;
        }

        printf("→ Guardando: nombre='%s', regex='%s'\n", nombre, regex_inicio);

        strncpy(tokens[num_tokens].nombre, nombre, sizeof(tokens[num_tokens].nombre) - 1);
        tokens[num_tokens].regex = strdup(regex_inicio);
        num_tokens++;
    }

    fclose(f);
    printf("--- Fin de lectura ---\n\n");
}


void generar_lexer_c() {
    FILE* out = stdout;

    if (!out) { perror("No se pudo crear lexer.c"); exit(1); }

    // Cabecera del lexer
    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "#include <string.h>\n");
    fprintf(out, "#include <stdlib.h>\n");
    fprintf(out, "#include <ctype.h>\n");
    fprintf(out, "#include \"lexer.h\"\n\n");

    // --- Inserta implementación local de my_strndup ---
    fprintf(out, "static char* my_strndup(const char* src, size_t n) {\n");
    fprintf(out, "    char* s = (char*)malloc(n + 1);\n");
    fprintf(out, "    if (!s) return NULL;\n");
    fprintf(out, "    strncpy(s, src, n);\n");
    fprintf(out, "    s[n] = '\\0';\n");
    fprintf(out, "    return s;\n");
    fprintf(out, "}\n\n");

    // Función para coincidencia con DFA
    fprintf(out, "static int match_dfa(EstadoDFA* estados, int num_estados, const char* input, int* length) {\n");
    fprintf(out, "    int estado_actual = 0;\n");
    fprintf(out, "    int last_final = -1;\n");
    fprintf(out, "    int match_length = 0;\n");
    fprintf(out, "    \n");
    fprintf(out, "    while (input[match_length]) {\n");
    fprintf(out, "        char c = input[match_length];\n");
    fprintf(out, "        int encontrado = 0;\n");
    fprintf(out, "        \n");
    fprintf(out, "        for (int i = 0; i < estados[estado_actual].num_transiciones; i++) {\n");
    fprintf(out, "            if (estados[estado_actual].transiciones[i].simbolo == c) {\n");
    fprintf(out, "                estado_actual = estados[estado_actual].transiciones[i].destino;\n");
    fprintf(out, "                encontrado = 1;\n");
    fprintf(out, "                break;\n");
    fprintf(out, "            }\n");
    fprintf(out, "        }\n");
    fprintf(out, "        \n");
    fprintf(out, "        if (!encontrado) break;\n");
    fprintf(out, "        match_length++;\n");
    fprintf(out, "        if (estados[estado_actual].es_final) {\n");
    fprintf(out, "            last_final = match_length;\n");
    fprintf(out, "        }\n");
    fprintf(out, "    }\n");
    fprintf(out, "    if (last_final > 0) {\n");
    fprintf(out, "        *length = last_final;\n");
    fprintf(out, "        return estados[estado_actual].tipo;\n");
    fprintf(out, "    }\n");
    fprintf(out, "    return TOKEN_ERROR;\n");
    fprintf(out, "}\n\n");

    // Generar DFAs para cada token
    for (int t = 0; t < num_tokens; t++) {
        FragmentoNFA nfa = parse_regex(tokens[t].regex);
        nfa.fin->es_final = 1;
        tokens[t].dfa = convertir_nfa_a_dfa(nfa.inicio, t);

        fprintf(out, "static EstadoDFA dfa_%s[] = {\n", tokens[t].nombre);
        for (int i = 0; i < tokens[t].dfa.cantidad_estados; i++) {
            EstadoDFA* est = &tokens[t].dfa.estados[i];
            fprintf(out, "    { %d, %d, {", est->id, est->num_transiciones);
            
            for (int j = 0; j < est->num_transiciones; j++) {
                char s = est->transiciones[j].simbolo;
                if (s == '\\') fprintf(out, "{%d, '\\\\'}", est->transiciones[j].destino);
                else if (s == '\'') fprintf(out, "{%d, '\\''}", est->transiciones[j].destino);
                else if (s == '\"') fprintf(out, "{%d, '\"'}", est->transiciones[j].destino);
                else if (s == '\n') fprintf(out, "{%d, '\\n'}", est->transiciones[j].destino);
                else if (s == '\t') fprintf(out, "{%d, '\\t'}", est->transiciones[j].destino);
                else if (s == '\r') fprintf(out, "{%d, '\\r'}", est->transiciones[j].destino);
                else fprintf(out, "{%d, '%c'}", est->transiciones[j].destino, s);
                
                if (j < est->num_transiciones - 1) fprintf(out, ", ");
            }
            
            fprintf(out, "}, %d, TOKEN_%s },\n", est->es_final, tokens[t].nombre);
        }
        fprintf(out, "};\n\n");
    }

    // Función principal del lexer
    fprintf(out, "Token next_token(const char** input) {\n");
    fprintf(out, "    // Saltar espacios en blanco\n");
    fprintf(out, "    while (**input && isspace(**input)) {\n");
    fprintf(out, "        (*input)++;\n");
    fprintf(out, "    }\n");
    fprintf(out, "    \n");
    fprintf(out, "    if (**input == '\\0') {\n");
    fprintf(out, "        return (Token){TOKEN_EOF, NULL, 0};\n");
    fprintf(out, "    }\n");
    fprintf(out, "    \n");
    fprintf(out, "    int max_len = 0;\n");
    fprintf(out, "    TokenType tipo = TOKEN_ERROR;\n");
    fprintf(out, "    \n");
    
    // Comparar con todos los DFAs
    for (int i = 0; i < num_tokens; i++) {
        fprintf(out, "    {\n");
        fprintf(out, "        int len = 0;\n");
        fprintf(out, "        TokenType t = match_dfa(dfa_%s, %d, *input, &len);\n", 
                tokens[i].nombre, tokens[i].dfa.cantidad_estados);
        fprintf(out, "        if (t != TOKEN_ERROR && len > max_len) {\n");
        fprintf(out, "            max_len = len;\n");
        fprintf(out, "            tipo = t;\n");
        fprintf(out, "        }\n");
        fprintf(out, "    }\n");
    }
    
    fprintf(out, "    \n");
    fprintf(out, "    if (max_len > 0) {\n");
    fprintf(out, "        char* lexema = my_strndup(*input, max_len);\n");
    fprintf(out, "        *input += max_len;\n");
    fprintf(out, "        return (Token){tipo, lexema, max_len};\n");
    fprintf(out, "    }\n");
    fprintf(out, "    \n");
    fprintf(out, "    // Manejo de errores: avanzar un carácter\n");
    fprintf(out, "    char* error_lexema = my_strndup(*input, 1);\n");
    fprintf(out, "    (*input)++;\n");
    fprintf(out, "    return (Token){TOKEN_ERROR, error_lexema, 1};\n");
    fprintf(out, "}\n\n");
    
    // Función para imprimir tokens (opcional)
    fprintf(out, "void print_token(Token t) {\n");
    fprintf(out, "    static const char* token_names[] = {\n");
    for (int i = 0; i < num_tokens; i++) {
        fprintf(out, "        \"%s\",\n", tokens[i].nombre);
    }
    fprintf(out, "        \"EOF\",\n");
    fprintf(out, "        \"ERROR\"\n");
    fprintf(out, "    };\n");
    fprintf(out, "    \n");
    fprintf(out, "    if (t.type == TOKEN_EOF) {\n");
    fprintf(out, "        printf(\"<EOF>\\n\");\n");
    fprintf(out, "    } else if (t.type == TOKEN_ERROR) {\n");
    fprintf(out, "        printf(\"<ERROR, '%%.*s'>\\n\", t.length, t.lexema);\n");
    fprintf(out, "    } else {\n");
    fprintf(out, "        printf(\"<%%s, '%%.*s'>\\n\", token_names[t.type], t.length, t.lexema);\n");
    fprintf(out, "    }\n");
    fprintf(out, "    \n");
    fprintf(out, "    if (t.lexema) free(t.lexema);\n");
    fprintf(out, "}\n");

    fclose(out);
}

int main() {
    leer_tokens("lexer_gen/tokens.def");
    generar_lexer_c();
    printf("✅ lexer.c generado correctamente con regex_to_dfa.\n");
    return 0;
}



