#include "../parser/parser.h"
#include "ast_build.h"
#include "../grammar/symbol.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


// Función para determinar el tipo de nodo basado en el operador
NodeType get_operator_node_type(const char* op_name) {
    // Operadores aritméticos
    if (strcmp(op_name, "PLUS") == 0) return NODE_PLUS;
    if (strcmp(op_name, "MINUS") == 0) return NODE_MINUS;
    if (strcmp(op_name, "STAR") == 0) return NODE_MULT;
    if (strcmp(op_name, "SLASH") == 0) return NODE_DIV;
    if (strcmp(op_name, "MODULO") == 0) return NODE_MOD;
    if (strcmp(op_name, "POWER") == 0 || strcmp(op_name, "DSTAR") == 0) return NODE_POW;
    
    // Operadores booleanos
    if (strcmp(op_name, "OR") == 0 || strcmp(op_name, "||") == 0) return NODE_OR;
    if (strcmp(op_name, "AND") == 0 || strcmp(op_name, "&&") == 0) return NODE_AND;
    
    // Operadores de comparación
    if (strcmp(op_name, "EQUAL_EQUAL") == 0) return NODE_EQUAL;
    if (strcmp(op_name, "NOT_EQUAL") == 0) return NODE_NOT_EQUAL;
    if (strcmp(op_name, "GREATER") == 0) return NODE_GREATER;
    if (strcmp(op_name, "GREATER_EQUAL") == 0) return NODE_GREATER_EQUAL;
    if (strcmp(op_name, "LESS") == 0) return NODE_LESS;
    if (strcmp(op_name, "LESS_EQUAL") == 0) return NODE_LESS_EQUAL;
    if (strcmp(op_name, "NOT") == 0) return NODE_NOT;
    
    // Tipos literales
    if (strcmp(op_name, "NUMBER") == 0) return NODE_NUMBER;
    if (strcmp(op_name, "IDENTIFIER") == 0) return NODE_VAR;
    if (strcmp(op_name, "TRUE") == 0 || strcmp(op_name, "FALSE") == 0) return NODE_BOOLEAN;
    if (strcmp(op_name, "STRING") == 0) return NODE_STRING;
    
    return NODE_ATOMIC; // Tipo genérico
}

// Conditional (if-else)
// Node* build_conditional(Production* p, Node** children) {
//     if (!children || !children[2] || !children[5]) {
//         fprintf(stderr, "Error: Nodos hijos nulos en condicional\n");
//         return NULL;
//     }
    

//     ExpressionNode* condition = (ExpressionNode*)children[2];
//     ExpressionNode* then_expr = (ExpressionNode*)children[5];
//     ExpressionNode* else_expr = NULL;
//     int condition_count = 1;

//     // Handle elif/else chains
//     if (children[6] && children[6]->tipo == NODE_CONDITIONAL) {
//         ConditionalNode* nested = (ConditionalNode*)children[6];
//         condition_count += nested->condition_counter;
        
//         ExpressionNode** conditions = malloc(condition_count * sizeof(ExpressionNode*));
//         ExpressionNode** expressions = malloc(condition_count * sizeof(ExpressionNode*));
        
//         conditions[0] = condition;
//         expressions[0] = then_expr;
        
//         for (int i = 0; i < nested->condition_counter-1; i++) {
//             conditions[i+1] = (ExpressionNode*)nested->conditions[i];
//             expressions[i+1] = (ExpressionNode*)nested->expressions[i];
//         }
        
//         else_expr = nested->default_expre;
//         free(nested->conditions);
//         free(nested->expressions);
//         free(nested);
        
//         return (Node*)ast_make_conditional(conditions, expressions, condition_count, else_expr, 0, 0);
//     } else if (children[6]) {
//         else_expr = (ExpressionNode*)children[6];
//     }

//     ExpressionNode** conditions = malloc(sizeof(ExpressionNode*));
//     ExpressionNode** expressions = malloc(sizeof(ExpressionNode*));
//     conditions[0] = condition;
//     expressions[0] = then_expr;
    
//     return (Node*)ast_make_conditional(conditions, expressions, 1, else_expr, 0, 0);
// }

// While loop
Node* build_while_loop(Production* p, Node** children) {
    return (Node*)ast_make_while(
        (ExpressionNode*)children[2],  // condition
        (ExpressionNode*)children[4],  // body
        0, 0
    );
}

// For loop
Node* build_for_loop(Production* p, Node** children) {
    return (Node*)ast_make_for(
        children[2]->lexeme,  // variable name
        (ExpressionNode*)children[4],  // iterable
        (ExpressionNode*)children[6],  // body
        0, 0
    );
}

// Función para construir nodos de operaciones binarias
Node* build_binary_operation(Production* p, Node** children, const char* operator_name) {
    NodeType type = get_operator_node_type(operator_name);
    
    printf("Construyendo operación binaria: %s con tipo %d\n", operator_name, type);

    switch(type) {
        case NODE_CONCAT:
            return (Node*)ast_make_string_binary(
                type, operator_name,
                (ExpressionNode*)children[0],
                (ExpressionNode*)children[2],
                0, 0
            );
            break;
        case NODE_DOUBLE_CONCAT:
            return (Node*)ast_make_string_binary(
                type, operator_name,
                (ExpressionNode*)children[0],
                (ExpressionNode*)children[2],
                0, 0
            );
            break;
        case NODE_PLUS:
            return (Node*)ast_make_arithmetic_binary(
                type, operator_name,
                (ExpressionNode*)children[0],
                (ExpressionNode*)children[2],
                0, 0
            );
        case NODE_MINUS:
            return (Node*)ast_make_arithmetic_binary(
                type, operator_name,
                (ExpressionNode*)children[0],
                (ExpressionNode*)children[2],
                0, 0
            );
        case NODE_MULT:
            return (Node*)ast_make_arithmetic_binary(
                type, operator_name,
                (ExpressionNode*)children[0],
                (ExpressionNode*)children[2],
                0, 0
            );
        case NODE_DIV:
            return (Node*)ast_make_arithmetic_binary(
                type, operator_name,
                (ExpressionNode*)children[0],
                (ExpressionNode*)children[2],
                0, 0
            );
        case NODE_MOD:
            return (Node*)ast_make_arithmetic_binary(
                type, operator_name,
                (ExpressionNode*)children[0],
                (ExpressionNode*)children[2],
                0, 0
            );
        case NODE_POW:
            return (Node*)ast_make_arithmetic_binary(
                type, operator_name,
                (ExpressionNode*)children[0],
                (ExpressionNode*)children[2],
                0, 0
            );
        
        case NODE_OR:
            return (Node*)ast_make_boolean_binary(
                type, operator_name,
                (ExpressionNode*)children[0],
                (ExpressionNode*)children[2],
                0, 0
            );
        case NODE_AND:
            return (Node*)ast_make_boolean_binary(
                type, operator_name,
                (ExpressionNode*)children[0],
                (ExpressionNode*)children[2],
                0, 0
            );
        
        case NODE_EQUAL:
            return (Node*)ast_make_equality_binary(
                type, operator_name,
                (ExpressionNode*)children[0],
                (ExpressionNode*)children[2],
                0, 0
            );
        case NODE_NOT_EQUAL:
            return (Node*)ast_make_equality_binary(
                type, operator_name,
                (ExpressionNode*)children[0],
                (ExpressionNode*)children[2],
                0, 0
            );
        
        case NODE_GREATER:
            return (Node*)ast_make_comparison_binary(
                type, operator_name,
                (ExpressionNode*)children[0],
                (ExpressionNode*)children[2],
                0, 0
            );
        case NODE_GREATER_EQUAL:
            return (Node*)ast_make_comparison_binary(
                type, operator_name,
                (ExpressionNode*)children[0],
                (ExpressionNode*)children[2],
                0, 0
            );
        case NODE_LESS:
            return (Node*)ast_make_comparison_binary(
                type, operator_name,
                (ExpressionNode*)children[0],
                (ExpressionNode*)children[2],
                0, 0
            );
        case NODE_LESS_EQUAL:
            return (Node*)ast_make_comparison_binary(
                type, operator_name,
                (ExpressionNode*)children[0],
                (ExpressionNode*)children[2],
                0, 0
            );
        
        default:
            return create_node(p->left, NULL, 3, children);
    }
}

// Función para construir nodos de operaciones unarias
Node* build_unary_operation(Production* p, Node** children, const char* operator_name) {
    NodeType type = get_operator_node_type(operator_name);
    
    if (type == NODE_NOT) {
        return (Node*)ast_make_boolean_unary(
            type, operator_name,
            (ExpressionNode*)children[1],
            0, 0
        );
    } else {
        // Para operadores + unario y - unario
        return (Node*)ast_make_arithmetic_unary(
            type == NODE_MINUS ? NODE_NEGATIVE : NODE_POSITIVE,
            operator_name,
            (ExpressionNode*)children[1],
            0, 0
        );
    }
}