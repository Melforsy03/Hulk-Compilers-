#include "generacion_codigo.h"
#include <stdlib.h>   
#include <string.h>   

Constante entorno_constantes[MAX_CONST_ENV];
int num_constantes = 0;

const char* obtener_valor_constante(const char* nombre) {
    for (int i = 0; i < num_constantes; i++) {
        if (strcmp(entorno_constantes[i].nombre, nombre) == 0) {
            return entorno_constantes[i].valor;
        }
    }
    return NULL;
}

ExpressionNode* crear_nodo_constante(const char* valor) {
    NumberNode* nodo = calloc(1, sizeof(NumberNode));
    nodo->base.base.base.base.tipo = NODE_NUMBER;
    nodo->base.lex = strdup(valor);
    return (ExpressionNode*)nodo;
}


ExpressionNode* optimizar_constantes(ExpressionNode* expr) {
    if (!expr) return NULL;

    NodeType tipo = ((Node*)expr)->tipo;

    switch (tipo) {
        // OPT: Binarias con constantes
        case NODE_PLUS: case NODE_MINUS:
        case NODE_MULT: case NODE_DIV:
        case NODE_MOD: case NODE_EQUAL: case NODE_NOT_EQUAL:
        case NODE_LESS:  case NODE_LESS_EQUAL:
        case NODE_GREATER:  case NODE_GREATER_EQUAL: {
            BinaryNode* bin = (BinaryNode*)expr;
            bin->left = optimizar_constantes(bin->left);
            bin->right = optimizar_constantes(bin->right);
            Node* izq = (Node*)bin->left;
            Node* der = (Node*)bin->right;

            // Constant folding
            if (izq->tipo == NODE_NUMBER && der->tipo == NODE_NUMBER) {
    const char* lex_izq = ((LiteralNode*)bin->left)->lex;
    const char* lex_der = ((LiteralNode*)bin->right)->lex;

    int is_float = strchr(lex_izq, '.') || strchr(lex_der, '.');

    if (is_float) {
        double val_izq = atof(lex_izq);
        double val_der = atof(lex_der);
        double resultado;

        switch (tipo) {
            case NODE_PLUS: resultado = val_izq + val_der; break;
            case NODE_MINUS: resultado = val_izq - val_der; break;
            case NODE_MULT: resultado = val_izq * val_der; break;
            case NODE_DIV:
                if (val_der != 0.0) resultado = val_izq / val_der;
                else return expr;
                break;
            default: return expr;
        }

        char* buf = malloc(32);
        snprintf(buf, 32, "%.6f", resultado);
        NumberNode* reemplazo = calloc(1, sizeof(NumberNode));
        reemplazo->base.base.base.base.tipo = NODE_NUMBER;
        reemplazo->base.lex = buf;
        return (ExpressionNode*)reemplazo;
    } else {
        int val_izq = atoi(lex_izq);
        int val_der = atoi(lex_der);
        int resultado;

        switch (tipo) {
            case NODE_PLUS: resultado = val_izq + val_der; break;
            case NODE_MINUS: resultado = val_izq - val_der; break;
            case NODE_MULT: resultado = val_izq * val_der; break;
            case NODE_DIV:
                if (val_der != 0) resultado = val_izq / val_der;
                else return expr;
                break;
            case NODE_MOD:
                if (val_der != 0) resultado = val_izq % val_der;
                else return expr;
                break;
            default: return expr;
        }

        char* buf = malloc(16);
        snprintf(buf, 16, "%d", resultado);
        NumberNode* reemplazo = calloc(1, sizeof(NumberNode));
        reemplazo->base.base.base.base.tipo = NODE_NUMBER;
        reemplazo->base.lex = buf;
        return (ExpressionNode*)reemplazo;
    }
}

            // Simplificación algebraica con constantes
            if (der->tipo == NODE_NUMBER) {
                int v = atoi(((LiteralNode*)bin->right)->lex);
                if ((tipo == NODE_PLUS || tipo == NODE_MINUS) && v == 0)
                    return bin->left;
                if (tipo == NODE_MULT && v == 1)
                    return bin->left;
                if (tipo == NODE_MULT && v == 0)
                    return crear_nodo_constante("0");
                if (tipo == NODE_DIV && v == 1)
                    return bin->left;
            }

            if (izq->tipo == NODE_NUMBER) {
                int v = atoi(((LiteralNode*)bin->left)->lex);
                if (tipo == NODE_PLUS&& v == 0)
                    return bin->right;
                if (tipo == NODE_MULT && v == 1)
                    return bin->right;
                if (tipo == NODE_MULT && v == 0)
                    return crear_nodo_constante("0");
            }

            return expr;
        }

        // OPT: Bloques, LET y RETURN
        case NODE_EXPRESSION_BLOCK: {
            ExpressionBlockNode* block = (ExpressionBlockNode*)expr;
            ExpressionNode** exprs = (ExpressionNode**)block->expressions;
            for (int i = 0; exprs && exprs[i]; i++)
                exprs[i] = optimizar_constantes(exprs[i]);
            return expr;
        }
   
        case NODE_LET_IN: {
            LetInNode* let = (LetInNode*)expr;
            VarDeclarationNode** decls = (VarDeclarationNode**)let->variables;
            for (int i = 0; decls && decls[i]; i++) {
                if (decls[i]->value)
                    decls[i]->value = optimizar_constantes(decls[i]->value);

                // Si es una constante numérica, registrarla
                if (decls[i]->value && ((Node*)decls[i]->value)->tipo == NODE_NUMBER) {
                    if (num_constantes < MAX_CONST_ENV) {
                        entorno_constantes[num_constantes].nombre = strdup(decls[i]->name);
                        entorno_constantes[num_constantes].valor = strdup(((LiteralNode*)decls[i]->value)->lex);
                        num_constantes++;
                    }
                }
            }
            let->body = optimizar_constantes(let->body);
            return expr;
        }

        case NODE_RETURN: {
            ReturnNode* ret = (ReturnNode*)expr;
            if (ret->expr)
                ret->expr = optimizar_constantes(ret->expr);
            return expr;
        }

        // OPT: Unarias (como NOT)
        case NODE_PRINT:
        case NODE_NOT: {
            UnaryNode* un = (UnaryNode*)expr;
            un->operand = optimizar_constantes(un->operand);
            return expr;
        }

        // OPT: IF y WHILE
        case NODE_CONDITIONAL: {
            ConditionalNode* cond = (ConditionalNode*)expr;
            ExpressionNode** conds = (ExpressionNode**)cond->conditions;
            ExpressionNode** exprs = (ExpressionNode**)cond->expressions;
            for (int i = 0; conds && conds[i]; i++)
                conds[i] = optimizar_constantes(conds[i]);
            for (int i = 0; exprs && exprs[i]; i++)
                exprs[i] = optimizar_constantes(exprs[i]);
            if (cond->default_expre)
                cond->default_expre = optimizar_constantes(cond->default_expre);
            return expr;
        }
        case NODE_WHILE: {
            WhileNode* w = (WhileNode*)expr;
           w->condition = optimizar_constantes(w->condition);
            w->body = optimizar_constantes(w->body);
            return expr;
        }
        

        default:
            return expr;
    }
}

