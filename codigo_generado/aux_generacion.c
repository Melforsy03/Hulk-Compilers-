#include "generacion_codigo.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
RegistroTipo tipos[MAX_TIPOS];
int total_tipos = 0;
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
            if (!call || !call->name) return TIPO_DESCONOCIDO;

            if (strcmp(call->name, "strcat2") == 0 || 
                strcmp(call->name, "int_to_string") == 0 ||
                strcmp(call->name, "bool_to_string") == 0) {
                return TIPO_STRING;
            }

            for (int i = 0; i < total_funciones; i++) {
                if (funciones[i].nombre && strcmp(funciones[i].nombre, call->name) == 0) {
                    return funciones[i].tipo_retorno;
                }
            }
            return TIPO_DESCONOCIDO;
        }

        case NODE_RETURN: {
            ReturnNode* ret = (ReturnNode*)expr;
            return ret && ret->expr ? tipo_expr(ret->expr) : TIPO_VOID;
        }

        case NODE_CONDITIONAL: {
            ConditionalNode* c = (ConditionalNode*)expr;
            if (!c) return TIPO_DESCONOCIDO;

            ExpressionNode** then_exprs = (ExpressionNode**)c->expressions;
            ExpressionNode* else_expr = (ExpressionNode*)c->default_expre;

            if (then_exprs && then_exprs[0]) {
                LLVMType t1 = tipo_expr(then_exprs[0]);
                if (else_expr) {
                    LLVMType t2 = tipo_expr(else_expr);
                    return (t1 == t2) ? t1 : TIPO_INT;
                }
                return t1;
            }
            return else_expr ? tipo_expr(else_expr) : TIPO_INT;
        }

        case NODE_LET_IN: {
            LetInNode* let = (LetInNode*)expr;
            return let && let->body ? tipo_expr((ExpressionNode*)let->body) : TIPO_DESCONOCIDO;
        }

        case NODE_NUMBER: {
            NumberNode* num = (NumberNode*)expr;
            if (!num || !num->base.lex) return TIPO_DESCONOCIDO;
            return strchr(num->base.lex, '.') ? TIPO_FLOAT : TIPO_INT;
        }

        case NODE_PLUS: case NODE_MINUS:
        case NODE_MULT: case NODE_DIV: {
            BinaryNode* bin = (BinaryNode*)expr;
            if (!bin || !bin->left || !bin->right) return TIPO_DESCONOCIDO;
            
            LLVMType t1 = tipo_expr(bin->left);
            LLVMType t2 = tipo_expr(bin->right);
            return (t1 == TIPO_FLOAT || t2 == TIPO_FLOAT) ? TIPO_FLOAT : TIPO_INT;
        }

        case NODE_MOD:
        case NODE_EQUAL: case NODE_NOT_EQUAL:
        case NODE_LESS: case NODE_LESS_EQUAL:
        case NODE_GREATER: case NODE_GREATER_EQUAL:
        case NODE_BOOLEAN:
            return TIPO_BOOL;

        case NODE_NEGATIVE: {
            UnaryNode* un = (UnaryNode*)expr;
            return un && un->operand ? tipo_expr(un->operand) : TIPO_DESCONOCIDO;
        }
        case NODE_CALL_METHOD: return TIPO_INT;
        case NODE_ASSING: return TIPO_VOID;
        case NODE_CALL_TYPE_ATTRIBUTE: {
            CallTypeAttributeNode* nodo = (CallTypeAttributeNode*)expr;
            return obtener_tipo_campo(nodo->inst_name, nodo->attribute);
        }
        case NODE_VAR: return TIPO_INT; // O lo que sea correcto para tu lenguaje.


        default:
            return TIPO_DESCONOCIDO;
    }
}
int obtener_indice_campo(const char* tipo_nombre, const char* campo) {
    // Recorre la lista de tipos declarados (ya la deberías tener guardada al declarar)
    for (int i = 0; i < total_tipos; i++) {
        if (strcmp(tipos[i].nombre, tipo_nombre) == 0) {
            for (int j = 0; j < tipos[i].num_campos; j++) {
                if (strcmp(tipos[i].nombres_campos[j], campo) == 0) {
                    return j;
                }
            }
        }
    }
    fprintf(stderr, "[ERROR] Campo '%s' no encontrado en tipo '%s'\n", campo, tipo_nombre);
    return 0; // fallback seguro
}
LLVMType obtener_tipo_campo(const char* tipo_nombre, const char* campo) {
    for (int i = 0; i < total_tipos; i++) {
        if (strcmp(tipos[i].nombre, tipo_nombre) == 0) {
            for (int j = 0; j < tipos[i].num_campos; j++) {
                if (strcmp(tipos[i].nombres_campos[j], campo) == 0) {
                    return tipos[i].tipos_campos[j];
                }
            }
        }
    }
    return TIPO_INT; // por defecto
}
void registrar_tipo(const char* nombre, TypeAttributeNode** atributos) {
    if (total_tipos >= MAX_TIPOS) return;

    RegistroTipo* t = &tipos[total_tipos++];
    t->nombre = strdup(nombre);
    t->num_campos = 0;

    for (int i = 0; atributos && atributos[i]; i++) {
        t->nombres_campos[i] = strdup(atributos[i]->name);
        if (atributos[i]->type && strcmp(atributos[i]->type, "string") == 0)
            t->tipos_campos[i] = TIPO_STRING;
        else if (atributos[i]->type && strcmp(atributos[i]->type, "float") == 0)
            t->tipos_campos[i] = TIPO_FLOAT;
        else if (atributos[i]->type && strcmp(atributos[i]->type, "bool") == 0)
            t->tipos_campos[i] = TIPO_BOOL;
        else
            t->tipos_campos[i] = TIPO_INT;

        t->num_campos++;
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

    // Si ambos lados son números, hacer constant folding
    if (izq_n->tipo == NODE_NUMBER && der_n->tipo == NODE_NUMBER) {
        int v1 = atoi(((LiteralNode*)bin->left)->lex);
        int v2 = atoi(((LiteralNode*)bin->right)->lex);
        int resultado = 0;

        if (strcmp(opcode, "add") == 0) resultado = v1 + v2;
        else if (strcmp(opcode, "sub") == 0) resultado = v1 - v2;
        else if (strcmp(opcode, "mul") == 0) resultado = v1 * v2;
        else if (strcmp(opcode, "sdiv") == 0) resultado = v1 / v2;
        else if (strcmp(opcode, "srem") == 0) resultado = v1 % v2;

        int temp = nuevo_temp();
        fprintf(salida_llvm, "  %%%d = add i32 0, %d\n", temp, resultado);
        return temp;
    }

    // Evaluar normalmente
    int izq = get_valor(bin->left);
    int der = get_valor(bin->right);
    int temp = nuevo_temp();
    fprintf(salida_llvm, "  %%%d = %s i32 %%%d, %%%d\n", temp, opcode, izq, der);
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
            tipo == NODE_GREATER || tipo == NODE_GREATER_EQUAL ||
            tipo == NODE_BOOLEAN);
 }
