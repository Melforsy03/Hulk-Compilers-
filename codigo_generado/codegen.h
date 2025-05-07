#ifndef CODEGEN_H
#define CODEGEN_H
#include "../ast_nodes/ast_nodes.h"


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

int generar_codigo(ExpressionNode* expr);
int generar_etiqueta();
void reset_temporales();
void generar_programa(ProgramNode* program);
const char* obtener_nombre_variable(VarNode* var);
#endif
