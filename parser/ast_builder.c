#include "ast_builder.h"
#include "../parser/lr1_table.h"
#include "../grammar/grammar.h"
#include <stdlib.h>
#include <string.h>

// ----- Fábricas de nodos -----
ProgramNode* ast_make_program(DeclarationNode** decls, int decl_count, ExpressionNode* expr, int row, int col){
    
    ProgramNode* n = malloc(sizeof *n);
    n->base.row    = row;
    n->base.column = col;
    n->base.tipo   = NODE_PROGRAM;       
    DeclarationNode** list = malloc((decl_count + 1) * sizeof *list);
    for (int i = 0; i < decl_count; ++i) {
        list[i] = decls[i];
    }
    list[decl_count] = NULL;

    n->declarations = list;
    n->expression   = expr;              // expr es ExpressionNode*

    return n;
}


FunctionDeclarationNode* ast_make_function(const char* name, DeclarationNode** params, int param_count, ExpressionNode* body, const char* returnType, int row, int col)
{
    FunctionDeclarationNode* n = malloc(sizeof *n);

    n->base.base.row    = row;
    n->base.base.column = col;
    n->base.base.tipo   = NODE_FUNCTION_DECLARATION;

    n->name = strdup(name);
    DeclarationNode** list = malloc((param_count + 1) * sizeof *list);
    for (int i = 0; i < param_count; ++i) {
        list[i] = params[i];
    }
    list[param_count] = NULL;

    n->params          = list;
    n->param_counter   = param_count;
    n->body            = body;
    n->returnType      = returnType ? strdup(returnType) : NULL;

    return n;
}

BinaryNode* ast_make_binary(NodeType kind, ExpressionNode* left, ExpressionNode* right, const char* op, int row, int col) {
    BinaryNode* n = malloc(sizeof(BinaryNode));
    n->base.base.row    = row;
    n->base.base.column = col;
    n->base.base.tipo   = kind;
    n->left             = left;
    n->right            = right;
    n->operator         = op ? strdup(op) : NULL;
    return n;
}

UnaryNode* ast_make_unary(NodeType kind, char* op, ExpressionNode* operand, int row, int col) {
    UnaryNode* n = malloc(sizeof(UnaryNode));
    n->base.base.row = row;
    n->base.base.column = col;
    n->base.base.tipo = kind;
    n->operand = operand;
    n->operator = op;
    return n;
}


LetInNode* ast_make_let_in(VarDeclarationNode** declarations, int decl_count, ExpressionNode* body, int row, int col) {
    LetInNode* node = malloc(sizeof(LetInNode));
    node->base.base.row = row;
    node->base.base.column = col;
    node->base.base.symbol = create_symbol("LetExpr", NON_TERMINAL);
    node->base.base.lexeme = "let";
    node->base.base.tipo = NODE_LET_IN;
    
    // Copiar array de declaraciones
    VarDeclarationNode** n = malloc((decl_count + 1) * sizeof(VarDeclarationNode*));
    for (int i = 0; i < decl_count; i++) {
        n[i] = declarations[i];
    }
    n[decl_count] = NULL;
    node->variable_counter = decl_count;
    
    node->variables = n;
    node->body = body;
    return node;
}

VarDeclarationNode* ast_make_var_decl(const char* name, ExpressionNode* init, const char* type, int row, int col) {
    VarDeclarationNode* node = malloc(sizeof(VarDeclarationNode));
    node->base.base.row = row;
    node->base.base.column = col;
    node->base.base.symbol = create_symbol("VarDecl", NON_TERMINAL);
    node->base.base.lexeme = strdup(name);
    node->base.base.tipo = NODE_VAR_DECLARATION;

    node->name = strdup(name);
    node->value = init;
    node->type = type ? strdup(type) : NULL;
    return node;
}


ConditionalNode* ast_make_conditional(ExpressionNode** conditions,  ExpressionNode** expressions, int condition_count, ExpressionNode* default_expr, int row, int col) {
    ConditionalNode* node = malloc(sizeof(ConditionalNode));
    node->base.base.row = row;
    node->base.base.column = col;
    node->base.base.symbol = create_symbol("If", NON_TERMINAL);
    node->base.base.lexeme = "if";
    node->base.base.tipo = NODE_CONDITIONAL;

    // Copiar arrays de condiciones y expresiones
    ExpressionNode** list = malloc(condition_count * sizeof(ExpressionNode*));
    ExpressionNode** expr = malloc(condition_count * sizeof(ExpressionNode*));
    
     
    for (int i = 0; i < condition_count; i++) {
        list[i] = conditions[i];
        expr[i] = expressions[i];
    }
    
    node->conditions = list;
    node->expressions = expr;
    node->condition_counter = condition_count;
    node->expression_counter = condition_count;
    node->default_expre = default_expr;
    return node;
}

WhileNode* ast_make_while(ExpressionNode* condition, ExpressionNode* body, int row, int col) {
    WhileNode* node = malloc(sizeof(WhileNode));
    node->base.base.row = row;
    node->base.base.column = col;
    node->base.base.symbol = create_symbol("While", NON_TERMINAL);
    node->base.base.lexeme = "while";
    node->base.base.tipo = NODE_POW;

    node->condition = condition;
    node->body = body;
    return node;
}

ForNode* ast_make_for(const char* var_name, ExpressionNode* iterable, ExpressionNode* body, int row, int col) {
    ForNode* node = malloc(sizeof(ForNode));
    node->base.base.row = row;
    node->base.base.column = col;
    node->base.base.symbol = create_symbol("For", NON_TERMINAL);
    node->base.base.lexeme = "for";
    node->base.base.tipo = NODE_FOR;

    node->item = strdup(var_name);
    node->iterable = iterable;
    node->body = body;
    return node;
}

LiteralNode* ast_make_literal(NodeType lit_kind, const char* lexeme, int row, int col) {
    LiteralNode* n = malloc(sizeof(LiteralNode));
    n->base.base.base.row    = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo   = lit_kind;
    n->base.base.base.lexeme = strdup(lexeme);
    n->lex              = strdup(lexeme);
    return n;
}

//---------------------------------------------------------------
MethodSignatureNode* ast_make_method_signature(const char* name, DeclarationNode** params, int param_count,
                                            const char* returnType, int row, int col){
    MethodSignatureNode* n = malloc(sizeof *n);
    n->base.base.row = row;
    n->base.base.column = col;
    n->base.base.tipo = NODE_METHOD_DECLARATION;
    n->base.base.lexeme = strdup(name);

    n->name = strdup(name);
    DeclarationNode** list = malloc((param_count + 1) * sizeof *list);
    for (int i = 0; i < param_count; ++i) {
        list[i] = params[i];
    }
    list[param_count] = NULL;
    
    n->params = list;
    n->param_counter = param_count;
    n->returnType = returnType ? strdup(returnType) : NULL;
    
    return n;
}


MethodDeclarationNode* ast_make_method(const char* name, DeclarationNode** params, int param_count, 
                                      ExpressionNode* body, const char* returnType, int row, int col) {
    MethodDeclarationNode* n = malloc(sizeof *n);
    n->base.base.row = row;
    n->base.base.column = col;
    n->base.base.tipo = NODE_METHOD_DECLARATION;
    n->base.base.lexeme = strdup(name);

    n->name = strdup(name);
    DeclarationNode** list = malloc((param_count + 1) * sizeof *list);
    for (int i = 0; i < param_count; ++i) {
        list[i] = params[i];
    }
    list[param_count] = NULL;
    
    n->params = list;
    n->param_counter = param_count;
    n->body = body;
    n->returnType = returnType ? strdup(returnType) : NULL;
    
    return n;
}

TypeDeclarationNode* ast_make_type(const char* name, DeclarationNode** params, int param_count,
                                  const char* parent, ExpressionNode** parent_args, int parent_args_count,
                                  TypeAttributeNode** attributes, int attr_count,
                                  MethodDeclarationNode** methods, int method_count, int row, int col) {
    TypeDeclarationNode* n = malloc(sizeof *n);
    n->base.base.row = row;
    n->base.base.column = col;
    n->base.base.tipo = NODE_TYPE_DECLARATION;
    n->base.base.lexeme = strdup(name);
    
    n->name = strdup(name);
    
    // Parámetros
    DeclarationNode** param_list = malloc((param_count + 1) * sizeof *param_list);
    for (int i = 0; i < param_count; ++i) param_list[i] = params[i];
    param_list[param_count] = NULL;
    n->params = param_list;
    n->param_count = param_count;
    
    // Herencia
    n->parent = parent ? strdup(parent) : strdup("Object");
    
    // Argumentos del padre
    ExpressionNode** parent_args_list = malloc((parent_args_count + 1) * sizeof *parent_args_list);
    for (int i = 0; i < parent_args_count; ++i) parent_args_list[i] = parent_args[i];
    parent_args_list[parent_args_count] = NULL;
    n->parent_args = parent_args_list;
    n->parent_args_count = parent_args_count;
    
    // Atributos
    TypeAttributeNode** attr_list = malloc((attr_count + 1) * sizeof *attr_list);
    for (int i = 0; i < attr_count; ++i) attr_list[i] = attributes[i];
    attr_list[attr_count] = NULL;
    n->attributes = attr_list;
    n->attribute_counter = attr_count;
    
    // Métodos
    MethodDeclarationNode** method_list = malloc((method_count + 1) * sizeof *method_list);
    for (int i = 0; i < method_count; ++i) method_list[i] = methods[i];
    method_list[method_count] = NULL;
    n->methods = method_list;
    n->method_counter = method_count;
    
    return n;
}

TypeAttributeNode* ast_make_type_attribute(const char* name, ExpressionNode* value, const char* type, int row, int col) {
    TypeAttributeNode* n = malloc(sizeof *n);
    n->base.base.row = row;
    n->base.base.column = col;
    n->base.base.lexeme = strdup(name);
    n->base.base.tipo = NODE_TYPE_ATTRIBUTE;
    
    n->name = strdup(name);
    n->value = value;
    n->type = type ? strdup(type) : NULL;
    
    return n;
}


ProtocolDeclarationNode* ast_make_protocol(const char* name, MethodSignatureNode** methods, 
                                          int method_count, const char* parent, int row, int col) {
    ProtocolDeclarationNode* n = malloc(sizeof *n);
    n->base.base.row = row;
    n->base.base.column = col;
    n->base.base.lexeme = strdup(name);
    n->base.base.tipo = NODE_PROTOCOL_DECLARATION;
    
    n->name = strdup(name);
    
    MethodSignatureNode** method_list = malloc((method_count + 1) * sizeof *method_list);
    for (int i = 0; i < method_count; ++i) method_list[i] = methods[i];
    method_list[method_count] = NULL;
    
    n->methods_signature = method_list;
    n->method_signature_counter = method_count;
    n->parent = parent ? strdup(parent) : NULL;
    
    return n;
}

CallFuncNode* ast_make_call_func(const char* name, ExpressionNode** args, int arg_count, int row, int col) {
    CallFuncNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.lexeme = strdup(name);
    n->base.base.base.tipo = NODE_CALL_FUNC;
    
    n->name = strdup(name);
    
    ExpressionNode** arg_list = malloc((arg_count + 1) * sizeof *arg_list);
    for (int i = 0; i < arg_count; ++i) arg_list[i] = args[i];
    arg_list[arg_count] = NULL;
    
    n->arguments = arg_list;
    n->arguments_counter = arg_count;
    
    return n;
}

TypeInstantiationNode* ast_make_type_instantiation(const char* name, ExpressionNode** args, int arg_count, int row, int col) {
    TypeInstantiationNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.lexeme = strdup(name);
    n->base.base.base.tipo = NODE_TYPE_INSTANTIATION;
    
    n->name = strdup(name);
    
    ExpressionNode** arg_list = malloc((arg_count + 1) * sizeof *arg_list);
    for (int i = 0; i < arg_count; ++i) arg_list[i] = args[i];
    arg_list[arg_count] = NULL;
    
    n->arguments = arg_list;
    n->arguments_counter = arg_count;
    
    return n;
}

ExplicitVectorNode* ast_make_explicit_vector(ExpressionNode** items, int item_count, int row, int col) {
    ExplicitVectorNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo = NODE_EXPLICIT_VECTOR;
    
    ExpressionNode** item_list = malloc((item_count + 1) * sizeof *item_list);
    for (int i = 0; i < item_count; ++i) item_list[i] = items[i];
    item_list[item_count] = NULL;
    
    n->items = item_list;
    n->item_counter = item_count;
    
    return n;
}

ImplicitVectorNode* ast_make_implicit_vector(const char* item, ExpressionNode* iterable, ExpressionNode* expr, int row, int col) {
    ImplicitVectorNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo = NODE_IMPLICIT_VECTOR;
    
    n->item = strdup(item);
    n->iterable = iterable;
    n->expr = expr;
    
    return n;
}

IndexObjectNode* ast_make_index_object(ExpressionNode* object, ExpressionNode* pos, int row, int col) {
    IndexObjectNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo = NODE_INDEX_OBJECT;
    
    n->object = object;
    n->pos = pos;
    
    return n;
}

CallMethodNode* ast_make_call_method(ExpressionNode* inst, const char* method_name, 
                                    ExpressionNode** args, int arg_count, int row, int col) {
    CallMethodNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo = NODE_CALL_METHOD;
    
    n->inst = inst;
    n->inst_name = strdup(inst->base.lexeme); // Asumiendo que el lexeme del nodo inst tiene el nombre
    n->method_name = strdup(method_name);
    
    ExpressionNode** arg_list = malloc((arg_count + 1) * sizeof *arg_list);
    for (int i = 0; i < arg_count; ++i) arg_list[i] = args[i];
    arg_list[arg_count] = NULL;
    
    n->method_args = arg_list;
    n->method_args_counter = arg_count;
    
    return n;
}

CallTypeAttributeNode* ast_make_call_type_attribute(ExpressionNode* inst, const char* attribute, int row, int col) {
    CallTypeAttributeNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo = NODE_CALL_TYPE_ATTRIBUTE;
    
    n->inst = inst;
    n->inst_name = strdup(inst->base.lexeme); // Asumiendo que el lexeme del nodo inst tiene el nombre
    n->attribute = strdup(attribute);
    
    return n;
}

CastTypeNode* ast_make_cast_type(ExpressionNode* inst, const char* type_cast, int row, int col) {
    CastTypeNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo = NODE_CAST_TYPE;
    
    n->inst = inst;
    n->inst_name = strdup(inst->base.lexeme);
    n->type_cast = strdup(type_cast);
    
    return n;
}

ReturnNode* ast_make_return(ExpressionNode* expr, int row, int col) {
    ReturnNode* n = malloc(sizeof *n);
    n->base.row = row;
    n->base.column = col;
    n->base.tipo = NODE_RETURN;
    n->expr = expr;
    return n;
}

ExpressionBlockNode* ast_make_expression_block(ExpressionNode** exprs, int expr_count, int row, int col) {
    ExpressionBlockNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo = NODE_EXPRESSION_BLOCK;
    
    ExpressionNode** expr_list = malloc((expr_count + 1) * sizeof *expr_list);
    for (int i = 0; i < expr_count; ++i) expr_list[i] = exprs[i];
    expr_list[expr_count] = NULL;
    
    n->expressions = expr_list;
    n->expression_counter = expr_count;
    
    return n;
}

DestrNode* ast_make_destr(ExpressionNode* var, ExpressionNode* expr, int row, int col){
    DestrNode* n = malloc(sizeof *n);
    n->base.base.row = row;
    n->base.base.column = col;
    n->base.base.tipo = NODE_DESTRUCTURING;

    n->expr = expr;
    n->var = var;

    return n;
}

BooleanBinaryNode* ast_make_boolean_binary(NodeType kind, char* op, ExpressionNode* left, ExpressionNode* right, int row, int col){
    BooleanBinaryNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo = kind;
    n->base.left = left;
    n->base.right = right;
    n->base.operator = op ? strdup(op) : NULL; //puede ser null
}

ComparisonBinaryNode* ast_make_comparison_binary(NodeType kind, char* op, ExpressionNode* left, ExpressionNode* right, int row, int col){
    ComparisonBinaryNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo = kind;
    n->base.left = left;
    n->base.right = right;
    n->base.operator = op ? strdup(op) : NULL; //puede ser null
}

EqualityBinaryNode* ast_make_equality_binary(NodeType kind, char* op, ExpressionNode* left, ExpressionNode* right, int row, int col){
    EqualityBinaryNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo = kind;
    n->base.left = left;
    n->base.right = right;
    n->base.operator = op ? strdup(op) : NULL; //puede ser null
}

StringBinaryNode* ast_make_string_binary(NodeType kind, char* op, ExpressionNode* left, ExpressionNode* right, int row, int col){
    StringBinaryNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo = kind;
    n->base.left = left;
    n->base.right = right;
    n->base.operator = op ? strdup(op) : NULL; //puede ser null
}

ArithmeticBinaryNode* ast_make_arithmetic_binary(NodeType kind, char* op, ExpressionNode* left, ExpressionNode* right, int row, int col){
    ArithmeticBinaryNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo = kind;
    n->base.left = left;
    n->base.right = right;
    n->base.operator = op ? strdup(op) : NULL; //puede ser null
}

ArithmeticUnaryNode* ast_make_arithmetic_unary(NodeType kind, char* op, ExpressionNode* operand, int row, int col){
    ArithmeticUnaryNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo = kind;
    n->base.operand = operand;
    n->base.operator = op ? strdup(op) : NULL;
}

BooleanUnaryNode* ast_make_boolean_unary(NodeType kind, char* op, ExpressionNode* operand, int row, int col){
    BooleanUnaryNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo = kind;
    n->base.operand = operand;
    n->base.operator = op ? strdup(op) : NULL;
}


// // ----- Conexión lexer → parser → AST tipado -----
// Node* build_ast(TokenList* tokens) {
//     int tok_count = token_list_length(tokens);
//     Symbol** input = tokens_to_symbols(tokens);

//     ActionEntryLR1* actions;
//     int action_count;
//     // parser devuelve un nodo tipado (ProgramNode*)
//     Node* root = parser(&lr1_table, input, tok_count, &actions, &action_count);
//     return root;
// }