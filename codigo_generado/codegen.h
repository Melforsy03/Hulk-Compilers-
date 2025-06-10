#ifndef CODEGEN_H
#define CODEGEN_H
#include "../ast_nodes/ast_nodes.h"
#include <stdio.h>

typedef struct {
    char* valor;
    int id;
} StringConst;
typedef enum {
    VAR_TYPE_INT,
    VAR_TYPE_STRING
} VarType;

typedef struct {
    char* nombre;
    VarType tipo;
    int inicializada;
} Variable;

#define MAX_VARIABLES 100
extern Variable variables_usadas[MAX_VARIABLES];
extern int num_variables;
extern FILE* salida_llvm;

int generar_codigo(ExpressionNode* expr);
int generar_etiqueta();
void reset_temporales();
void generar_programa(ProgramNode* program);
const char* obtener_nombre_variable(VarNode* var);
// Funciones auxiliares usadas en codegen.c
int get_valor(ExpressionNode* node);
int generar_binario(BinaryNode* bin, const char* opcode);
int generar_comparacion(BinaryNode* bin, const char* cmp);
int registrar_string_global(const char* texto);
void recorrer_ast_para_strings(ExpressionNode* expr);
void generar_funciones(ExpressionNode* expr);
#endif
// int main() {
//     fprintf(stderr, "[INFO] Creando AST de prueba...\n");

//     ProgramNode* prog = crear_programa_de_prueba();
//     if (!prog) {
//         fprintf(stderr, "[ERROR] AST no fue creado correctamente (prog == NULL)\n");
//         return 1;
//     }

//     // Abrimos el archivo de salida LLVM
//     salida_llvm = fopen("prueba.ll", "w");
//     if (!salida_llvm) {
//         fprintf(stderr, "[ERROR] No se pudo abrir el archivo de salida 'prueba.ll'\n");
//         return 1;
//     }

//     fprintf(stderr, "[INFO] AST creado exitosamente. Iniciando generación de código...\n");

//     generar_programa(prog);

//     fprintf(stderr, "[INFO] Generación completada. Código escrito en 'prueba.ll'\n");

//     fclose(salida_llvm);
//     return 0;
// }