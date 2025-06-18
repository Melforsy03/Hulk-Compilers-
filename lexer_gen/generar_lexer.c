// generar_lexer.c actualizado con línea + columna
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
    if (!f) { perror("No se pudo abrir tokens.def"); exit(1); }
    
    char linea[256];
    num_tokens = 0;

    while (fgets(linea, sizeof(linea), f)) {
        if (linea[0] == '#' || linea[0] == '\n' || linea[0] == '\r') continue;
        char nombre[32];
        int offset = 0;
        if (sscanf(linea, "%31s%n", nombre, &offset) != 1) exit(1);
        char* regex_inicio = linea + offset;
        while (*regex_inicio && isspace(*regex_inicio)) regex_inicio++;
        if (*regex_inicio == '\0') exit(1);

        char* end = regex_inicio + strlen(regex_inicio) - 1;
        while (end > regex_inicio && (isspace(*end) || *end == '\r' || *end == '\n')) {
            *end = '\0'; end--;
        }

        strncpy(tokens[num_tokens].nombre, nombre, sizeof(tokens[num_tokens].nombre)-1);
        tokens[num_tokens].regex = strdup(regex_inicio);
        num_tokens++;
    }
    fclose(f);
}

void generar_lexer_c() {
    FILE* out = stdout;
    if (!out) { perror("No se pudo crear lexer.c"); exit(1); }

    // Cabecera
    fprintf(out, "#include <stdio.h>\n#include <string.h>\n#include <stdlib.h>\n#include <ctype.h>\n#include \"lexer.h\"\n\n");

    // my_strndup
    fprintf(out, "static char* my_strndup(const char* src, size_t n) { char* s = (char*)malloc(n+1); if (!s) return NULL; strncpy(s, src, n); s[n] = '\\0'; return s; }\n\n");

    // match_dfa
    fprintf(out, "static int match_dfa(EstadoDFA* estados, int num_estados, const char* input, int* length) {\n");
    fprintf(out, "    int estado_actual = 0, last_final = -1, match_length = 0;\n");
    fprintf(out, "    while (input[match_length]) {\n");
    fprintf(out, "        char c = input[match_length]; int encontrado = 0;\n");
    fprintf(out, "        for (int i = 0; i < estados[estado_actual].num_transiciones; i++) {\n");
    fprintf(out, "            if (estados[estado_actual].transiciones[i].simbolo == c) {\n");
    fprintf(out, "                estado_actual = estados[estado_actual].transiciones[i].destino; encontrado = 1; break;\n");
    fprintf(out, "            }\n        }\n");
    fprintf(out, "        if (!encontrado) break;\n");
    fprintf(out, "        match_length++;\n");
    fprintf(out, "        if (estados[estado_actual].es_final) last_final = match_length;\n");
    fprintf(out, "    }\n");
    fprintf(out, "    if (last_final > 0) { *length = last_final; return estados[estado_actual].tipo; }\n");
    fprintf(out, "    return TOKEN_ERROR; }\n\n");

    // DFAs
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
                else if (s == '\'') fprintf(out, "{%d, '\\'\''}", est->transiciones[j].destino);
                else if (s == '\"') fprintf(out, "{%d, '\\\"'}", est->transiciones[j].destino);
                else if (s == '\n') fprintf(out, "{%d, '\\n'}", est->transiciones[j].destino);
                else if (s == '\t') fprintf(out, "{%d, '\\t'}", est->transiciones[j].destino);
                else fprintf(out, "{%d, '%c'}", est->transiciones[j].destino, s);
                if (j < est->num_transiciones - 1) fprintf(out, ", ");
            }
            fprintf(out, "}, %d, TOKEN_%s },\n", est->es_final, tokens[t].nombre);
        }
        fprintf(out, "};\n\n");
    }

    // next_token
    fprintf(out, "static int current_line = 1, current_column = 1;\n");
    fprintf(out, "Token next_token(const char** input) {\n");
    fprintf(out, "    while (**input && isspace(**input)) {\n");
    fprintf(out, "        if (**input == '\\n') { current_line++; current_column = 1; } else { current_column++; }\n");
    fprintf(out, "        (*input)++;\n    }\n");
    fprintf(out, "    if (**input == '\\0') { return (Token){TOKEN_EOF, NULL, 0, current_line, current_column}; }\n");
    fprintf(out, "    int max_len = 0; TokenType tipo = TOKEN_ERROR;\n");
    for (int i = 0; i < num_tokens; i++) {
        fprintf(out, "    { int len = 0; TokenType t = match_dfa(dfa_%s, %d, *input, &len);\n", tokens[i].nombre, tokens[i].dfa.cantidad_estados);
        fprintf(out, "      if (t != TOKEN_ERROR && len > max_len) { max_len = len; tipo = t; } }\n");
    }
    fprintf(out, "    if (max_len > 0) {\n");
    fprintf(out, "        char* lexema = my_strndup(*input, max_len);\n");
    fprintf(out, "        int token_line = current_line, token_column = current_column;\n");
    fprintf(out, "        for (int i = 0; i < max_len; i++) {\n");
    fprintf(out, "            if ((*input)[i] == '\\n') { current_line++; current_column = 1; } else { current_column++; }\n        }\n");
    fprintf(out, "        *input += max_len;\n");
    fprintf(out, "        return (Token){tipo, lexema, max_len, token_line, token_column};\n    }\n");
    fprintf(out, "    char* error_lexema = my_strndup(*input, 1);\n");
    fprintf(out, "    int error_line = current_line, error_column = current_column;\n");
    fprintf(out, "    if (**input == '\\n') { current_line++; current_column = 1; } else { current_column++; }\n");
    fprintf(out, "    (*input)++;\n");
    fprintf(out, "    return (Token){TOKEN_ERROR, error_lexema, 1, error_line, error_column};\n}\n\n");

    // print_token
    fprintf(out, "void print_token(Token t) {\n");
    fprintf(out, "    static const char* token_names[] = {\n");
    for (int i = 0; i < num_tokens; i++) {
        fprintf(out, "        \"%s\",\n", tokens[i].nombre);
    }
    fprintf(out, "        \"EOF\",\n");
    fprintf(out, "        \"ERROR\"\n");
    fprintf(out, "    };\n\n");

    fprintf(out, "    if (t.type == TOKEN_EOF) {\n");
    fprintf(out, "        printf(\"<EOF>\\n\");\n");
    fprintf(out, "    } else if (t.type == TOKEN_ERROR) {\n");
    fprintf(out, "        printf(\"<ERROR, '%%.*s', línea %%d, columna %%d>\\n\", t.length, t.lexema, t.line, t.column);\n");
    fprintf(out, "    } else {\n");
    fprintf(out, "        printf(\"<%%s, '%%.*s', línea %%d, columna %%d>\\n\", token_names[t.type], t.length, t.lexema, t.line, t.column);\n");
    fprintf(out, "    }\n");

    fprintf(out, "    if (t.lexema) free(t.lexema);\n");
    fprintf(out, "}\n");

}

int main() {
    leer_tokens("lexer_gen/tokens.def");
    generar_lexer_c();
  
    return 0;
}
