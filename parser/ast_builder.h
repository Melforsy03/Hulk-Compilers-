#ifndef AST_BUILDER_H
#define AST_BUILDER_H

#include "ast_nodes.h"
#include "../parser/parser.h"   
#include "../lexer/lexer.h"     


ProgramNode* ast_make_program(DeclarationNode** decls, int decl_count, ExpressionNode* expr, int row, int col);
VarDeclarationNode* ast_make_var_decl(const char* name, ExpressionNode* init, const char* type,int row, int col);
FunctionDeclarationNode* ast_make_function(const char* name, DeclarationNode** params, int param_count, ExpressionNode* body, const char* returnType, int row, int col);
BinaryNode* ast_make_binary(NodeType kind, ExpressionNode* left, ExpressionNode* right,const char* op, int row, int col);
LiteralNode* ast_make_literal(NodeType lit_kind, const char* lexeme, int row, int col);

// ast_builder.h
LetInNode* ast_make_let_in(VarDeclarationNode** declarations, int decl_count, ExpressionNode* body, int row, int col);
ConditionalNode* ast_make_conditional(ExpressionNode** conditions, ExpressionNode** expressions, int condition_count, ExpressionNode* default_expr, int row, int col);
TypeDeclarationNode* ast_make_type_decl(const char* name, void* attributes, const char* parent, int row, int col);
CallFuncNode* ast_make_call_func(const char* name, void* arguments, int row, int col);

#endif