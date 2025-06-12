#include "generacion_codigo.h"
#include <string.h> 
int contador_temporales = 0;
int contador_etiquetas = 0;
int tmp_counter = 0;
char last_tmp[32];
Variable variables_usadas[MAX_VARIABLES];
int num_variables = 0;

int nuevo_temp() {
    return contador_temporales++;
}

int nuevo_label() {
    return contador_etiquetas++;
}

void reset_temporales() {
    contador_temporales = 0;
}

const char* nuevo_tmp() {
    sprintf(last_tmp, "%%t%d", tmp_counter++);
    return last_tmp;
}

const char* tmp_actual() {
    return last_tmp;
}

const char* obtener_nombre_variable(VarNode* var) {
    return ((LiteralNode*)var)->lex;
}

VarType obtener_tipo_variable(const char* nombre) {
    for (int i = 0; i < num_variables; i++) {
        if (strcmp(variables_usadas[i].nombre, nombre) == 0) {
            return variables_usadas[i].tipo;
        }
    }
    return VAR_TYPE_INT;
}

void registrar_variables(ProgramNode* program) {
    if (!program || !program->declarations) return;

    VarDeclarationNode** decls = (VarDeclarationNode**)program->declarations;
    for (int i = 0; decls[i] != NULL; ++i) {
        variables_usadas[num_variables].nombre = strdup(decls[i]->name);
        variables_usadas[num_variables].tipo = VAR_TYPE_INT;
        variables_usadas[num_variables].inicializada = 0;
        num_variables++;
    }
}

int variable_existe(const char* nombre) {
    for (int i = 0; i < num_variables; i++) {
        if (strcmp(variables_usadas[i].nombre, nombre) == 0) {
            return 1;
        }
    }
    return 0;
}
