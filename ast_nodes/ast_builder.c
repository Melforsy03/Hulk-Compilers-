#include "ast_builder.h"
#include "../parser/lr1_table.h"
#include "../grammar/grammar.h"
#include "../grammar/symbol.h"
#include "ast_nodes.h"
#include <stdlib.h>
#include <string.h>


void stack_push(StackNode**s,int st){
    StackNode*n = malloc(sizeof* n);
    n->state=st;
    n->next=*s;
    *s=n;
}

int stack_top(StackNode*s){
    return s ? s->state: -1;
}

void stack_pop(StackNode**s){
    if(!*s)return;
    StackNode*t = *s;
    *s = t->next;
    free(t);
}  

void clear_stack(StackNode* s){
    while(s){StackNode*t=s->next;free(s);s=t;}
}

TypedStack* create_typed_stack(int capacity) {
    TypedStack* s = malloc(sizeof(TypedStack));
    s->items = malloc(capacity * sizeof(TypedNode));
    s->types = malloc(capacity * sizeof(NodeType));
    s->top = -1;
    s->capacity = capacity;
    return s;
}

void typed_push(TypedStack* s, TypedNode node, NodeType type) {
    if (s->top == s->capacity - 1) {
        s->capacity *= 2;
        s->items = realloc(s->items, s->capacity * sizeof(TypedNode));
        s->types = realloc(s->types, s->capacity * sizeof(NodeType));
    }
    s->top++;
    s->items[s->top] = node;
    s->types[s->top] = type;
}

TypedNode typed_pop(TypedStack* s) {
    if (s->top == -1) {
        return (TypedNode){ .any = NULL };
    }
    return s->items[s->top--];
}

NodeType typed_peek_type(TypedStack* s) {
    if (s->top == -1) return -1;
    return s->types[s->top];
}

void free_typed_stack(TypedStack* s) {
    free(s->items);
    free(s->types);
    free(s);
}



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
    n->expression   = expr;              

    return n;
}

TypeConstructorSignatureNode* ast_make_type_constructor_signature(const char* name, 
    DeclarationNode** params, int param_count, int row, int col) {
    TypeConstructorSignatureNode* n = malloc(sizeof *n);
    n->base.base.row = row;
    n->base.base.column = col;
    n->base.base.tipo = NODE_TYPE_CONSTRUCTOR_SIGNATURE;
    n->base.base.lexeme = strdup(name);
    
    n->name = strdup(name);
    DeclarationNode** list = malloc((param_count + 1) * sizeof *list);
    for (int i = 0; i < param_count; ++i) {
        list[i] = params[i];
    }
    list[param_count] = NULL;
    
    n->params = list;
    n->param_counter = param_count;
    
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
    node->base.base.symbol= create_symbol("LetExpr", NON_TERMINAL);
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
    node->base.base.symbol= create_symbol("VarDecl", NON_TERMINAL);
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
    node->base.base.symbol= create_symbol("IF", NON_TERMINAL);
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
    node->base.base.symbol= create_symbol("While", NON_TERMINAL);
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
    node->base.base.symbol= create_symbol("For", NON_TERMINAL);
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

BooleanBinaryNode* ast_make_boolean_binary(NodeType kind, const char* op, ExpressionNode* left, ExpressionNode* right, int row, int col){
    BooleanBinaryNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo = kind;
    n->base.left = left;
    n->base.right = right;
    n->base.operator = op ? strdup(op) : NULL; //puede ser null
}

CheckTypeNode* ast_make_check_type(NodeType kind, ExpressionNode* left, ExpressionNode* right, int row, int col) {
    CheckTypeNode* n = malloc(sizeof *n);
    
    // Inicializar la base ExpressionNode
    n->base.base.row = row;
    n->base.base.column = col;
    n->base.base.tipo = kind;
    n->base.base.symbol = NULL;  // Puedes asignar un símbolo si es necesario
    n->base.base.lexeme = NULL;  // El lexema no es relevante para este nodo
    
    // Asignar los operandos
    n->left = left;
    n->right = right;
    
    return n;
}

ComparisonBinaryNode* ast_make_comparison_binary(NodeType kind, const char* op, ExpressionNode* left, ExpressionNode* right, int row, int col){
    ComparisonBinaryNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo = kind;
    n->base.left = left;
    n->base.right = right;
    n->base.operator = op ? strdup(op) : NULL; //puede ser null
}

EqualityBinaryNode* ast_make_equality_binary(NodeType kind, const char* op, ExpressionNode* left, ExpressionNode* right, int row, int col){
    EqualityBinaryNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo = kind;
    n->base.left = left;
    n->base.right = right;
    n->base.operator = op ? strdup(op) : NULL; //puede ser null
}

StringBinaryNode* ast_make_string_binary(NodeType kind, const char* op, ExpressionNode* left, ExpressionNode* right, int row, int col){
    StringBinaryNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo = kind;
    n->base.left = left;
    n->base.right = right;
    n->base.operator = op ? strdup(op) : NULL; //puede ser null
}

ArithmeticBinaryNode* ast_make_arithmetic_binary(NodeType kind, const char* op, ExpressionNode* left, ExpressionNode* right, int row, int col){
    ArithmeticBinaryNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo = kind;
    n->base.left = left;
    n->base.right = right;
    n->base.operator = op ? strdup(op) : NULL; //puede ser null
}

ArithmeticUnaryNode* ast_make_arithmetic_unary(NodeType kind, const char* op, ExpressionNode* operand, int row, int col){
    ArithmeticUnaryNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo = kind;
    n->base.operand = operand;
    n->base.operator = op ? strdup(op) : NULL;
}

BooleanUnaryNode* ast_make_boolean_unary(NodeType kind, const char* op, ExpressionNode* operand, int row, int col){
    BooleanUnaryNode* n = malloc(sizeof *n);
    n->base.base.base.row = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo = kind;
    n->base.operand = operand;
    n->base.operator = op ? strdup(op) : NULL;
}


// Node* build_ast_node(Production* p, Node** children) {
    
//     Node* node = NULL;
//     int N = p->right_len;
    
//     // Program
//     if (strcmp(p->left->name, "Program") == 0) {
//         node = (Node*)ast_make_program(
//             (DeclarationNode**)children[0], 
//             children[0] ? children[0]->child_count : 0,
//             (ExpressionNode*)children[1],
//             0, 0
//         );
//     }
//     // Type and Function declarations
//     else if (strcmp(p->left->name, "Type_function_list") == 0 && N == 2) {
//         // Just pass through the child nodes (handled in Program)
//         node = children[0];
//     }
//     else if (strcmp(p->left->name, "Func") == 0) {
//         FunctionDeclarationNode* inline_form = (FunctionDeclarationNode*)children[1];
//         node = (Node*)ast_make_function(
//             inline_form->name,
//             (DeclarationNode**)inline_form->params,
//             inline_form->param_counter,
//             (ExpressionNode*)inline_form->body,
//             inline_form->returnType,
//             0, 0
//         );
//     }
//     else if (strcmp(p->left->name, "Type") == 0) {
//         // Handle different type declaration variants
//         TypeDeclarationNode* type_dec = (TypeDeclarationNode*)children[1];
//         char* parent = NULL;
//         ExpressionNode** parent_args = NULL;
//         int parent_args_count = 0;
        
//         if (N >= 4 && strcmp(p->right[3]->name, "inherits") == 0) {
//             parent = children[4]->lexeme;
//             if (N > 5 && strcmp(p->right[5]->name, "LPAREN") == 0) {
//                 parent_args = (ExpressionNode**)children[6];
//                 parent_args_count = children[6]->child_count;
//             }
//         }
        
//         node = (Node*)ast_make_type(
//             type_dec->name,
//             (DeclarationNode**)type_dec->params,
//             type_dec->param_count,
//             parent,
//             parent_args,
//             parent_args_count,
//             NULL, 0,  // Attributes (to be filled from Type_block)
//             NULL, 0,  // Methods (to be filled from Type_block)
//             0, 0
//         );
//     }
//     // Expressions
//     else if (strcmp(p->left->name, "Expr") == 0) {
//         // Just pass through the specific expression type
//         node = children[0];
//     }
//     else if (strcmp(p->left->name, "Conditional") == 0) {
//         ExpressionNode** conditions = malloc(sizeof(ExpressionNode*));
//         ExpressionNode** exprs = malloc(sizeof(ExpressionNode*));
//         conditions[0] = (ExpressionNode*)children[2];
//         exprs[0] = (ExpressionNode*)children[5];
//         node = (Node*)ast_make_conditional(
//             conditions, exprs, 1,
//             (ExpressionNode*)children[6],  // else/elif part
//             0, 0
//         );
//     }
//     else if (strcmp(p->left->name, "Let_expr") == 0) {
//         node = (Node*)ast_make_let_in(
//             (VarDeclarationNode**)children[1], 
//             children[1] ? children[1]->child_count : 0,
//             (ExpressionNode*)children[3],
//             0, 0
//         );
//     }
//     else if (strcmp(p->left->name, "While_loop") == 0) {
//         node = (Node*)ast_make_while(
//             (ExpressionNode*)children[2],
//             (ExpressionNode*)children[4],
//             0, 0
//         );
//     }
//     else if (strcmp(p->left->name, "For_loop") == 0) {
//         node = (Node*)ast_make_for(
//             children[2]->lexeme,
//             (ExpressionNode*)children[4],
//             (ExpressionNode*)children[6],
//             0, 0
//         );
//     }
//     // Binary expressions
//     else if (strcmp(p->left->name, "Or_expr") == 0 && N == 3) {
//         node = (Node*)ast_make_boolean_binary(
//             NODE_OR, "||",
//             (ExpressionNode*)children[0],
//             (ExpressionNode*)children[2],
//             0, 0
//         );
//     }
//     else if (strcmp(p->left->name, "And_expr") == 0 && N == 3) {
//         node = (Node*)ast_make_boolean_binary(
//             NODE_AND, "&&",
//             (ExpressionNode*)children[0],
//             (ExpressionNode*)children[2],
//             0, 0
//         );
//     }
//     else if (strcmp(p->left->name, "Aritm_comp") == 0 && N == 3) {
//         const char* op = children[1]->lexeme;
//         NodeType type;
//         if (strcmp(op, "==") == 0) type = NODE_EQUAL;
//         else if (strcmp(op, "!=") == 0) type = NODE_NOT_EQUAL;
//         else if (strcmp(op, ">") == 0) type = NODE_GREATER;
//         else if (strcmp(op, ">=") == 0) type = NODE_GREATER_EQUAL;
//         else if (strcmp(op, "<") == 0) type = NODE_LESS;
//         else if (strcmp(op, "<=") == 0) type = NODE_LESS_EQUAL;
//         else type = NODE_BINARY;
        
//         node = (Node*)ast_make_equality_binary(
//             type, op,
//             (ExpressionNode*)children[0],
//             (ExpressionNode*)children[2],
//             0, 0
//         );
//     }
//     else if (strcmp(p->left->name, "Concat") == 0 && N == 3) {
//         const char* op = children[1]->lexeme;
//         NodeType type = (strcmp(op, "@") == 0) ? NODE_CONCAT : NODE_DOUBLE_CONCAT;
        
//         node = (Node*)ast_make_string_binary(
//             type, op,
//             (ExpressionNode*)children[0],
//             (ExpressionNode*)children[2],
//             0, 0
//         );
//     }
//     else if (strcmp(p->left->name, "Arithmetic") == 0 && N == 3) {
//         const char* op = children[1]->lexeme;
//         NodeType type;
//         if (strcmp(op, "+") == 0) type = NODE_PLUS;
//         else if (strcmp(op, "-") == 0) type = NODE_MINUS;
//         else type = NODE_ARITHMETIC_BINARY;
        
//         node = (Node*)ast_make_arithmetic_binary(
//             type, op,
//             (ExpressionNode*)children[0],
//             (ExpressionNode*)children[2],
//             0, 0
//         );
//     }
//     else if (strcmp(p->left->name, "Term") == 0 && N == 3) {
//         const char* op = children[1]->lexeme;
//         NodeType type;
//         if (strcmp(op, "*") == 0) type = NODE_MULT;
//         else if (strcmp(op, "/") == 0) type = NODE_DIV;
//         else if (strcmp(op, "%") == 0) type = NODE_MOD;
//         else type = NODE_ARITHMETIC_BINARY;
        
//         node = (Node*)ast_make_arithmetic_binary(
//             type, op,
//             (ExpressionNode*)children[0],
//             (ExpressionNode*)children[2],
//             0, 0
//         );
//     }
//     else if (strcmp(p->left->name, "Pow") == 0 && N == 3) {
//         node = (Node*)ast_make_arithmetic_binary(
//             NODE_POW, children[1]->lexeme,
//             (ExpressionNode*)children[0],
//             (ExpressionNode*)children[2],
//             0, 0
//         );
//     }
//     // Unary expressions
//     else if (strcmp(p->left->name, "Sign") == 0 && N == 2) {
//         node = (Node*)ast_make_arithmetic_unary(
//             (strcmp(children[0]->lexeme, "-") == 0) ? NODE_NEGATIVE : NODE_POSITIVE,
//             children[0]->lexeme,
//             (ExpressionNode*)children[1],
//             0, 0
//         );
//     }
//     else if (strcmp(p->left->name, "Factor") == 0 && N == 2) {
//         node = (Node*)ast_make_boolean_unary(
//             NODE_NOT, "!",
//             (ExpressionNode*)children[1],
//             0, 0
//         );
//     }
//     // Atoms
//     else if (strcmp(p->left->name, "Atom") == 0) {
//         // Just pass through the atomic expression
//         node = children[0];
//     }
//     else if (strcmp(p->left->name, "Call_func") == 0) {
//         if (N == 4) {
//             node = (Node*)ast_make_call_func(
//                 children[0]->lexeme,
//                 (ExpressionNode**)children[2],
//                 children[2] ? children[2]->child_count : 0,
//                 0, 0
//             );
//         } else { // N == 3 (empty arguments)
//             node = (Node*)ast_make_call_func(
//                 children[0]->lexeme,
//                 NULL, 0,
//                 0, 0
//             );
//         }
//     }
//     else if (strcmp(p->left->name, "Type_inst") == 0) {
//         node = (Node*)ast_make_type_instantiation(
//             children[1]->lexeme,
//             (ExpressionNode**)children[1]->children[2],
//             children[1]->children[2] ? children[1]->children[2]->child_count : 0,
//             0, 0
//         );
//     }
//     else if (strcmp(p->left->name, "Vector") == 0) {
//         node = children[0]; // Handled by Vector_exp or Vector_imp
//     }
//     else if (strcmp(p->left->name, "Vector_exp") == 0) {
//         if (N == 3) {
//             node = (Node*)ast_make_explicit_vector(
//                 (ExpressionNode**)children[1],
//                 children[1] ? children[1]->child_count : 0,
//                 0, 0
//             );
//         } else { // N == 2 (empty vector)
//             node = (Node*)ast_make_explicit_vector(NULL, 0, 0, 0);
//         }
//     }
//     else if (strcmp(p->left->name, "Index_object") == 0) {
//         node = (Node*)ast_make_index_object(
//             (ExpressionNode*)children[0],
//             (ExpressionNode*)children[2],
//             0, 0
//         );
//     }
//     else if (strcmp(p->left->name, "Member") == 0) {
//         if (children[2]->tipo == NODE_CALL_FUNC) {
//             node = (Node*)ast_make_call_method(
//                 (ExpressionNode*)children[0],
//                 children[2]->lexeme,
//                 (ExpressionNode**)children[2]->children[2],
//                 children[2]->children[2] ? children[2]->children[2]->child_count : 0,
//                 0, 0
//             );
//         } else {
//             node = (Node*)ast_make_call_type_attribute(
//                 (ExpressionNode*)children[0],
//                 children[2]->lexeme,
//                 0, 0
//             );
//         }
//     }
//     else if (strcmp(p->left->name, "Cast_type") == 0) {
//         node = (Node*)ast_make_cast_type(
//             (ExpressionNode*)children[0],
//             children[2]->lexeme,
//             0, 0
//         );
//     }
//     // Literals
//     else if (strcmp(p->left->name, "TRUE") == 0 || strcmp(p->left->name, "FALSE") == 0) {
//         node = (Node*)ast_make_literal(NODE_BOOLEAN, children[0]->lexeme, 0, 0);
//     }
//     else if (strcmp(p->left->name, "IDENTIFIER") == 0) {
//         node = (Node*)ast_make_literal(NODE_VAR, children[0]->lexeme, 0, 0);
//     }
//     else if (strcmp(p->left->name, "NUMBER") == 0) {
//         node = (Node*)ast_make_literal(NODE_NUMBER, children[0]->lexeme, 0, 0);
//     }
//     else if (strcmp(p->left->name, "STRING") == 0) {
//         node = (Node*)ast_make_literal(NODE_STRING, children[0]->lexeme, 0, 0);
//     }
//     // Blocks
//     else if (strcmp(p->left->name, "Expr_block") == 0) {
//         node = (Node*)ast_make_expression_block(
//             (ExpressionNode**)children[1],
//             children[1] ? children[1]->child_count : 0,
//             0, 0
//         );
//     }
//     else if (strcmp(p->left->name, "Type_block") == 0) {
//         // Type members are handled during Type creation
//         node = children[1]; // Type_member_list
//     }
//     // extra
//     else if (strcmp(p->left->name, "Check_type") == 0 && N == 3) {
//         node = (Node*)ast_make_check_type(NODE_CHECK_TYPE,
//             (ExpressionNode*)children[0],
//             (ExpressionNode*)children[2],
//             0, 0
//         );
//     }
//     else if (strcmp(p->left->name, "Assignment") == 0 && (N == 3 || N == 5)) {
//         // Handle variable assignments (including destructuring)
//         if (children[0]->tipo == NODE_VAR) {
//             // Simple assignment
//             node = (Node*)ast_make_var_decl(
//                 children[0]->lexeme,
//                 (ExpressionNode*)children[2],
//                 NULL, // Type annotation if present
//                 0, 0
//             );
//         } else {
//             // Destructuring assignment
//             node = (Node*)ast_make_destr(
//                 (ExpressionNode*)children[0],
//                 (ExpressionNode*)children[2],
//                 0, 0
//             );
//         }
        
//         // Handle chained assignments
//         if (N == 5) {
//             Node** assignments = malloc(2 * sizeof(Node*));
//             assignments[0] = node;
//             assignments[1] = children[4];
//             node = create_node(p->left, NULL, 2, assignments);
//             node->tipo = NODE_DESTRUCTURING;
//         }
//     }
//     else if (strcmp(p->left->name, "Destr_assig") == 0) {
//         node = (Node*)ast_make_destr(
//             (ExpressionNode*)children[0],
//             (ExpressionNode*)children[2],
//             0, 0
//         );
//     }
//     else if (strcmp(p->left->name, "Params_list") == 0) {
//         // Handle parameter lists for functions
//         if (N == 1) {
//             node = children[0];
//         } else {
//             // Combine multiple parameters
//             int param_count = children[0]->child_count + children[2]->child_count;
//             Node** params = malloc(param_count * sizeof(Node*));
//             // Copy existing parameters
//             for (int i = 0; i < children[0]->child_count; i++) {
//                 params[i] = children[0]->children[i];
//             }
//             for (int i = 0; i < children[2]->child_count; i++) {
//                 params[children[0]->child_count + i] = children[2]->children[i];
//             }
//             node = create_node(p->left, NULL, param_count, params);
//             node->tipo = NODE_VAR_DECLARATION; // Parameters are essentially variable declarations
//         }
//     }
//     else if (strcmp(p->left->name, "Method_signature") == 0) {
//         node = (Node*)ast_make_method_signature(
//             children[0]->lexeme,
//             (DeclarationNode**)children[2],
//             children[2] ? children[2]->child_count : 0,
//             NULL, // Return type to be filled during semantic analysis
//             0, 0
//         );
//     }
//     else if (strcmp(p->left->name, "Inline_form") == 0) {
//         // This is handled by the parent Func or Type_member_item production
//         node = children[0]; // Method_signature
//     }
//     else if (strcmp(p->left->name, "Sig") == 0) {
//         // This is handled by the parent Inline_form production
//         node = children[0]; // Expression or Expr_block
//     }
//     else if (strcmp(p->left->name, "Type_member_item") == 0) {
//         if (N == 4) {
//             // Method declaration
//             node = (Node*)ast_make_method(
//                 children[0]->lexeme,
//                 (DeclarationNode**)children[0]->children[2],
//                 children[0]->children[2] ? children[0]->children[2]->child_count : 0,
//                 (ExpressionNode*)children[3],
//                 NULL, // Return type to be filled during semantic analysis
//                 0, 0
//             );
//         } else if (N == 5) {
//             // Attribute with type annotation
//             node = (Node*)ast_make_type_attribute(
//                 children[0]->lexeme,
//                 (ExpressionNode*)children[4],
//                 children[2]->lexeme,
//                 0, 0
//             );
//         } else if (N == 3) {
//             // Attribute without type annotation
//             node = (Node*)ast_make_type_attribute(
//                 children[0]->lexeme,
//                 (ExpressionNode*)children[2],
//                 NULL,
//                 0, 0
//             );
//         }
//     }
//     else if (strcmp(p->left->name, "Protocol_block") == 0) {
//         node = (Node*)ast_make_protocol(
//             "Protocol", // Name should come from parent
//             (MethodSignatureNode**)children[1],
//             children[1] ? children[1]->child_count : 0,
//             NULL, // Parent protocol
//             0, 0
//         );
//     }
//     else if (strcmp(p->left->name, "Vector_imp") == 0) {
//         node = (Node*)ast_make_implicit_vector(
//             children[3]->lexeme, // Item variable
//             (ExpressionNode*)children[5], // Iterable
//             (ExpressionNode*)children[8], // Expression
//             0, 0
//         );
//     }
//     else if (strcmp(p->left->name, "Vector_item_list") == 0) {
//         if (N == 1) {
//             node = children[0];
//         } else {
//             // Combine vector items
//             int item_count = 1 + children[2]->child_count;
//             Node** items = malloc(item_count * sizeof(Node*));
//             items[0] = children[0];
//             for (int i = 0; i < children[2]->child_count; i++) {
//                 items[i + 1] = children[2]->children[i];
//             }
//             node = create_node(p->left, NULL, item_count, items);
//             node->tipo = NODE_EXPLICIT_VECTOR;
//         }
//     }
//     // Default case for epsilon productions
//     else if (strcmp(p->right[0]->name, "epsilon") == 0) {
//         node = create_node(p->left, NULL, 0, NULL);
//     }
//     // extras
//     else if (strcmp(p->left->name, "Non_empty_expr_list") == 0) {
//         if (N == 2) {
//         // Single expression
//         node = (Node*)ast_make_expression_block(
//             (ExpressionNode**)children[0],
//             1,
//             0, 0
//         );
//         } else {
//             // Multiple expressions
//             int expr_count = 1 + children[1]->child_count;
//             ExpressionNode** exprs = malloc(expr_count * sizeof(ExpressionNode*));
//             exprs[0] = (ExpressionNode*)children[0];
//             for (int i = 0; i < children[1]->child_count; i++) {
//                 exprs[i+1] = (ExpressionNode*)children[1]->children[i];
//             }
//             node = (Node*)ast_make_expression_block(exprs, expr_count, 0, 0);
//         }
//     }
//     else if (strcmp(p->left->name, "Expr_list") == 0) {
//         node = children[0]; // Pasa a través de Non_empty_expr_list o epsilon
//     }
//     else if (strcmp(p->left->name, "Expr_item_list") == 0) {
//         if (N == 2) {
//             // Single expression with semicolon
//             node = children[0];
//         } else {
//             // Expression block
//             node = children[0];
//         }
//     }
//     else if (strcmp(p->left->name, "Type_member_list") == 0) {
//         if (N == 2) {
//             // Combine members
//             int member_count = children[0]->child_count + children[1]->child_count;
//             Node** members = malloc(member_count * sizeof(Node*));
//             for (int i = 0; i < children[0]->child_count; i++) {
//                 members[i] = children[0]->children[i];
//             }
//             for (int i = 0; i < children[1]->child_count; i++) {
//                 members[children[0]->child_count + i] = children[1]->children[i];
//             }
//             node = create_node(p->left, NULL, member_count, members);
//         } else {
//             // Empty member list
//             node = create_node(p->left, NULL, 0, NULL);
//         }
//     }
//     else if (strcmp(p->left->name, "Type_dec") == 0) {
//         // Type_dec -> IDENTIFIER Signature
//         node = (Node*)ast_make_type_constructor_signature(
//             children[0]->lexeme,
//             (DeclarationNode**)children[1],
//             children[1] ? children[1]->child_count : 0,
//             0, 0
//         );
//     }
//     else if (strcmp(p->left->name, "Arguments_list") == 0) {
//         if (N == 1) {
//             node = children[0];
//         } else {
//             // Combine arguments
//             int arg_count = children[0]->child_count + children[2]->child_count;
//             Node** args = malloc(arg_count * sizeof(Node*));
//             for (int i = 0; i < children[0]->child_count; i++) {
//                 args[i] = children[0]->children[i];
//             }
//             for (int i = 0; i < children[2]->child_count; i++) {
//                 args[children[0]->child_count + i] = children[2]->children[i];
//             }
//             node = create_node(p->left, NULL, arg_count, args);
//         }
//     }
//     else if (strcmp(p->left->name, "Signature") == 0) {
//         if (N == 3) {
//             // LPAREN Params RPAREN
//             node = children[1];
//         } else {
//             // epsilon
//             node = create_node(p->left, NULL, 0, NULL);
//         }
//     }
//     else if (strcmp(p->left->name, "Params") == 0) {
//         if (N == 1) {
//             // Single parameter
//             node = (Node*)ast_make_var_decl(
//                 children[0]->lexeme,
//                 NULL, // No initial value
//                 NULL, // No type annotation
//                 0, 0
//             );
//         } else {
//             // epsilon
//             node = create_node(p->left, NULL, 0, NULL);
//         }
//     }
//     else if (strcmp(p->left->name, "Method_dec_list") == 0) {
//         // Similar to Type_member_list but for protocol methods
//         if (N == 2) {
//             int method_count = children[0]->child_count + children[1]->child_count;
//             Node** methods = malloc(method_count * sizeof(Node*));
//             for (int i = 0; i < children[0]->child_count; i++) {
//                 methods[i] = children[0]->children[i];
//             }
//             for (int i = 0; i < children[1]->child_count; i++) {
//                 methods[children[0]->child_count + i] = children[1]->children[i];
//             }
//             node = create_node(p->left, NULL, method_count, methods);
//         } else {
//             node = create_node(p->left, NULL, 0, NULL);
//         }
//     }
//     else if (strcmp(p->left->name, "Cond_other_case") == 0) {
//         if (N == 5) {  // ELIF LPAREN Expr RPAREN THEN Expr Cond_other_case
//         ConditionalNode* cond = (ConditionalNode*)node; // Asumiendo que el nodo padre es un ConditionalNode
//         // Expandir los arrays de condiciones y expresiones
//         ExpressionNode** new_conditions = realloc(cond->conditions, (cond->condition_counter + 1) * sizeof(ExpressionNode*));
//         ExpressionNode** new_exprs = realloc(cond->expressions, (cond->expression_counter + 1) * sizeof(ExpressionNode*));
        
//         new_conditions[cond->condition_counter] = (ExpressionNode*)children[2];
//         new_exprs[cond->expression_counter] = (ExpressionNode*)children[5];
        
//         cond->conditions = new_conditions;
//         cond->expressions = new_exprs;
//         cond->condition_counter++;
//         cond->expression_counter++;
        
//         // Manejar el caso restante (puede ser otro ELIF o ELSE)
//         node = (Node*)children[6];
//         } 
//         else if (N == 2) {  // ELSE Expr
//             ConditionalNode* cond = (ConditionalNode*)node;
//             cond->default_expre = (ExpressionNode*)children[1];
//             node = (Node*)cond;
//         }
//     }
//     else if (strcmp(p->left->name, "Arguments") == 0) {
//         if (N == 3) {  // Expr COMMA Arguments
//             int arg_count = 1 + children[2]->child_count;
//             ExpressionNode** args = malloc(arg_count * sizeof(ExpressionNode*));
//             args[0] = (ExpressionNode*)children[0];
//             for (int i = 0; i < children[2]->child_count; i++) {
//                 args[i + 1] = (ExpressionNode*)children[2]->children[i];
//             }
//             node = create_node(p->left, NULL, arg_count, (Node**)args);
//         } 
//         else {  // Expr
//             node = children[0];
//         }
//     }
//     else if (strcmp(p->left->name, "Protocol") == 0) {
//         if (N == 3) {  // PROTOCOL IDENTIFIER Protocol_block
//             node = (Node*)ast_make_protocol(
//                 children[1]->lexeme,
//                 (MethodSignatureNode**)children[2],
//                 children[2] ? children[2]->child_count : 0,
//                 NULL,  // No parent protocol
//                 0, 0
//             );
//         }
//         else if (N == 5) {  // PROTOCOL IDENTIFIER EXTENDS IDENTIFIER Protocol_block
//             node = (Node*)ast_make_protocol(
//                 children[1]->lexeme,
//                 (MethodSignatureNode**)children[4],
//                 children[4] ? children[4]->child_count : 0,
//                 children[3]->lexeme,  // Parent protocol
//                 0, 0
//             );
//         }
//     }
//     // Final fallback for any unhandled cases
//     else {
//         node = create_node(p->left, NULL, N, children);
//         printf("Unhandled production: %s -> ...\n", p->left->name);
//     }
    
//     return node;
// }
