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

// Funciones auxiliares
TypeChecker* create_type_checker(Context* context);
void add_error(TypeChecker* tc, const char* message, int row, int column);
bool is_error_type(Type* type);
bool is_auto_type(Type* type);
Type* assign_type(TypeChecker* tc, Type* var_type, Type* expr_type, int row, int col);
bool conforms_to(Type* type, Type* other);

// Funciones visitantes para cada tipo de nodo
void visit_program(TypeChecker* tc, ProgramNode* node, Scope* scope); // OK
//Declarations
void visit_type_declaration(TypeChecker* tc, TypeDeclarationNode* node, Scope* scope); // OK
void visit_type_attribute(TypeChecker* tc, TypeAttributeNode* node, Scope* scope); //OK
void visit_method_declaration(TypeChecker* tc, MethodDeclarationNode* node, Scope* scope); // OK
void visit_function_declaration(TypeChecker* tc, FunctionDeclarationNode* node, Scope* scope); // OK
void visit_var_declaration(TypeChecker* tc, VarDeclarationNode* node, Scope* scope); //OK
//Expressions
void visit_conditional(TypeChecker* tc, ConditionalNode* node, Scope* scope); //OK
void visit_let_in(TypeChecker* tc, LetInNode* node, Scope* scope);//OK
void visit_while(TypeChecker* tc, WhileNode* node, Scope* scope);//Ok
void visit_for(TypeChecker* tc, ForNode* node, Scope* scope);//Ok
void visit_destr(TypeChecker* tc, DestrNode* node, Scope* scope);
//Binary
void visit_equality_binary(TypeChecker* tc, EqualityBinaryNode* node, Scope* scope);
void visit_comparison_binary(TypeChecker* tc, ComparisonBinaryNode* node, Scope* scope);
void visit_arithmetic_binary(TypeChecker* tc, ArithmeticBinaryNode* node, Scope* scope);
void visit_boolean_binary(TypeChecker* tc, BooleanBinaryNode* node, Scope* scope);
void visit_check_type(TypeChecker* tc, CheckTypeNode* node, Scope* scope);
void visit_string_binary(TypeChecker* tc, StringBinaryNode* node, Scope* scope);

void visit_expression_block(TypeChecker* tc, ExpressionBlockNode* node, Scope* scope);
void visit_call_func(TypeChecker* tc, CallFuncNode* node, Scope* scope);
void visit_type_instantiation(TypeChecker* tc, TypeInstantiationNode* node, Scope* scope);
void visit_explicit_vector(TypeChecker* tc, ExplicitVectorNode* node, Scope* scope);
void visit_implicit_vector(TypeChecker* tc, ImplicitVectorNode* node, Scope* scope);
void visit_index_object(TypeChecker* tc, IndexObjectNode* node, Scope* scope);
void visit_call_method(TypeChecker* tc, CallMethodNode* node, Scope* scope);
void visit_call_type_attribute(TypeChecker* tc, CallTypeAttributeNode* node, Scope* scope);
void visit_cast_type(TypeChecker* tc, CastTypeNode* node, Scope* scope);
void visit_arithmetic_unary(TypeChecker* tc, ArithmeticUnaryNode* node, Scope* scope);
void visit_boolean_unary(TypeChecker* tc, BooleanUnaryNode* node, Scope* scope);
void visit_boolean_node(TypeChecker* tc, BooleanNode* node, Scope* scope);
void visit_string_node(TypeChecker* tc, StringNode* node, Scope* scope);
void visit_number_node(TypeChecker* tc, NumberNode* node, Scope* scope);
void visit_var_node(TypeChecker* tc, VarNode* node, Scope* scope);

//================Dispatcher================================
Type* visit(TypeChecker* tc, Node* node, Scope* scope); //OK

// Implementación de funciones auxiliares

TypeChecker* create_type_checker(Context* context) {
    TypeChecker* tc = (TypeChecker*)malloc(sizeof(TypeChecker));
    tc->context = context;
    tc->current_type = NULL;
    tc->current_method = NULL;
    tc->errors = NULL;
    return tc;
}

void add_error(TypeChecker* tc, const char* error_msg, int row, int column) {
    
    HulkSemanticError error;
    HulkSemanticError_init(&error, error_msg, row, column);
    HulkErrorList_add(tc->errors, (HulkError*)&error);
}

bool is_error_type(Type* type) {
    return type != NULL && strcmp(type->name, "<error>") == 0;
}

bool is_auto_type(Type* type) {
    return type != NULL && strcmp(type->name, "<auto>") == 0;
}

bool tc_conforms_to(Type* type, Type* other) {
    if (type == other) return true;
    
    // Check inheritance hierarchy
    Type* current = type;
    while (current != NULL) {
        if (current == other) return true;
        current = current->parent;
    }
    
    return false;
}

Type* assign_type(TypeChecker* tc, Type* var_type, Type* expr_type, int row, int col) {
    if (is_error_type(var_type) || is_error_type(expr_type)) {
        return var_type;
    }
    
    if (is_auto_type(var_type)) {
        return expr_type;
    }
    
    if (!tc_conforms_to(expr_type, var_type)) {
        char* error_msg = format_string(HULK_SEM_INCOMPATIBLE_TYPES, expr_type->name, var_type->name);
        add_error(tc, error_msg, row, col);
        free(error_msg);
        return context_get_type(tc->context, "<error>");
    }
    
    return var_type;
}




// ============================Implementación de funciones visitantes==================================

void visit_program(TypeChecker* tc, ProgramNode* node, Scope* scope) {
    node->scope = scope;
    
    Node** decl = (Node**)node->declarations;
    for (int i = 0; decl[i] != NULL; i++) {

        Scope* child_scope = create_scope(scope);
        
        // Dispatch based on declaration type
        switch (decl[i]->tipo) {
            //-------------declarations----------------------
            case NODE_TYPE_DECLARATION:
                visit_type_declaration(tc, (TypeDeclarationNode*)decl[i], child_scope);
                break;
            case NODE_FUNCTION_DECLARATION:
                visit_function_declaration(tc, (FunctionDeclarationNode*)decl[i], child_scope);
                break;
            case NODE_VAR_DECLARATION:
                visit_var_declaration(tc, (VarDeclarationNode*)decl[i], child_scope);
                break;
            default:
                break;
        }
    }
    
    tc->current_type = NULL;
    tc->current_method = NULL;
    
    Node** exp = (Node**)node->expression;
    for (int i = 0; exp[i] != NULL; i++) {

        Scope* child_scope = create_scope(scope);
        
        // Dispatch based on declaration type
        switch (exp->tipo) {
             //-----------------expressions---------------------
            case NODE_CONDITIONAL:
                visit_conditional(tc, (ConditionalNode*)exp[i], child_scope);
                break;
            case NODE_LET_IN:
                visit_let_in(tc, (LetInNode*)exp[i], child_scope);
                break;
            case NODE_WHILE:
                visit_while(tc, (WhileNode*)exp[i], child_scope);
                break;
            case NODE_FOR:
                visit_for(tc, (ForNode*)exp[i], child_scope);
            case NODE_DESTRUCTURING:
                visit_destr(tc, (DestrNode*)exp[i], child_scope);
            default:
                break;
        }
    }
}
 


//==================== DECLARATIONS ==================== 

void visit_type_declaration(TypeChecker* tc, TypeDeclarationNode* node, Scope* scope) {
    node->base.base.scope = scope;
    
    // Check if it's a built-in Hulk type
    bool is_hulk_type = false;
    for (int i = 0; i < tc->context->hulk_type_count; i++) {
        if (strcmp(node->name, tc->context->hulk_types[i]) == 0) {
            is_hulk_type = true;
            break;
        }
    }
    if (is_hulk_type) return;
    
    tc->current_type = context_get_type(tc->context, node->name);
    if (is_error_type(tc->current_type)) return;
    
    // Handle type parameters
    if (tc->current_type->param_count > 0) {
        for (int i = 0; i < tc->current_type->param_count; i++) {
            scope_define_variable(scope, tc->current_type->param_names[i], 
                          tc->current_type->param_types[i], true);
        }
        
        // Check parent arguments
        if (!is_error_type(tc->current_type->parent)) {
            if (node->parent_args_count != tc->current_type->parent->param_count) {
                char* error_msg = format_string(HULK_SEM_INVALID_LEN_ARGUMENTS, 
                    tc->current_type->parent->name, 
                    tc->current_type->parent->param_count, 
                    node->parent_args_count);
                add_error(tc, error_msg, node->base.base.row, node->base.base.column);
            } else {
                for (int i = 0; i < node->parent_args_count; i++) {
                    // Type check each argument
                    Type* arg_type = visit(tc, (Node*)node->parent_args[i], scope);
                    assign_type(tc, tc->current_type->parent->param_types[i], arg_type, 
                                node->base.base.row, node->base.base.column);
                }
            }
        }
    }
    
    // Check attributes
    Node** attribute = (Node**)node->attributes;
    for (int i = 0; i < node->attribute_counter; i++) {
        if(attribute[i]->tipo == NODE_TYPE_ATTRIBUTE){
            visit_type_attribute(tc, attribute[i], scope);
        }
    }
    
    // Check methods
    Node** methods = (Node**)node->methods;
    for (int i = 0; i < node->method_counter; i++) {
        if(methods[i]->tipo == NODE_METHOD_DECLARATION){
            Scope* method_scope = create_scope(scope);
            visit_method_declaration(tc, methods[i], method_scope);

        }
    }
}

void visit_type_attribute(TypeChecker* tc, TypeAttributeNode* node, Scope* scope) {
    node->base.base.scope = scope;
    tc->current_method = NULL;
    
    Attribute* attribute = get_attribute(tc->current_type, node->name);
    if (is_error_type((Type*)attribute)) return;
    
    // Check attribute value
    Type* expr_type = visit(tc, (Node*)node->value, scope);
    Type* var_type = assign_type(tc, attribute->type, expr_type, node->base.base.row, node->base.base.column);
    attribute->type = var_type;
}

void visit_method_declaration(TypeChecker* tc, MethodDeclarationNode* node, Scope* scope) {
    node->base.base.scope = scope;
    tc->current_method = get_method(tc->current_type, node->name);
    
    if (is_error_type((Type*)tc->current_method)) return;
    
    // Check for invalid override
    if (tc->current_type->parent != NULL) {
        Method* parent_method = get_method(tc->current_type->parent, node->name);
        if (parent_method != NULL && tc->current_method != parent_method) {
            char* error_msg = format_string(HULK_SEM_INVALID_OVERRIDE, 
                node->name, tc->current_type->name);
            add_error(tc, error_msg, node->base.base.row, node->base.base.column);
            free(error_msg);
        }
    }
    
    // Define parameters in scope
    for (int i = 0; i < tc->current_method->param_count; i++) {
        scope_define_variable(scope, tc->current_method->param_names[i], 
                      tc->current_method->param_types[i], true);
    }
    
    //Check return type
    Type* expr_type = visit(tc, (Node*)node->body, scope);
    Type* return_type = assign_type(tc, tc->current_method->return_type, expr_type, 
                                    node->base.base.row, node->base.base.column);
    tc->current_method->inferred_return_type = return_type;
}

void visit_function_declaration(TypeChecker* tc, FunctionDeclarationNode* node, Scope* scope) {
    tc->current_type = NULL;
    node->base.base.scope = scope;
    
    // Check if it's a built-in Hulk function
    bool is_hulk_func = false;
    for (int i = 0; i < tc->context->hulk_function_count; i++) {
        if (strcmp(node->name, tc->context->hulk_functions[i]) == 0) {
            is_hulk_func = true;
            break;
        }
    }
    if (is_hulk_func) return;
    
    tc->current_method = context_get_function(tc->context, node->name);
    if (is_error_type((Type*)tc->current_method)) return;
    
    // Define parameters in scope
    for (int i = 0; i < tc->current_method->param_count; i++) {
        scope_define_variable(scope, tc->current_method->param_names[i], 
                      tc->current_method->param_types[i], true);
    }
    
    //Check return type
    Type* expr_type = visit(tc, node->body, scope);
    Type* return_type = assign_type(tc, tc->current_method->return_type, expr_type, 
                                    node->base.base.row, node->base.base.column);
    tc->current_method->inferred_return_type = return_type;
}

void visit_var_declaration(TypeChecker* tc, VarDeclarationNode* node, Scope* scope) {
    node->base.base.scope = scope;
    
    if (scope_find_variable(scope, node->name, -1) != NULL) {
        char* error_msg = format_string(HULK_SEM_VARIABLE_IS_DEFINED, node->name);
        add_error(tc, error_msg, node->base.base.row, node->base.base.column);
        free(error_msg);
        return;
    }
    
    Type* var_type;
    if (node->type != NULL) {
        var_type = context_get_type(tc->context, node->type);
        if (is_error_type(var_type)) {
            char* error_msg = format_string(HULK_SEM_NOT_DEFINED, node->type);
            add_error(tc, error_msg, node->base.base.row, node->base.base.column);
            free(error_msg);
        }
    } else {
        var_type = context_get_type(tc->context, "<auto>");
    }
    
    // Check variable value
    Type* expr_type = visit(tc, (Node*)node->value, scope);
    var_type = assign_type(tc, var_type, expr_type, node->base.base.row, node->base.base.column);
    scope_define_variable(scope, node->name, var_type, false);
}


// ==================== EXPRESSIONS ==================== 

void visit_conditional(TypeChecker* tc, ConditionalNode* node, Scope* scope) {
    node->base.base.scope = scope;

    Type* default_type = NULL;
    Type** types = (Type**)malloc(sizeof(Type*) * (node->condition_counter + 1));
    
    // Check conditions and expressions
    Node** conditions = (Node**)node->conditions;
    Node** expressions = (Node**)node->expressions;

    for (int i = 0; i < node->condition_counter; i++) {
        Type* cond_type = visit(tc, conditions[i], scope);
        if (!is_error_type(cond_type) && !is_auto_type(cond_type) && 
            strcmp(cond_type->name, "Boolean") != 0) {
            char* error_msg = format_string(HULK_SEM_INCOMPATIBLE_TYPES, cond_type->name, "Boolean");
            add_error(tc, error_msg, node->base.base.row, node->base.base.column);
            free(error_msg);
        }
        types[i] = visit(tc, expressions[i], scope);
        // Special case for 'true' condition
        if (node->conditions[i]->tipo == NODE_BOOLEAN && 
            strcmp(((BooleanNode*)conditions[i])->base.lex, "true") == 0) {
            Type* result = visit(tc, expressions[i], scope);
            free(types);
            return result;
        }
    }
    
    // Check default expression
    default_type = visit(tc, node->default_expr, scope);
    types[node->condition_count] = default_type;
    
    // Get lowest common ancestor
    Type* result = get_lowest_common_ancestor(types, node->condition_count + 1);
    free(types);
    return result;
}

void visit_let_in(TypeChecker* tc, LetInNode* node, Scope* scope) {
    node->base.base.scope = scope;
    Scope* current_scope = scope;
    
    // Process variable declarations
    Node** variables = (Node**)node->variables;
    for (int i = 0; i < node->variable_counter; i++) {
        current_scope = create_scope(current_scope);
        visit_var_declaration(tc, (Node**)variables[i], current_scope);
    }
    
    // Check body expression
    return visit(tc, (Node*)node->body, current_scope);
}

void visit_while(TypeChecker* tc, WhileNode* node, Scope* scope) {
    node->base.base.scope = scope;
    
    // Check condition
    Type* cond_type = visit(tc, (Node*)node->condition, scope);
    if (!is_error_type(cond_type) && !is_auto_type(cond_type) && 
        strcmp(cond_type->name, "Boolean") != 0) {
        char* error_msg = format_string(HULK_SEM_INCOMPATIBLE_TYPES, cond_type->name, "Boolean");
        add_error(tc, error_msg, node->base.base.row, node->base.base.column);
        free(error_msg);
    }
    
    // Check body
    return visit(tc, (Node*)node->body, scope);
}

void visit_for(TypeChecker* tc, ForNode* node, Scope* scope) {
    node->base.base.scope = create_scope(scope);
    
    // Check iterable
    Type* iterable_type = visit(tc, (Node*)node->iterable, node->base.base.scope);
    Protocol* iterable_protocol = context_get_protocol(tc->context, "Iterable");
    
    if (!is_error_type(iterable_type) && !is_auto_type(iterable_type)) {
        if (!conforms_to(iterable_type, (Type*)iterable_protocol)) {
            char* error_msg = format_string(HULK_SEM_NOT_CONFORMS_TO, iterable_type->name, "Iterable");
            add_error(tc, error_msg, node->base.base.row, node->base.base.column);
            free(error_msg);
        } else {
            // Define item variable with current type from iterable
            Method* current_method = get_method(iterable_type, "current");
            VarNode* item = (VarNode*)node->item;
            scope_define_variable(node->base.base.scope, item->base.lex, current_method->inferred_return_type, false);
        }
    } else {
        // Define item variable with auto type if we can't determine the type
        VarNode* item = (VarNode*)node->item;
        define_variable(node->base.scope, item->base.lex, context_get_type(tc->context, "<auto>"), false);
    }
    
    // Check body
    return visit(tc, (Node*)node->body, node->base.base.scope);
}

void visit_destr(TypeChecker* tc, DestrNode* node, Scope* scope) {
    node->base.base.scope = scope;
    
    // Check variable
    Type* var_type = visit(tc, (Node*)node->var, scope);
    
    // Check expression
    Type* expr_type = visit(tc, (Node*)node->expr, scope);
    
    // Special case for 'self'
    if (tc->current_type != NULL && tc->current_method != NULL && 
        strcmp(((VarNode*)node->var)->base.lex, "self") == 0) {
        add_error(tc, HULK_SEM_SELF_IS_READONLY, node->base.base.row, node->base.base.column);
        return context_get_type(tc->context, "<error>");
    }
    
    // Check type compatibility
    if (!is_error_type(var_type) && !is_error_type(expr_type) && 
        !is_auto_type(var_type) && !is_auto_type(expr_type) && 
        !conforms_to(expr_type, var_type)) {
        char* error_msg = format_string(HULK_SEM_INCOMPATIBLE_TYPES, expr_type->name, var_type->name);
        add_error(tc, error_msg, node->base.base.row, node->base.base.column);
        free(error_msg);
    }
    
    return var_type;
}

// ==================== BINARY ==================== 

void visit_equality_binary(TypeChecker* tc, EqualityBinaryNode* node, Scope* scope) {
    node->base.scope = scope;
    Type* left_type = visit(tc, node->left, scope);
    Type* right_type = visit(tc, node->right, scope);
    
    if (is_error_type(left_type) || is_error_type(right_type)) {
        return get_type(tc->context, "Boolean");
    }
    
    // Check valid combinations
    bool valid = false;
    if (strcmp(left_type->name, "Boolean") == 0 && strcmp(right_type->name, "Boolean") == 0) ||
        (strcmp(left_type->name, "Number") == 0 && strcmp(right_type->name, "Number") == 0) ||
        (is_auto_type(left_type) && is_auto_type(right_type)) {
        valid = true;
    }
    
    if (!valid) {
        char* error_msg = format_string(HULK_SEM_INVALID_OPERATION, left_type->name, right_type->name);
        add_error(tc, error_msg, node->base.row, node->base.column);
        free(error_msg);
    }
    
    return get_type(tc->context, "Boolean");
}

void visit_comparison_binary(TypeChecker* tc, ComparisonBinaryNode* node, Scope* scope) {
    node->base.scope = scope;
    Type* left_type = visit(tc, node->left, scope);
    Type* right_type = visit(tc, node->right, scope);
    
    if (is_error_type(left_type) || is_error_type(right_type)) {
        return get_type(tc->context, "Boolean");
    }
    
    // Check valid combinations (only numbers)
    if (strcmp(left_type->name, "Number") != 0 || strcmp(right_type->name, "Number") != 0) {
        char* error_msg = format_string(HULK_SEM_INVALID_OPERATION, left_type->name, right_type->name);
        add_error(tc, error_msg, node->base.row, node->base.column);
        free(error_msg);
    }
    
    return get_type(tc->context, "Boolean");
}

void visit_arithmetic_binary(TypeChecker* tc, ArithmeticBinaryNode* node, Scope* scope) {
    node->base.scope = scope;
    Type* left_type = visit(tc, node->left, scope);
    Type* right_type = visit(tc, node->right, scope);
    
    if (is_error_type(left_type) || is_error_type(right_type)) {
        return get_type(tc->context, "Number");
    }
    
    // Check valid combinations (only numbers)
    if ((strcmp(left_type->name, "Number") != 0 && !is_auto_type(left_type)) ||
        (strcmp(right_type->name, "Number") != 0 && !is_auto_type(right_type))) {
        char* error_msg = format_string(HULK_SEM_INVALID_OPERATION, left_type->name, right_type->name);
        add_error(tc, error_msg, node->base.row, node->base.column);
        free(error_msg);
    }
    
    return get_type(tc->context, "Number");
}

void visit_boolean_binary(TypeChecker* tc, BooleanBinaryNode* node, Scope* scope) {
    node->base.scope = scope;
    Type* left_type = visit(tc, node->left, scope);
    Type* right_type = visit(tc, node->right, scope);
    
    if (is_error_type(left_type) || is_error_type(right_type)) {
        return get_type(tc->context, "Boolean");
    }
    
    // Check valid combinations (only booleans)
    if ((strcmp(left_type->name, "Boolean") != 0 && !is_auto_type(left_type)) ||
        (strcmp(right_type->name, "Boolean") != 0 && !is_auto_type(right_type))) {
        char* error_msg = format_string(HULK_SEM_INVALID_OPERATION, left_type->name, right_type->name);
        add_error(tc, error_msg, node->base.row, node->base.column);
        free(error_msg);
    }
    
    return get_type(tc->context, "Boolean");
}

void visit_check_type(TypeChecker* tc, CheckTypeNode* node, Scope* scope) {
    node->base.scope = scope;
    Type* left_type = visit(tc, node->left, scope);
    
    if (is_error_type(left_type)) {
        return get_type(tc->context, "Boolean");
    }
    
    // Check if right type exists
    Type* right_type = get_type(tc->context, node->right);
    if (right_type == NULL) {
        Protocol* protocol = get_protocol(tc->context, node->right);
        if (protocol == NULL) {
            char* error_msg = format_string(HULK_SEM_INVALID_IS_OPERATION, node->right);
            add_error(tc, error_msg, node->base.row, node->base.column);
            free(error_msg);
        }
    }
    
    return get_type(tc->context, "Boolean");
}

void visit_string_binary(TypeChecker* tc, StringBinaryNode* node, Scope* scope) {
    node->base.scope = scope;
    Type* left_type = visit(tc, node->left, scope);
    Type* right_type = visit(tc, node->right, scope);
    
    // Check if types are valid for string operations
    bool left_valid = strcmp(left_type->name, "String") == 0 || 
                     strcmp(left_type->name, "Boolean") == 0 || 
                     strcmp(left_type->name, "Number") == 0 ||
                     is_auto_type(left_type) || 
                     is_error_type(left_type);
    
    bool right_valid = strcmp(right_type->name, "String") == 0 || 
                      strcmp(right_type->name, "Boolean") == 0 || 
                      strcmp(right_type->name, "Number") == 0 ||
                      is_auto_type(right_type) || 
                      is_error_type(right_type);
    
    if (!left_valid || !right_valid) {
        char* error_msg = format_string(HULK_SEM_INVALID_OPERATION, left_type->name, right_type->name);
        add_error(tc, error_msg, node->base.row, node->base.column);
        free(error_msg);
    }
    
    return get_type(tc->context, "String");
}
/*
// ==================== ATOMIC ==================== 

void visit_expression_block(TypeChecker* tc, ExpressionBlockNode* node, Scope* scope) {
    node->base.scope = create_scope(scope);
    Type* last_type = NULL;
    
    for (int i = 0; i < node->expression_count; i++) {
        last_type = visit(tc, node->expressions[i], node->base.scope);
    }
    
    return last_type ? last_type : get_type(tc->context, "<error>");
}

void visit_call_func(TypeChecker* tc, CallFuncNode* node, Scope* scope) {
    node->base.scope = scope;
    
    // Special case for 'base' call
    if (strcmp(node->name, "base") == 0 && node->argument_count == 0 && 
        tc->current_type != NULL && tc->current_method != NULL) {
        Method* parent_method = get_method(tc->current_type->parent, tc->current_method->name);
        if (parent_method == NULL) {
            add_error(tc, "Parent method not found", node->base.row, node->base.column);
            return get_type(tc->context, "<error>");
        }
        return parent_method->inferred_return_type;
    }
    
    // Get function from context
    Method* function = get_function(tc->context, node->name);
    if (function == NULL || is_error_type((Type*)function)) {
        char* error_msg = format_string(HULK_SEM_NOT_DEFINED, node->name);
        add_error(tc, error_msg, node->base.row, node->base.column);
        free(error_msg);
        return get_type(tc->context, "<error>");
    }
    
    // Check argument count
    if (node->argument_count != function->param_count) {
        char* error_msg = format_string(HULK_SEM_INVALID_LEN_ARGUMENTS, 
                                      function->name, function->param_count, node->argument_count);
        add_error(tc, error_msg, node->base.row, node->base.column);
        free(error_msg);
        return function->inferred_return_type;
    }
    
    // Check argument types
    for (int i = 0; i < node->argument_count; i++) {
        Type* arg_type = visit(tc, node->arguments[i], scope);
        Type* param_type = function->param_types[i];
        
        if (!is_error_type(arg_type) && !is_error_type(param_type)) {
            if (!is_auto_type(param_type) && !conforms_to(arg_type, param_type)) {
                char* error_msg = format_string(HULK_SEM_INVALID_TYPE_ARGUMENTS, 
                                              function->param_names[i], param_type->name, 
                                              function->name, arg_type->name);
                add_error(tc, error_msg, node->base.row, node->base.column);
                free(error_msg);
            }
        }
    }
    
    // Special case for print function
    if (strcmp(node->name, "print") == 0 && node->argument_count > 0) {
        return visit(tc, node->arguments[0], scope);
    }
    
    return function->inferred_return_type;
}

void visit_type_instantiation(TypeChecker* tc, TypeInstantiationNode* node, Scope* scope) {
    node->base.scope = scope;
    
    // Get type from context
    Type* type = get_type(tc->context, node->name);
    if (type == NULL || is_error_type(type)) {
        char* error_msg = format_string(HULK_SEM_NOT_DEFINED, node->name);
        add_error(tc, error_msg, node->base.row, node->base.column);
        free(error_msg);
        return get_type(tc->context, "<error>");
    }
    
    // Check argument count
    if (node->argument_count != type->param_count) {
        char* error_msg = format_string(HULK_SEM_INVALID_LEN_ARGUMENTS, 
                                      type->name, type->param_count, node->argument_count);
        add_error(tc, error_msg, node->base.row, node->base.column);
        free(error_msg);
        return type;
    }
    
    // Check argument types
    for (int i = 0; i < node->argument_count; i++) {
        Type* arg_type = visit(tc, node->arguments[i], scope);
        Type* param_type = type->param_types[i];
        
        if (!is_error_type(arg_type) {
            if (!is_auto_type(param_type) && !conforms_to(arg_type, param_type)) {
                char* error_msg = format_string(HULK_SEM_INVALID_TYPE_ARGUMENTS, 
                                              type->param_names[i], param_type->name, 
                                              type->name, arg_type->name);
                add_error(tc, error_msg, node->base.row, node->base.column);
                free(error_msg);
            }
        }
    }
    
    return type;
}

void visit_explicit_vector(TypeChecker* tc, ExplicitVectorNode* node, Scope* scope) {
    node->base.scope = scope;
    
    if (node->item_count == 0) {
        return create_vector_type(get_type(tc->context, "<auto>"));
    }
    
    // Check all items have compatible types
    Type** item_types = (Type**)malloc(sizeof(Type*) * node->item_count);
    for (int i = 0; i < node->item_count; i++) {
        item_types[i] = visit(tc, node->items[i], scope);
    }
    
    Type* common_type = get_lowest_common_ancestor(item_types, node->item_count);
    free(item_types);
    
    if (is_error_type(common_type)) {
        add_error(tc, HULK_SEM_VECTOR_OBJECT_DIFFERENT_TYPES, node->base.row, node->base.column);
    }
    
    return create_vector_type(common_type);
}

void visit_implicit_vector(TypeChecker* tc, ImplicitVectorNode* node, Scope* scope) {
    node->base.scope = create_scope(scope);
    
    // Check iterable
    Type* iterable_type = visit(tc, node->iterable, node->base.scope);
    Protocol* iterable_protocol = get_protocol(tc->context, "Iterable");
    
    if (!is_error_type(iterable_type) && !is_auto_type(iterable_type)) {
        if (!conforms_to(iterable_type, (Type*)iterable_protocol)) {
            char* error_msg = format_string(HULK_SEM_NOT_CONFORMS_TO, iterable_type->name, "Iterable");
            add_error(tc, error_msg, node->base.row, node->base.column);
            free(error_msg);
        } else {
            // Define item variable with current type from iterable
            Method* current_method = get_method(iterable_type, "current");
            define_variable(node->base.scope, node->item, current_method->inferred_return_type, false);
        }
    } else {
        // Define item variable with auto type if we can't determine the type
        define_variable(node->base.scope, node->item, get_type(tc->context, "<auto>"), false);
    }
    
    // Check expression
    Type* expr_type = visit(tc, node->expr, node->base.scope);
    return create_vector_type(expr_type);
}

void visit_index_object(TypeChecker* tc, IndexObjectNode* node, Scope* scope) {
    node->base.scope = scope;
    
    // Check index
    Type* pos_type = visit(tc, node->pos, scope);
    if (!is_error_type(pos_type) && !is_auto_type(pos_type) && 
        strcmp(pos_type->name, "Number") != 0) {
        char* error_msg = format_string(HULK_SEM_INVALID_INDEXING_OPERATION, pos_type->name);
        add_error(tc, error_msg, node->base.row, node->base.column);
        free(error_msg);
    }
    
    // Check object
    Type* object_type = visit(tc, node->object, scope);
    if (is_error_type(object_type)) {
        return get_type(tc->context, "<error>");
    }
    
    // Check if object is indexable (has a current method)
    Method* current_method = get_method(object_type, "current");
    if (current_method == NULL) {
        char* error_msg = format_string(HULK_SEM_INVALID_INDEXING, object_type->name);
        add_error(tc, error_msg, node->base.row, node->base.column);
        free(error_msg);
        return get_type(tc->context, "<error>");
    }
    
    return current_method->inferred_return_type;
}

void visit_call_method(TypeChecker* tc, CallMethodNode* node, Scope* scope) {
    node->base.scope = scope;
    
    // Check instance
    Type* owner_type = visit(tc, node->inst_name, scope);
    if (is_error_type(owner_type)) {
        return get_type(tc->context, "<error>");
    }
    
    // Get method
    Method* method = get_method(owner_type, node->method_name);
    if (method == NULL || is_error_type((Type*)method)) {
        char* error_msg = format_string(HULK_SEM_NOT_DEFINED, node->method_name);
        add_error(tc, error_msg, node->base.row, node->base.column);
        free(error_msg);
        return get_type(tc->context, "<error>");
    }
    
    // Check argument count
    if (node->method_args_count != method->param_count) {
        char* error_msg = format_string(HULK_SEM_INVALID_LEN_ARGUMENTS, 
                                      method->name, method->param_count, node->method_args_count);
        add_error(tc, error_msg, node->base.row, node->base.column);
        free(error_msg);
        return method->inferred_return_type;
    }
    
    // Check argument types
    for (int i = 0; i < node->method_args_count; i++) {
        Type* arg_type = visit(tc, node->method_args[i], scope);
        Type* param_type = method->param_types[i];
        
        if (!is_error_type(arg_type)) {
            if (!is_auto_type(param_type) && !conforms_to(arg_type, param_type)) {
                char* error_msg = format_string(HULK_SEM_INVALID_TYPE_ARGUMENTS, 
                                              method->param_names[i], param_type->name, 
                                              arg_type->name);
                add_error(tc, error_msg, node->base.row, node->base.column);
                free(error_msg);
            }
        }
    }
    
    return method->inferred_return_type;
}

void visit_call_type_attribute(TypeChecker* tc, CallTypeAttributeNode* node, Scope* scope) {
    node->base.scope = scope;
    
    // Check instance
    Type* owner_type = visit(tc, node->inst_name, scope);
    if (is_error_type(owner_type)) {
        return get_type(tc->context, "<error>");
    }
    
    // Check if attribute is private
    if (tc->current_type == NULL || owner_type != tc->current_type) {
        char* error_msg = format_string(HULK_SEM_PRIVATE_ATTRIBUTE, node->attribute, owner_type->name);
        add_error(tc, error_msg, node->base.row, node->base.column);
        free(error_msg);
        return get_type(tc->context, "<error>");
    }
    
    // Get attribute
    Attribute* attribute = get_attribute(owner_type, node->attribute);
    if (attribute == NULL || is_error_type((Type*)attribute)) {
        char* error_msg = format_string(HULK_SEM_NOT_DEFINED, node->attribute);
        add_error(tc, error_msg, node->base.row, node->base.column);
        free(error_msg);
        return get_type(tc->context, "<error>");
    }
    
    return attribute->type;
}

void visit_cast_type(TypeChecker* tc, CastTypeNode* node, Scope* scope) {
    node->base.scope = scope;
    
    // Check instance
    Type* inst_type = visit(tc, node->inst_name, scope);
    if (is_error_type(inst_type)) {
        return get_type(tc->context, "<error>");
    }
    
    // Get target type
    Type* target_type = get_type(tc->context, node->type_cast);
    if (target_type == NULL) {
        Protocol* protocol = get_protocol(tc->context, node->type_cast);
        if (protocol == NULL) {
            char* error_msg = format_string(HULK_SEM_NOT_DEFINED, node->type_cast);
            add_error(tc, error_msg, node->base.row, node->base.column);
            free(error_msg);
            return get_type(tc->context, "<error>");
        }
        target_type = (Type*)protocol;
    }
    
    // Check if cast is valid
    if (!conforms_to(inst_type, target_type)) {
        char* error_msg = format_string(HULK_SEM_INVALID_CAST_OPERATION, inst_type->name, target_type->name);
        add_error(tc, error_msg, node->base.row, node->base.column);
        free(error_msg);
    }
    
    return target_type;
}

// ==================== UNARY ==================== 

void visit_arithmetic_unary(TypeChecker* tc, ArithmeticUnaryNode* node, Scope* scope) {
    node->base.scope = scope;
    Type* operand_type = visit(tc, node->operand, scope);
    
    if (is_error_type(operand_type)) {
        return get_type(tc->context, "Number");
    }
    
    if (strcmp(operand_type->name, "Number") != 0 && !is_auto_type(operand_type)) {
        char* error_msg = format_string(HULK_SEM_INVALID_UNARY_OPERATION, operand_type->name);
        add_error(tc, error_msg, node->base.row, node->base.column);
        free(error_msg);
    }
    
    return get_type(tc->context, "Number");
}

void visit_boolean_unary(TypeChecker* tc, BooleanUnaryNode* node, Scope* scope) {
    node->base.scope = scope;
    Type* operand_type = visit(tc, node->operand, scope);
    
    if (is_error_type(operand_type)) {
        return get_type(tc->context, "Boolean");
    }
    
    if (strcmp(operand_type->name, "Boolean") != 0 && !is_auto_type(operand_type)) {
        char* error_msg = format_string(HULK_SEM_INVALID_UNARY_OPERATION, operand_type->name);
        add_error(tc, error_msg, node->base.row, node->base.column);
        free(error_msg);
    }
    
    return get_type(tc->context, "Boolean");
}

// ==================== LITERALS ==================== 

void visit_boolean_node(TypeChecker* tc, BooleanNode* node, Scope* scope) {
    node->base.scope = scope;
    return get_type(tc->context, "Boolean");
}

void visit_string_node(TypeChecker* tc, StringNode* node, Scope* scope) {
    node->base.scope = scope;
    return get_type(tc->context, "String");
}

void visit_number_node(TypeChecker* tc, NumberNode* node, Scope* scope) {
    node->base.scope = scope;
    return get_type(tc->context, "Number");
}

void visit_var_node(TypeChecker* tc, VarNode* node, Scope* scope) {
    node->base.scope = scope;
    
    // Special case for 'self'
    if (strcmp(node->lex, "self") == 0) {
        if (tc->current_type != NULL && tc->current_method != NULL) {
            return tc->current_type;
        } else {
            add_error(tc, "\"self\" is not defined in this context", node->base.row, node->base.column);
            return get_type(tc->context, "<error>");
        }
    }
    
    // Look for variable in scope
    VariableInfo* var = find_variable(scope, node->lex, -1);
    if (var == NULL) {
        char* error_msg = format_string(HULK_SEM_NOT_DEFINED, node->lex);
        add_error(tc, error_msg, node->base.row, node->base.column);
        free(error_msg);
        return get_type(tc->context, "<error>");
    }
    
    return var->type;
}

// ==================== DISPATCH ==================== 
*/

Type* visit(TypeChecker* tc, Node* node, Scope* scope) {
    if (node == NULL) return get_type(tc->context, "<error>");
    
    switch (node->tipo) {
        // Declarations
        case NODE_TYPE_DECLARATION:
            visit_type_declaration(tc, (TypeDeclarationNode*)node, scope);
            return NULL;
        case NODE_FUNCTION_DECLARATION:
            visit_function_declaration(tc, (FunctionDeclarationNode*)node, scope);
            return NULL;
        case NODE_VAR_DECLARATION:
            visit_var_declaration(tc, (VarDeclarationNode*)node, scope);
            return NULL;
            
        // Expressions
        case NODE_CONDITIONAL:
            return visit_conditional(tc, (ConditionalNode*)node, scope);
        case NODE_LET_IN:
            return visit_let_in(tc, (LetInNode*)node, scope);
        case NODE_WHILE:
            return visit_while(tc, (WhileNode*)node, scope);
        case NODE_FOR:
            return visit_for(tc, (ForNode*)node, scope);
        case NODE_DESTRUCTURING:
            return visit_destr(tc, (DestrNode*)node, scope);
            
        // Binary operations
        case NODE_EQUAL:
        case NODE_NOT_EQUAL:
            return visit_equality_binary(tc, (EqualityBinaryNode*)node, scope);
        case NODE_LESS:
        case NODE_LESS_EQUAL:
        case NODE_GREATER:
        case NODE_GREATER_EQUAL:
            return visit_comparison_binary(tc, (ComparisonBinaryNode*)node, scope);
        case NODE_PLUS:
        case NODE_MINUS:
        case NODE_MULT:
        case NODE_DIV:
        case NODE_POW:
        case NODE_MOD:
            return visit_arithmetic_binary(tc, (ArithmeticBinaryNode*)node, scope);
        case NODE_AND:
        case NODE_OR:
            return visit_boolean_binary(tc, (BooleanBinaryNode*)node, scope);
        case NODE_CHECK_TYPE:
            return visit_check_type(tc, (CheckTypeNode*)node, scope);
        case NODE_CONCAT:
            return visit_string_binary(tc, (StringBinaryNode*)node, scope);
            
        // Atomic nodes
        case NODE_EXPRESSION_BLOCK:
            return visit_expression_block(tc, (ExpressionBlockNode*)node, scope);
        case NODE_CALL_FUNC:
            return visit_call_func(tc, (CallFuncNode*)node, scope);
        case NODE_TYPE_INSTANTIATION:
            return visit_type_instantiation(tc, (TypeInstantiationNode*)node, scope);
        case NODE_EXPLICIT_VECTOR:
            return visit_explicit_vector(tc, (ExplicitVectorNode*)node, scope);
        case NODE_IMPLICIT_VECTOR:
            return visit_implicit_vector(tc, (ImplicitVectorNode*)node, scope);
        case NODE_INDEX_OBJECT:
            return visit_index_object(tc, (IndexObjectNode*)node, scope);
        case NODE_CALL_METHOD:
            return visit_call_method(tc, (CallMethodNode*)node, scope);
        case NODE_CALL_TYPE_ATTRIBUTE:
            return visit_call_type_attribute(tc, (CallTypeAttributeNode*)node, scope);
        case NODE_CAST_TYPE:
            return visit_cast_type(tc, (CastTypeNode*)node, scope);
            
        // Unary operations
        case NODE_NOT:
            return visit_boolean_unary(tc, (BooleanUnaryNode*)node, scope);
        case NODE_POSITIVE:
        case NODE_NEGATIVE:
            return visit_arithmetic_unary(tc, (ArithmeticUnaryNode*)node, scope);
            
        // Literals
        case NODE_BOOLEAN:
            return visit_boolean_node(tc, (BooleanNode*)node, scope);
        case NODE_STRING:
            return visit_string_node(tc, (StringNode*)node, scope);
        case NODE_NUMBER:
            return visit_number_node(tc, (NumberNode*)node, scope);
        case NODE_VAR:
            return visit_var_node(tc, (VarNode*)node, scope);
        case NODE_SELF:
            return visit_var_node(tc, (VarNode*)node, scope);
            
        default:
            return get_type(tc->context, "<error>");
    }
}

