#include <stdio.h> 
#include <stdio.h>
#include <stdlib.h>
#include "../lexer/lexer.h"
#include "../codigo_generado/generacion_codigo.h"
#include "../parser/parser.h"
#include "../parser/print_ast.h"

// Declarar tu FILE* global
FILE* salida_llvm;

int main() {
    // 1️⃣ Abrir archivo de entrada
    FILE* archivo_fuente = fopen("entrada.txt", "r");
    if (!archivo_fuente) {
        perror("No se pudo abrir fuente.txt");
        exit(1);
    }

    // 2️⃣ Leer todo el contenido en un buffer dinámico
    fseek(archivo_fuente, 0, SEEK_END);
    long tamano = ftell(archivo_fuente);
    fseek(archivo_fuente, 0, SEEK_SET);

    char* codigo_fuente = malloc(tamano + 1);
    if (!codigo_fuente) {
        fprintf(stderr, "No se pudo asignar memoria\n");
        fclose(archivo_fuente);
        exit(1);
    }

    fread(codigo_fuente, 1, tamano, archivo_fuente);
    codigo_fuente[tamano] = '\0'; // Null-terminate

    fclose(archivo_fuente);

    // 3️⃣ Inicializar parser
    init_parser(codigo_fuente);

    // 4️⃣ Parsear a AST
    ProgramNode* program = parse_program();
    print_program(program);

    // 5️⃣ Generar LLVM IR
    salida_llvm = fopen("hulk/programa.ll", "w");
    if (!salida_llvm) {
        perror("No se pudo abrir output.ll");
        exit(1);
    }

    generar_programa(program);
    fclose(salida_llvm);

    free(codigo_fuente);
    return 0;
}
