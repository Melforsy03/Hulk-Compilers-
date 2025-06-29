#ifndef PARSER_H
#define PARSER_H

#include "../lexer/lexer.h"
#include "../ast_nodes/ast_nodes.h"
#include <stdbool.h>
// ----------------------------
// Estado del lexer y parser
// ----------------------------
extern Token current_token;
extern Token lookahead_token;
extern const char* input_source;

// ----------------------------
// Inicialización
// ----------------------------
void init_parser(const char* source);
void advance();
Token peek_token();
bool lookahead_token_is_call();
void match(int token_type);
void syntax_error(const char* expected);

// ----------------------------
// Punto de entrada principal
// ----------------------------
ProgramNode* parse_program();

// ----------------------------
// Declarations
// ----------------------------
DeclarationNode** parse_type_function_list(int* count);
DeclarationNode* parse_func();
DeclarationNode* parse_type();
MethodDeclarationNode* parse_method();
void parse_params(TypeDeclarationNode* type_node);
void parse_parent_args(TypeDeclarationNode* type_node);
void parse_method_params(MethodDeclarationNode* method);

// ----------------------------
// Expr items y bloques
// ----------------------------
ExpressionNode* parse_expr_item_list();
ExpressionNode* parse_expr_block();
ExpressionNode* parse_expr();
ExpressionNode* parse_print_expr();
// ----------------------------
// Operaciones lógicas y aritméticas
// ----------------------------
ExpressionNode* parse_or_expr();
ExpressionNode* parse_and_expr();
ExpressionNode* parse_check_type();
ExpressionNode* parse_aritm_comp();
ExpressionNode* parse_concat();
ExpressionNode* parse_arithmetic();
ExpressionNode* parse_term();
ExpressionNode* parse_pow();
ExpressionNode* parse_sign();
ExpressionNode* parse_factor();
ExpressionNode* parse_atom();

// ----------------------------
// Condicionales y ciclos
// ----------------------------
ExpressionNode* parse_conditional();
ExpressionNode* parse_cond_other_case(ConditionalNode* cond_node);
ExpressionNode* parse_while_loop();
ExpressionNode* parse_for_loop();
ExpressionNode* parse_let_expr();
void parse_var_declaration_list(LetInNode* let_node);

// ----------------------------
// Funciones y llamadas
// ----------------------------
ExpressionNode* parse_call_func();
void parse_arguments(CallFuncNode* call_node);

// ----------------------------
// Postfix: indexación y acceso a miembro
// ----------------------------
ExpressionNode* parse_index_object(ExpressionNode* atom);
ExpressionNode* parse_vector();
ExpressionNode* parse_member(ExpressionNode* atom);

#endif

