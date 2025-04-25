// generar_lexer.c (corregido: transiciones válidas y cierre correcto)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "regex_parser.h"
#include "nfa_to_dfa.h"
#include "utils.h"

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
        perror("tokens.def no encontrado");
        exit(1);
    }

    char linea[256];
    while (fgets(linea, sizeof(linea), f)) {
        char* nombre = strtok(linea, " \t\r\n");
        char* regex = strtok(NULL, "\r\n");

        // ✅ CORRECCIÓN: eliminar espacios iniciales del regex
        while (regex && *regex == ' ') regex++;

        if (nombre && regex) {
            strncpy(tokens[num_tokens].nombre, nombre, 31);
            tokens[num_tokens].regex = strdup(regex);
            num_tokens++;
        }
    }
    fclose(f);
}


void generar_lexer_c() {
    FILE* out = fopen("lexer.c", "w");
    if (!out) { perror("No se pudo crear lexer.c"); exit(1); }

    fprintf(out, "#include <stdio.h>\n#include <string.h>\n\n");
    fprintf(out, "typedef enum {\n");
    for (int i = 0; i < num_tokens; i++)
        fprintf(out, "    TOKEN_%s,\n", tokens[i].nombre);
    fprintf(out, "    TOKEN_EOF,\n    TOKEN_ERROR\n} TokenType;\n\n");

    fprintf(out, "typedef struct {\n    TokenType type;\n    const char* lexema;\n    int length;\n} Token;\n\n");

    fprintf(out, "typedef struct { int destino; char simbolo; } Transicion;\n\n");
    fprintf(out, "typedef struct {\n    int id; int num_transiciones; Transicion transiciones[128];\n    int final; TokenType tipo;\n} EstadoDFA;\n\n");

    fprintf(out, "int match_dfa(EstadoDFA* estados, int cantidad, const char* input) {\n");
    fprintf(out, "    int estado = 0, len = 0, ultimo_final = -1;\n");
    fprintf(out, "    for (int i = 0; input[i]; i++) {\n");
    fprintf(out, "        int trans_found = 0;\n        for (int j = 0; j < estados[estado].num_transiciones; j++) {\n");
    fprintf(out, "            if (estados[estado].transiciones[j].simbolo == input[i]) {\n");
    fprintf(out, "                estado = estados[estado].transiciones[j].destino;\n                trans_found = 1; break; } }\n");
    fprintf(out, "        if (!trans_found) break;\n        len++;\n        if (estados[estado].final) ultimo_final = len;\n    }\n    return ultimo_final;\n}\n\n");

    for (int t = 0; t < num_tokens; t++) {
        FragmentoNFA nfa = parse_regex(tokens[t].regex);
        nfa.fin->es_final = 1;
        tokens[t].dfa = convertir_nfa_a_dfa(nfa.inicio, t);

        fprintf(out, "EstadoDFA dfa_%s[] = {\n", tokens[t].nombre);
        for (int i = 0; i < tokens[t].dfa.cantidad_estados; i++) {
            EstadoDFA* est = &tokens[t].dfa.estados[i];
            fprintf(out, "  { %d, %d, {", est->id, est->num_transiciones);
            for (int j = 0; j < est->num_transiciones; j++) {
                char s = est->transiciones[j].simbolo;
                if (s == '\\') fprintf(out, "{%d, '\\\\\\\\'}%s", est->transiciones[j].destino, (j < est->num_transiciones - 1) ? ", " : "");
                else if (s == '\'') fprintf(out, "{%d, '\\\\''}%s", est->transiciones[j].destino, (j < est->num_transiciones - 1) ? ", " : "");
                else if (s == '\"') fprintf(out, "{%d, '\\\"'}%s", est->transiciones[j].destino, (j < est->num_transiciones - 1) ? ", " : "");
                else if (s == '\n') fprintf(out, "{%d, '\\\\n'}%s", est->transiciones[j].destino, (j < est->num_transiciones - 1) ? ", " : "");
                else fprintf(out, "{%d, '%c'}%s", est->transiciones[j].destino, s, (j < est->num_transiciones - 1) ? ", " : "");

            }
            fprintf(out, "}, %d, TOKEN_%s },\n", est->es_final, tokens[t].nombre);
        }
        fprintf(out, "};\n\n");
    }

    fprintf(out, "Token next_token(const char* input) {\n");
    fprintf(out, "    int max_len = 0; TokenType tipo = TOKEN_ERROR;\n");
    for (int i = 0; i < num_tokens; i++) {
        fprintf(out, "    int len%d = match_dfa(dfa_%s, %d, input);\n", i, tokens[i].nombre, tokens[i].dfa.cantidad_estados);
        fprintf(out, "    if (len%d > max_len) { max_len = len%d; tipo = TOKEN_%s; }\n", i, i, tokens[i].nombre);
    }
    fprintf(out, "    if (max_len > 0) return (Token){tipo, input, max_len};\n");
    fprintf(out, "    return (Token){TOKEN_ERROR, input, 1};\n}\n\n");

    fprintf(out, "void print_token(Token t) {\n    const char* nombres[] = {\n");
    for (int i = 0; i < num_tokens; i++)
        fprintf(out, "        \"%s\",\n", tokens[i].nombre);
    fprintf(out, "        \"EOF\",\n        \"ERROR\" };\n");
    fprintf(out, "    printf(\"<%%s, %%.*s>\\n\", nombres[t.type], t.length, t.lexema);\n");
    fprintf(out, "}\n");        
    

    fclose(out);
}


