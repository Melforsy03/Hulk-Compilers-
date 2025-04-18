
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINEA 256

typedef struct {
    char* nombre;
    char* patron;
} ReglaToken;

ReglaToken* reglas = NULL;
 int cantidad =0; 
 int capacidad=0 ;


char* trim(char* str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char* fin = str + strlen(str) - 1;
    while (fin > str && isspace((unsigned char)*fin)) fin--;
    *(fin + 1) = '\0';
    return str;
}

void agregar_regla(const char* nombre, const char* patron) {
    if (cantidad >= capacidad) {
        capacidad = capacidad == 0 ? 16 : capacidad * 2;
        reglas = realloc(reglas, sizeof(ReglaToken) * capacidad);
    }
    reglas[cantidad].nombre = strdup(nombre);
    reglas[cantidad].patron = strdup(patron);
    cantidad++;
}

void leer_tokens_lex(const char* archivo) {
    FILE* f = fopen(archivo, "r");
    if (!f) {
        fprintf(stderr, "No se pudo abrir %s\n", archivo);
        exit(1);
    }

    char linea[MAX_LINEA];
    while (fgets(linea, sizeof(linea), f)) {
        if (linea[0] == '#' || linea[0] == '\n') continue;
        char* igual = strchr(linea, '=');
        if (!igual) continue;

        *igual = '\0';
        char* nombre = trim(linea);
        char* patron = trim(igual + 1);
        agregar_regla(nombre, patron);
    }
    fclose(f);
}

void generar_archivos_lexer(const char* archivo_c, const char* archivo_h) {
    FILE* fc = fopen(archivo_c, "w");
    FILE* fh = fopen(archivo_h, "w");

    if (!fc || !fh) {
        fprintf(stderr, "No se pudo crear los archivos '%s' o '%s'\n", archivo_c, archivo_h);
        exit(1);
    }

    // Header (.h)
    fprintf(fh, "#ifndef LEXER_MODULO_H\n#define LEXER_MODULO_H\n\n");
    fprintf(fh, "typedef enum {\n");
    for (int i = 0; i < cantidad; i++) {
        fprintf(fh, "    TOKEN_%s,\n", reglas[i].nombre);
    }
    fprintf(fh, "    TOKEN_ERROR\n} TokenType;\n\n");
    fprintf(fh, "typedef struct {\n    TokenType type;\n    const char* lexeme;\n    int line;\n} Token;\n\n");
    fprintf(fh, "Token analizar(const char* lexema);\n\n#endif\n");

    // Source (.c)
    fprintf(fc, "#include <string.h>\n#include \"lexer_modulo.h\"\n\n");
    fprintf(fc, "Token analizar(const char* lexema) {\n");
    for (int i = 0; i < cantidad; i++) {
        if (reglas[i].patron[0] == '\"') {
            fprintf(fc, "    if (strcmp(lexema, %s) == 0) return (Token){TOKEN_%s, lexema, lexer.linea};\n",
                    reglas[i].patron, reglas[i].nombre);
        }
    }
    fprintf(fc, "    return (Token){TOKEN_ERROR, lexema, 0};\n}\n");

    fclose(fc);
    fclose(fh);

    printf("Lexer generado en '%s' y '%s'\n", archivo_c, archivo_h);
}


// int main() {
//     leer_tokens_lex("tokens.lex");
//     generar_archivos_lexer("lexer_modulo.c", "lexer_modulo.h");
//     return 0;
// }

