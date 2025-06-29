#ifndef PRINT_AST_H
#define PRINT_AST_H

#include "../ast_nodes/ast_nodes.h"

void print_program(ProgramNode* prog);
void print_declaration(DeclarationNode* decl, int indent);
void print_expression(ExpressionNode* expr, int indent);

#endif
