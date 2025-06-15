#ifndef AST_BUILDER_H
#define AST_BUILDER_H

#include "ast_nodes.h"
#include "../parser/parser.h"   
#include "../lexer/lexer.h"  
   
Node* build_ast_node(Production* p, Node** children);

// Nodos principales
ProgramNode* ast_make_program(DeclarationNode** decls, int decl_count, ExpressionNode* expr, int row, int col);

// Declaraciones
MethodSignatureNode* ast_make_method_signature(const char* name, DeclarationNode** params, int param_count,
                                            const char* returnType, int row, int col);
VarDeclarationNode* ast_make_var_decl(const char* name, ExpressionNode* init, const char* type, int row, int col);
FunctionDeclarationNode* ast_make_function(const char* name, DeclarationNode** params, int param_count, 
                                         ExpressionNode* body, const char* returnType, int row, int col);
MethodDeclarationNode* ast_make_method(const char* name, DeclarationNode** params, int param_count,
                                     ExpressionNode* body, const char* returnType, int row, int col);
TypeDeclarationNode* ast_make_type(const char* name, DeclarationNode** params, int param_count,
                                  const char* parent, ExpressionNode** parent_args, int parent_args_count,
                                  TypeAttributeNode** attributes, int attr_count,
                                  MethodDeclarationNode** methods, int method_count, int row, int col);
TypeAttributeNode* ast_make_type_attribute(const char* name, ExpressionNode* value, const char* type, int row, int col);
ProtocolDeclarationNode* ast_make_protocol(const char* name, MethodSignatureNode** methods, 
                                         int method_count, const char* parent, int row, int col);
TypeConstructorSignatureNode* ast_make_type_constructor_signature(const char* name, 
    DeclarationNode** params, int param_count, int row, int col);


// Expresiones
ConditionalNode* ast_make_conditional(ExpressionNode** conditions, ExpressionNode** expressions, 
                                    int condition_count, ExpressionNode* default_expr, int row, int col);
WhileNode* ast_make_while(ExpressionNode* condition, ExpressionNode* body, int row, int col);
ForNode* ast_make_for(const char* var_name, ExpressionNode* iterable, ExpressionNode* body, int row, int col);
DestrNode* ast_make_destr(ExpressionNode* var, ExpressionNode* expr, int row, int col);
ReturnNode* ast_make_return(ExpressionNode* expr, int row, int col);
BinaryNode* ast_make_binary(NodeType kind, ExpressionNode* left, ExpressionNode* right, const char* op, int row, int col);
UnaryNode* ast_make_unary(NodeType kind, char* op, ExpressionNode* operand, int row, int col);
LetInNode* ast_make_let_in(VarDeclarationNode** declarations, int decl_count, ExpressionNode* body, int row, int col);

// Operadores binarios específicos
BooleanBinaryNode* ast_make_boolean_binary(NodeType kind, const char* op, ExpressionNode* left, ExpressionNode* right, int row, int col);
ComparisonBinaryNode* ast_make_comparison_binary(NodeType kind, const char* op, ExpressionNode* left, ExpressionNode* right, int row, int col);
EqualityBinaryNode* ast_make_equality_binary(NodeType kind, const char* op, ExpressionNode* left, ExpressionNode* right, int row, int col);
StringBinaryNode* ast_make_string_binary(NodeType kind, const char* op, ExpressionNode* left, ExpressionNode* right, int row, int col);
ArithmeticBinaryNode* ast_make_arithmetic_binary(NodeType kind, const char* op, ExpressionNode* left, ExpressionNode* right, int row, int col);
CheckTypeNode* ast_make_check_type(NodeType kind, ExpressionNode* left, ExpressionNode* right, int row, int col);
// Operadores unarios específicos
ArithmeticUnaryNode* ast_make_arithmetic_unary(NodeType kind, const char* op, ExpressionNode* operand, int row, int col);
BooleanUnaryNode* ast_make_boolean_unary(NodeType kind, const char* op, ExpressionNode* operand, int row, int col);

// Nodos atómicos
LiteralNode* ast_make_literal(NodeType lit_kind, const char* lexeme, int row, int col);
CallFuncNode* ast_make_call_func(const char* name, ExpressionNode** args, int arg_count, int row, int col);
TypeInstantiationNode* ast_make_type_instantiation(const char* name, ExpressionNode** args, int arg_count, int row, int col);
ExplicitVectorNode* ast_make_explicit_vector(ExpressionNode** items, int item_count, int row, int col);
ImplicitVectorNode* ast_make_implicit_vector(const char* item, ExpressionNode* iterable, ExpressionNode* expr, int row, int col);
IndexObjectNode* ast_make_index_object(ExpressionNode* object, ExpressionNode* pos, int row, int col);
CallMethodNode* ast_make_call_method(ExpressionNode* inst, const char* method_name, 
                                   ExpressionNode** args, int arg_count, int row, int col);
CallTypeAttributeNode* ast_make_call_type_attribute(ExpressionNode* inst, const char* attribute, int row, int col);
CastTypeNode* ast_make_cast_type(ExpressionNode* inst, const char* type_cast, int row, int col);
ExpressionBlockNode* ast_make_expression_block(ExpressionNode** exprs, int expr_count, int row, int col);

/*El resto de gente q faltan aqui q son los de depth 4 basta conque les hagas lo siguiente
por ejemplo si fueras a crear un StringNode, eso es depth4, lo q significa que su definicion es simplemente esto 

typedef struct StringNode {
    LiteralNode base;
} StringNode;

o sea solo tendrias q crear un LiteralNode usando este metodo
LiteralNode* ast_make_literal(NodeType lit_kind, const char* lexeme, int row, int col);
y donde dice NodeType lit_lind le pasarias NODE_STRING y ya

*/
#endif