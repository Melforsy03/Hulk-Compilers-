#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "./codigo_generado/parser.h"
#include "./codigo_generado/codegen.h"
//#include "./lexer_generado/generar_lexer.h"
#include "./lexer/lexer.h"
char* leer_archivo(const char* nombre_archivo);

#define MAX_LINEA 1024

int main() {
    //para probar el parser 
    // printf("; archivo LLVM generado\n"); // línea segura inicial
    // printf("%%Point = type { i32, i32 }\n");
    // printf("declare void @print(i32)\n\n");
    // printf("define i32 @main() {\n");
    // printf("entry:\n");

    // Node* programa = parse();
    // generar_codigo(programa);

    // printf("  ret i32 0\n");
    // printf("}\n");

    // return 0;

    //Para probar el lexer 
    // char linea[MAX_LINEA];

    // printf("Introduce una línea de código: ");
    // if (fgets(linea, MAX_LINEA, stdin) == NULL) {
    //     fprintf(stderr, "Error leyendo la entrada.\n");
    //     return 1;
    // }

    // const char* ptr = linea;

    // printf("\n📦 Tokens encontrados:\n");
    // while (*ptr) {
    //     // Saltar espacios y saltos de línea
    //     while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n') ptr++;
    //     if (*ptr == '\0') break;

    //     Token t = next_token(ptr);
    //     if (t.type == TOKEN_ERROR) {
    //         printf("❌ Token no reconocido: '%.*s'\n", t.length, t.lexema);
    //     } else {
    //         print_token(t);
    //     }

    //     ptr += t.length;
    // }

    // return 0;
}