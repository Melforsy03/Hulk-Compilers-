#ifndef AST_H
#define AST_H

#include "../ast_nodes/ast_nodes.h"

// Constructores adaptados
NumberNode* nodo_numero(const char* valor);
PlusNode* nodo_suma(ExpressionNode* izq, ExpressionNode* der);
CallFuncNode* nodo_llamada_funcion(const char* nombre);
UnaryNode* nodo_print(ExpressionNode* valor);
FunctionDeclarationNode* nodo_funcion(const char* nombre, ExpressionNode* cuerpo);
ProgramNode* wrap_program(ExpressionNode* expr1, ExpressionNode* expr2);  // como block

#endif
