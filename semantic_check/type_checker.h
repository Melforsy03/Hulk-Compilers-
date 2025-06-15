#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "semantic_errors.h"
#include "semantic.h"
#include "../parser/ast_nodes.h"



// Estructura del Type Checker
typedef struct TypeChecker {
    Context* context;
    Type* current_type;
    Method* current_method;
    HulkErrorList* errors;
} TypeChecker;

void type_check_program(ProgramNode* ast, Context* context, HulkErrorList* output_errors);
// Funciones auxiliares
TypeChecker* create_type_checker(Context* context);
void add_error(TypeChecker* tc, const char* message, int row, int column);
bool is_error_type(Type* type);
bool is_auto_type(Type* type);
Type* assign_type(TypeChecker* tc, Type* var_type, Type* expr_type, int row, int col);
bool conforms_to(Type* type, Type* other);

// Funciones visitantes para cada tipo de nodo
void* visit_program(TypeChecker* tc, ProgramNode* node, Scope* scope); // OK
//Declarations
void* visit_type_declaration(TypeChecker* tc, TypeDeclarationNode* node, Scope* scope); // OK
void* visit_type_attribute(TypeChecker* tc, TypeAttributeNode* node, Scope* scope); //OK
void* visit_method_declaration(TypeChecker* tc, MethodDeclarationNode* node, Scope* scope); // OK
void* visit_function_declaration(TypeChecker* tc, FunctionDeclarationNode* node, Scope* scope); // OK
void* visit_var_declaration(TypeChecker* tc, VarDeclarationNode* node, Scope* scope); //OK
//Expressions
void* visit_conditional(TypeChecker* tc, ConditionalNode* node, Scope* scope); //OK
void* visit_let_in(TypeChecker* tc, LetInNode* node, Scope* scope);//OK
void* visit_while(TypeChecker* tc, WhileNode* node, Scope* scope);//Ok
void* visit_for(TypeChecker* tc, ForNode* node, Scope* scope);//Ok
void* visit_destr(TypeChecker* tc, DestrNode* node, Scope* scope);//ok
//Binary
void* visit_equality_binary(TypeChecker* tc, EqualityBinaryNode* node, Scope* scope);//ok
void* visit_comparison_binary(TypeChecker* tc, ComparisonBinaryNode* node, Scope* scope);//ok
void* visit_arithmetic_binary(TypeChecker* tc, ArithmeticBinaryNode* node, Scope* scope);//ok
void* visit_boolean_binary(TypeChecker* tc, BooleanBinaryNode* node, Scope* scope); //Ok
void* visit_check_type(TypeChecker* tc, CheckTypeNode* node, Scope* scope);//ok
void* visit_string_binary(TypeChecker* tc, StringBinaryNode* node, Scope* scope);//ok

//Atomic
void* visit_expression_block(TypeChecker* tc, ExpressionBlockNode* node, Scope* scope);//ok
void* visit_call_func(TypeChecker* tc, CallFuncNode* node, Scope* scope);//ok
void* visit_type_instantiation(TypeChecker* tc, TypeInstantiationNode* node, Scope* scope); //ok
void* visit_explicit_vector(TypeChecker* tc, ExplicitVectorNode* node, Scope* scope);//ok
void* visit_implicit_vector(TypeChecker* tc, ImplicitVectorNode* node, Scope* scope);//ok
void* visit_index_object(TypeChecker* tc, IndexObjectNode* node, Scope* scope);//ok
void* visit_call_method(TypeChecker* tc, CallMethodNode* node, Scope* scope);//ok
void* visit_call_type_attribute(TypeChecker* tc, CallTypeAttributeNode* node, Scope* scope);//ok
void* visit_cast_type(TypeChecker* tc, CastTypeNode* node, Scope* scope);//ok

//Unary
void* visit_arithmetic_unary(TypeChecker* tc, ArithmeticUnaryNode* node, Scope* scope); //ok
void* visit_boolean_unary(TypeChecker* tc, BooleanUnaryNode* node, Scope* scope);//ok
//Literals
Type* visit_boolean_node(TypeChecker* tc, BooleanNode* node, Scope* scope);
Type* visit_string_node(TypeChecker* tc, StringNode* node, Scope* scope);
Type* visit_number_node(TypeChecker* tc, NumberNode* node, Scope* scope);
Type* visit_var_node(TypeChecker* tc, VarNode* node, Scope* scope);

//================Dispatcher================================
Type* tc_visit(TypeChecker* tc, Node* node, Scope* scope); //OK


