#ifndef CODEGEN_H
#define CODEGEN_H
#include "../parser/ast_nodes.h"
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
#define MAX_CONST_ENV 100

typedef struct {
    char* nombre;
    char* valor;  // almacenamos la cadena del número ("5", "42", etc.)
} Constante;


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
ExpressionNode* optimizar_constantes(ExpressionNode* expr);
#endif
