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

// Función para manejar caracteres especiales
void escribir_caracter_especial(FILE* out, char c) {
    switch (c) {
        case '\'': fprintf(out, "'\\''"); break;
        case '\\': fprintf(out, "'\\\\'"); break;
        case '\"': fprintf(out, "'\\\"'"); break;
        case '\n': fprintf(out, "'\\n'"); break;
        case '\t': fprintf(out, "'\\t'"); break;
        case '\r': fprintf(out, "'\\r'"); break;
        default:
            if (isprint((unsigned char)c)) {
                fprintf(out, "'%c'", c);
            } else {
                fprintf(out, "%d", (unsigned char)c);
            }
            break;
    }
}

void leer_tokens(const char* archivo) {
    FILE* f = fopen(archivo, "r");
    if (!f) { 
        perror("No se pudo abrir tokens.def"); 
        exit(1); 
    }

    char linea[256];
    num_tokens = 0;

    while (fgets(linea, sizeof(linea), f)) {
        char* comentario = strchr(linea, '#');
        if (comentario) *comentario = '\0';
        
        char* inicio = linea;
        while (*inicio && isspace(*inicio)) inicio++;
        
        char* fin = inicio + strlen(inicio) - 1;
        while (fin > inicio && isspace(*fin)) fin--;
        *(fin + 1) = '\0';
        
        if (*inicio == '\0') continue;
        
        char* sep = strchr(inicio, ' ');
        if (!sep) {
            fprintf(stderr, "Error: formato incorrecto en línea: %s\n", inicio);
            exit(1);
        }
        
        *sep = '\0';
        char* nombre = inicio;
        char* regex = sep + 1;
        while (*regex && isspace(*regex)) regex++;
        
        if (strlen(nombre) >= sizeof(tokens[0].nombre)) {
            fprintf(stderr, "Error: nombre de token demasiado largo: %s\n", nombre);
            exit(1);
        }
        
        strcpy(tokens[num_tokens].nombre, nombre);
        tokens[num_tokens].regex = strdup(regex);
        num_tokens++;
        
        if (num_tokens >= MAX_TOKENS) {
            fprintf(stderr, "Error: demasiados tokens definidos (máximo %d)\n", MAX_TOKENS);
            exit(1);
        }
    }

    fclose(f);
}

void generar_dfa_normal(FILE* out, EntradaToken* token) {
    fprintf(out, "static EstadoDFA dfa_%s[] = {\n", token->nombre);
    
    for (int i = 0; i < token->dfa.cantidad_estados; i++) {
        EstadoDFA* est = &token->dfa.estados[i];
        fprintf(out, "    { %d, %d, {", est->id, est->num_transiciones);

        for (int j = 0; j < est->num_transiciones; j++) {
            char s = est->transiciones[j].simbolo;
            int destino = est->transiciones[j].destino;

            fprintf(out, "{%d, ", destino);
            escribir_caracter_especial(out, s);
            fprintf(out, "}");

            if (j < est->num_transiciones - 1)
                fprintf(out, ", ");
        }

        fprintf(out, "}, %d, TOKEN_%s },\n", est->es_final, token->nombre);
    }
    fprintf(out, "};\n\n");
}

void generar_next_token(FILE* out) {
    fprintf(out, "Token next_token(const char** input) {\n");
    fprintf(out, "    // Saltar espacios en blanco\n");
    fprintf(out, "    while (**input && isspace((unsigned char)**input)) {\n");
    fprintf(out, "        if (**input == '\\n') {\n");
    fprintf(out, "            current_line++;\n");
    fprintf(out, "            current_column = 1;\n");
    fprintf(out, "        } else {\n");
    fprintf(out, "            current_column++;\n");
    fprintf(out, "        }\n");
    fprintf(out, "        (*input)++;\n");
    fprintf(out, "    }\n\n");
    
    fprintf(out, "    // Manejar fin de entrada\n");
    fprintf(out, "    if (**input == '\\0') {\n");
    fprintf(out, "        return (Token){TOKEN_EOF, NULL, 0, current_line, current_column};\n");
    fprintf(out, "    }\n\n");
    
    fprintf(out, "    int max_len = 0;\n");
    fprintf(out, "    TokenType tipo = TOKEN_ERROR;\n\n");

    for (int t = 0; t < num_tokens; t++) {
        fprintf(out, "    {\n");
        fprintf(out, "        int len = 0;\n");
        fprintf(out, "        TokenType t = match_dfa(dfa_%s, %d, *input, &len);\n",
                tokens[t].nombre, tokens[t].dfa.cantidad_estados);
        fprintf(out, "        if (t != TOKEN_ERROR && len > max_len) {\n");
        fprintf(out, "            max_len = len;\n");
        fprintf(out, "            tipo = t;\n");
        fprintf(out, "        }\n");
        fprintf(out, "    }\n");
    }

    fprintf(out, "\n    if (max_len > 0) {\n");
    fprintf(out, "        char* lexema = my_strndup(*input, max_len);\n");
    fprintf(out, "        if (!lexema) {\n");
    fprintf(out, "            return (Token){TOKEN_ERROR, NULL, 0, current_line, current_column};\n");
    fprintf(out, "        }\n");
    fprintf(out, "        int token_line = current_line;\n");
    fprintf(out, "        int token_column = current_column;\n");
    fprintf(out, "        \n");
    fprintf(out, "        for (int i = 0; i < max_len; i++) {\n");
    fprintf(out, "            if ((*input)[i] == '\\n') {\n");
    fprintf(out, "                current_line++;\n");
    fprintf(out, "                current_column = 1;\n");
    fprintf(out, "            } else {\n");
    fprintf(out, "                current_column++;\n");
    fprintf(out, "            }\n");
    fprintf(out, "        }\n");
    fprintf(out, "        *input += max_len;\n");
    fprintf(out, "        return (Token){tipo, lexema, max_len, token_line, token_column};\n");
    fprintf(out, "    }\n\n");

    fprintf(out, "    // Carácter no reconocido\n");
    fprintf(out, "    if (**input == '\\0') {\n");
    fprintf(out, "        return (Token){TOKEN_EOF, NULL, 0, current_line, current_column};\n");
    fprintf(out, "    }\n\n");
    
    fprintf(out, "    char* error_lexema = my_strndup(*input, 1);\n");
    fprintf(out, "    int error_line = current_line;\n");
    fprintf(out, "    int error_column = current_column;\n");
    fprintf(out, "    if (**input == '\\n') {\n");
    fprintf(out, "        current_line++;\n");
    fprintf(out, "        current_column = 1;\n");
    fprintf(out, "    } else {\n");
    fprintf(out, "        current_column++;\n");
    fprintf(out, "    }\n");
    fprintf(out, "    (*input)++;\n");
    fprintf(out, "    return (Token){TOKEN_ERROR, error_lexema, 1, error_line, error_column};\n");
    fprintf(out, "}\n\n");
}

void generar_lexer_c(FILE *out) {
    if (!out) {
        fprintf(stderr, "Error: Archivo de salida no válido\n");
        exit(EXIT_FAILURE);
    }

    fprintf(out, "// Lexer generado automáticamente - NO EDITAR MANUALMENTE\n\n");
    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "#include <stdlib.h>\n");
    fprintf(out, "#include <string.h>\n");
    fprintf(out, "#include <ctype.h>\n");
    fprintf(out, "#include \"lexer.h\"\n\n");

    fprintf(out, "static char* my_strndup(const char* src, size_t n) {\n");
    fprintf(out, "    char* s = malloc(n + 1);\n");
    fprintf(out, "    if (!s) return NULL;\n");
    fprintf(out, "    strncpy(s, src, n);\n");
    fprintf(out, "    s[n] = '\\0';\n");
    fprintf(out, "    return s;\n");
    fprintf(out, "}\n\n");

    fprintf(out, "static int match_dfa(EstadoDFA* estados, int num_estados, const char* input, int* length) {\n");
    fprintf(out, "    int estado_actual = 0;\n");
    fprintf(out, "    int last_final = -1;\n");
    fprintf(out, "    int last_final_state = -1;\n");
    fprintf(out, "    *length = 0;\n\n");
    fprintf(out, "    while (input[*length]) {\n");
    fprintf(out, "        char c = input[*length];\n");
    fprintf(out, "        int encontrado = 0;\n\n");
    fprintf(out, "        for (int i = 0; i < estados[estado_actual].num_transiciones; i++) {\n");
    fprintf(out, "            if (estados[estado_actual].transiciones[i].simbolo == c) {\n");
    fprintf(out, "                estado_actual = estados[estado_actual].transiciones[i].destino;\n");
    fprintf(out, "                (*length)++;\n");
    fprintf(out, "                if (estados[estado_actual].es_final) {\n");
    fprintf(out, "                    last_final = *length;\n");
    fprintf(out, "                    last_final_state = estado_actual;\n");
    fprintf(out, "                }\n");
    fprintf(out, "                encontrado = 1;\n");
    fprintf(out, "                break;\n");
    fprintf(out, "            }\n");
    fprintf(out, "        }\n");
    fprintf(out, "        if (!encontrado) break;\n");
    fprintf(out, "    }\n");
    fprintf(out, "    if (last_final > 0) {\n");
    fprintf(out, "        *length = last_final;\n");
    fprintf(out, "        return estados[last_final_state].tipo;\n");
    fprintf(out, "    }\n");
    fprintf(out, "    return TOKEN_ERROR;\n");
    fprintf(out, "}\n\n");

    for (int t = 0; t < num_tokens; t++) {
        extern int estado_id_global;
        extern int estado_dfa_id_global;
        estado_id_global = 0;
        estado_dfa_id_global = 0;

        FragmentoNFA nfa;

        if (strcmp(tokens[t].nombre, "IDENTIFIER") == 0) {
            nfa = crear_identificador();
        } else {
            nfa = parse_regex(tokens[t].regex);
        }

        nfa.fin->es_final = 1;

        tokens[t].dfa = convertir_nfa_a_dfa(nfa.inicio, t);

        generar_dfa_normal(out, &tokens[t]);

        liberar_nfa(nfa.inicio);
        
        if (ferror(out)) {
            fprintf(stderr, "Error al escribir en el archivo de salida\n");
            exit(EXIT_FAILURE);
        }
    }

    fprintf(out, "static int current_line = 1;\n");
    fprintf(out, "static int current_column = 1;\n\n");

    generar_next_token(out);
    
  
    fprintf(out, "const char* token_type_to_str(TokenType type) {\n");
    fprintf(out, "    static const char* names[] = {\n");
    for (int i = 0; i < num_tokens; i++) {
        fprintf(out, "        \"%s\",\n", tokens[i].nombre);
    }
     
    fprintf(out, "        \"EOF\",\n");
    fprintf(out, "        \"ERROR\"\n");
    fprintf(out, "    };\n\n");
    fprintf(out, "    if (type >= 0 && type < %d) return names[type];\n", num_tokens);
    fprintf(out, "    if (type == TOKEN_EOF) return names[%d];\n", num_tokens);
    fprintf(out, "    return names[%d];\n", num_tokens + 1);
    fprintf(out, "}\n\n");

    fprintf(out, "static const char* token_names[] = {\n");
    for (int i = 0; i < num_tokens; i++) {
        fprintf(out, "    \"%s\",\n", tokens[i].nombre);
    }
    fprintf(out, "    \"EOF\",\n");
    fprintf(out, "    \"ERROR\"\n");
    fprintf(out, "};\n\n");

    fprintf(out, "void print_token(Token t) {\n");
    fprintf(out, "    if (t.type == TOKEN_EOF) {\n");
    fprintf(out, "        printf(\"<EOF>\\n\");\n");
    fprintf(out, "    } else if (t.type == TOKEN_ERROR) {\n");
    fprintf(out, "        printf(\"<ERROR: '%.*s' at line %d, column %d>\\n\",\n");
    fprintf(out, "               t.length, t.lexema, t.line, t.column);\n");
    fprintf(out, "    } else {\n");
    fprintf(out, "        printf(\"<%%s: '%%.*s' at line %%d, column %%d>\\n\",\n");
    fprintf(out, "               token_names[t.type], t.length, t.lexema, t.line, t.column);\n");
    fprintf(out, "    }\n");
    fprintf(out, "    if (t.lexema) free(t.lexema);\n");
    fprintf(out, "}\n");
   
    fflush(out);
    if (ferror(out)) {
        fprintf(stderr, "Error al escribir en el archivo de salida\n");
        exit(EXIT_FAILURE);
    }
}


int main() {

    leer_tokens("lexer_gen/tokens.def");
    // Generar lexer.c
    FILE *f = fopen("lexer/lexer.c", "w");
    if (!f) {
        perror("No se pudo abrir lexer.c");
        return 1;
    }
    generar_lexer_c(f);
    fclose(f);

    return 0;
}

