#include "generacion_codigo.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
LLVMType tipo_funcion(const char* nombre) {
    for (int i = 0; i < total_funciones; i++) {
        if (strcmp(funciones[i].nombre, nombre) == 0)
            return funciones[i].tipo_retorno;
    }
    return TIPO_DESCONOCIDO;
}

void registrar_funcion(const char* nombre, LLVMType tipo) {
    funciones[total_funciones].nombre = strdup(nombre);
    funciones[total_funciones].tipo_retorno = tipo;
    total_funciones++;
}
int get_valor(ExpressionNode* node) {
    if (!node) return -1;
    return generar_codigo(node);
}
LLVMType tipo_expr(ExpressionNode* expr) {
    if (!expr) return TIPO_DESCONOCIDO;

    NodeType tipo = ((Node*)expr)->tipo;
    switch (tipo) {
        case NODE_STRING:
        case NODE_CONCAT:
            return TIPO_STRING;

        case NODE_CALL_FUNC: {
            CallFuncNode* call = (CallFuncNode*)expr;
            if (strcmp(call->name, "strcat2") == 0 || strcmp(call->name, "int_to_string") == 0)
                return TIPO_STRING;
            return TIPO_INT;
        }

       case NODE_RETURN: {
            ReturnNode* ret = (ReturnNode*)expr;
            return ret->expr ? tipo_expr(ret->expr) : TIPO_VOID;
        }


        case NODE_CONDITIONAL: {
            ConditionalNode* c = (ConditionalNode*)expr;
            ExpressionNode** then_exprs = (ExpressionNode**)c->expressions;
            ExpressionNode* else_expr = (ExpressionNode*)c->default_expre;

            if (then_exprs && then_exprs[0]) {
                LLVMType t1 = tipo_expr(then_exprs[0]);
                if (else_expr) {
                    LLVMType t2 = tipo_expr(else_expr);
                    return (t1 == t2) ? t1 : TIPO_INT; // Default to int if mismatch
                }
                return t1;
            }
            if (else_expr)
                return tipo_expr(else_expr);

            return TIPO_INT;
        }

        case NODE_LET_IN: {
            LetInNode* let = (LetInNode*)expr;
            return tipo_expr((ExpressionNode*)let->body);
        }

        case NODE_NUMBER:
        case NODE_PLUS: case NODE_MINUS:
        case NODE_MULT: case NODE_DIV:
        case NODE_MOD:
        case NODE_EQUAL: case NODE_NOT_EQUAL:
        case NODE_LESS: case NODE_LESS_EQUAL:
        case NODE_GREATER: case NODE_GREATER_EQUAL:
        case NODE_BOOLEAN:
            return TIPO_INT;

        default:
            return TIPO_DESCONOCIDO;
    }
}
LLVMType tipo_param_funcion(const char* nombre, int i) {
    for (int j = 0; j < total_funciones; j++) {
        if (strcmp(funciones[j].nombre, nombre) == 0) {
            if (i < funciones[j].cantidad_param) {
                return funciones[j].tipos_param[i];
            }
        }
    }
    return TIPO_INT;  // por defecto
}
int generar_binario(BinaryNode* bin, const char* opcode) {
    Node* izq_n = (Node*)bin->left;
    Node* der_n = (Node*)bin->right;

    // Detectar si estamos operando con float
    int es_float = 0;
    if (izq_n->tipo == NODE_NUMBER && strchr(((LiteralNode*)bin->left)->lex, '.')) {
        es_float = 1;
    }
    if (der_n->tipo == NODE_NUMBER && strchr(((LiteralNode*)bin->right)->lex, '.')) {
        es_float = 1;
    }

    // Constant folding si ambos son literales
    if (izq_n->tipo == NODE_NUMBER && der_n->tipo == NODE_NUMBER) {
        int temp = nuevo_temp();

        if (es_float) {
            float v1 = atof(((LiteralNode*)bin->left)->lex);
            float v2 = atof(((LiteralNode*)bin->right)->lex);
            float resultado = 0;

            if (strcmp(opcode, "add") == 0) resultado = v1 + v2;
            else if (strcmp(opcode, "sub") == 0) resultado = v1 - v2;
            else if (strcmp(opcode, "mul") == 0) resultado = v1 * v2;
            else if (strcmp(opcode, "sdiv") == 0) resultado = v1 / v2;

            // Guardar como constante flotante
            fprintf(salida_llvm, "  %%%d = fadd float 0.0, %f\n", temp, resultado);
        } else {
            int v1 = atoi(((LiteralNode*)bin->left)->lex);
            int v2 = atoi(((LiteralNode*)bin->right)->lex);
            int resultado = 0;

            if (strcmp(opcode, "add") == 0) resultado = v1 + v2;
            else if (strcmp(opcode, "sub") == 0) resultado = v1 - v2;
            else if (strcmp(opcode, "mul") == 0) resultado = v1 * v2;
            else if (strcmp(opcode, "sdiv") == 0) resultado = v1 / v2;
            else if (strcmp(opcode, "srem") == 0) resultado = v1 % v2;

            fprintf(salida_llvm, "  %%%d = add i32 0, %d\n", temp, resultado);
        }
        return temp;
    }

    // Evaluación normal
    int izq = get_valor(bin->left);
    int der = get_valor(bin->right);
    int temp = nuevo_temp();

    if (es_float) {
        // Instrucciones float: fadd, fsub, fmul, fdiv
        const char* mop = NULL;
        if (strcmp(opcode, "add") == 0) mop = "fadd";
        else if (strcmp(opcode, "sub") == 0) mop = "fsub";
        else if (strcmp(opcode, "mul") == 0) mop = "fmul";
        else if (strcmp(opcode, "sdiv") == 0) mop = "fdiv";
        else mop = "fadd"; // por defecto

        fprintf(salida_llvm, "  %%%d = %s float %%%d, %%%d\n", temp, mop, izq, der);
    } else {
        fprintf(salida_llvm, "  %%%d = %s i32 %%%d, %%%d\n", temp, opcode, izq, der);
    }

    return temp;
}

int generar_comparacion(BinaryNode* bin, const char* cmp) {
    Node* izq_n = (Node*)bin->left;
    Node* der_n = (Node*)bin->right;

    // Detectar si es comparación de floats
    int es_float = 0;
    if (izq_n->tipo == NODE_NUMBER && strchr(((LiteralNode*)bin->left)->lex, '.'))
        es_float = 1;
    if (der_n->tipo == NODE_NUMBER && strchr(((LiteralNode*)bin->right)->lex, '.'))
        es_float = 1;

    int izq = get_valor(bin->left);
    int der = get_valor(bin->right);
    int temp = nuevo_temp();

    if (es_float) {
        // Ordenado no igual: oeq, ogt, oge, olt, ole, one
        // También puedes usar "ueq", "ult", etc. si no quieres orden estricto.
        fprintf(salida_llvm, "  %%%d = fcmp %s float %%%d, %%%d\n", temp, cmp, izq, der);
    } else {
        fprintf(salida_llvm, "  %%%d = icmp %s i32 %%%d, %%%d\n", temp, cmp, izq, der);
    }

    return temp;
}

int contiene_return(ExpressionNode* expr) {
    if (!expr) return 0;
    NodeType tipo = ((Node*)expr)->tipo;

    if (tipo == NODE_RETURN) return 1;

    if (tipo == NODE_EXPRESSION_BLOCK) {
        ExpressionBlockNode* block = (ExpressionBlockNode*)expr;
        ExpressionNode** exprs = (ExpressionNode**)block->expressions;
        for (int i = 0; exprs && exprs[i]; i++) {
            if (contiene_return(exprs[i])) return 1;
        }
    }

    if (tipo == NODE_CONDITIONAL) {
        ConditionalNode* cond = (ConditionalNode*)expr;
        ExpressionNode** exprs = (ExpressionNode**)cond->expressions;
        for (int i = 0; exprs && exprs[i]; i++) {
            if (contiene_return(exprs[i])) return 1;
        }
        if (cond->default_expre && contiene_return(cond->default_expre)) return 1;
    }

    return 0;
}
void generar_declaraciones_variables() {
    for (int i = 0; i < num_variables; i++) {
        // Asegurarse que el nombre no esté vacío
        if (variables_usadas[i].nombre && strlen(variables_usadas[i].nombre) > 0) {
            fprintf(salida_llvm, "  %%var_%s = alloca i32\n", nombre_llvm(variables_usadas[i].nombre));
            fprintf(salida_llvm, "  store i32 0, i32* %%var_%s\n", nombre_llvm(variables_usadas[i].nombre));
        }
    }
}
int es_comparacion(NodeType tipo) {
    return (tipo == NODE_EQUAL || tipo == NODE_NOT_EQUAL ||
            tipo == NODE_LESS || tipo == NODE_LESS_EQUAL ||
            tipo == NODE_GREATER || tipo == NODE_GREATER_EQUAL||
            tipo == NODE_BOOLEAN);
}
