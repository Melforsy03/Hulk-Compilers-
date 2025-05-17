#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast_nodes.h"
#include "semantic.h"
#include "errors.h"
#include "scope.h"

//------------------------- TypeChecker -------------------------

typedef struct {
    Context* context;
    Type* current_type;
    Method* current_method;
    List* errors;
} TypeChecker;

//------------------------- Helper Functions -------------------------

Type* assign_type(TypeChecker* checker, Type* var_type, Type* expr_type) {
    if (type_is_auto(var_type)) {
        return expr_type;
    } else if (!type_is_error(var_type) && !type_is_error(expr_type) && !type_conforms_to(expr_type, var_type)) {
        return NULL; // Indica error de tipo
    }
    return var_type;
}

Type* get_lowest_common_ancestor(List* types) {
    // Implementación simplificada - en realidad necesitarías calcular el LCA en el árbol de herencia
    if (list_length(types) == 0) return error_type();
    
    Type* common = list_get(types, 0);
    for (int i = 1; i < list_length(types); i++) {
        Type* current = list_get(types, i);
        if (!type_conforms_to(current, common)) {
            if (type_conforms_to(common, current)) {
                common = current;
            } else {
                // Buscar ancestro común
                common = object_type();
            }
        }
    }
    return common;
}

//------------------------- Visitor Implementation -------------------------

void* type_checker_visit(TypeChecker* checker, Node* node, Scope* scope) {
    if (!node || !checker) return NULL;
    
    switch (node->tipo) {
        case NODE_PROGRAM:
            return type_checker_visit_program(checker, (ProgramNode*)node, scope);
        case NODE_TYPE_DEF:
            return type_checker_visit_type_declaration(checker, (TypeDeclarationNode*)node, scope);
        case NODE_TYPE_ATTRIBUTE:
            return type_checker_visit_type_attribute(checker, (TypeAttributeNode*)node, scope);
        case NODE_METHOD_DECLARATION:
            return type_checker_visit_method_declaration(checker, (MethodDeclarationNode*)node, scope);
        case NODE_FUNCTION_DEF:
            return type_checker_visit_function_declaration(checker, (FunctionDeclarationNode*)node, scope);
        case NODE_VAR_DECLARATION:
            return type_checker_visit_var_declaration(checker, (VarDeclarationNode*)node, scope);
        case NODE_CONDITIONAL:
            return type_checker_visit_conditional(checker, (ConditionalNode*)node, scope);
        case NODE_LET_IN:
            return type_checker_visit_let_in(checker, (LetInNode*)node, scope);
        case NODE_WHILE:
            return type_checker_visit_while(checker, (WhileNode*)node, scope);
        case NODE_FOR:
            return type_checker_visit_for(checker, (ForNode*)node, scope);
        case NODE_DESTR:
            return type_checker_visit_destr(checker, (DestrNode*)node, scope);
        case NODE_EQUALITY_BINARY:
            return type_checker_visit_equality_binary(checker, (EqualityBinaryNode*)node, scope);
        case NODE_COMPARISON_BINARY:
            return type_checker_visit_comparison_binary(checker, (ComparisonBinaryNode*)node, scope);
        case NODE_ARITHMETIC_BINARY:
            return type_checker_visit_arithmetic_binary(checker, (ArithmeticBinaryNode*)node, scope);
        case NODE_BOOLEAN_BINARY:
            return type_checker_visit_boolean_binary(checker, (BooleanBinaryNode*)node, scope);
        case NODE_CHECK_TYPE:
            return type_checker_visit_check_type(checker, (CheckTypeNode*)node, scope);
        case NODE_STRING_BINARY:
            return type_checker_visit_string_binary(checker, (StringBinaryNode*)node, scope);
        case NODE_EXPRESSION_BLOCK:
            return type_checker_visit_expression_block(checker, (ExpressionBlockNode*)node, scope);
        case NODE_CALL_FUNC:
            return type_checker_visit_call_func(checker, (CallFuncNode*)node, scope);
        case NODE_TYPE_INSTANTIATION:
            return type_checker_visit_type_instantiation(checker, (TypeInstantiationNode*)node, scope);
        case NODE_EXPLICIT_VECTOR:
            return type_checker_visit_explicit_vector(checker, (ExplicitVectorNode*)node, scope);
        case NODE_IMPLICIT_VECTOR:
            return type_checker_visit_implicit_vector(checker, (ImplicitVectorNode*)node, scope);
        case NODE_INDEX_OBJECT:
            return type_checker_visit_index_object(checker, (IndexObjectNode*)node, scope);
        case NODE_CALL_METHOD:
            return type_checker_visit_call_method(checker, (CallMethodNode*)node, scope);
        case NODE_CALL_TYPE_ATTRIBUTE:
            return type_checker_visit_call_type_attribute(checker, (CallTypeAttributeNode*)node, scope);
        case NODE_CAST_TYPE:
            return type_checker_visit_cast_type(checker, (CastTypeNode*)node, scope);
        case NODE_ARITHMETIC_UNARY:
            return type_checker_visit_arithmetic_unary(checker, (ArithmeticUnaryNode*)node, scope);
        case NODE_BOOLEAN_UNARY:
            return type_checker_visit_boolean_unary(checker, (BooleanUnaryNode*)node, scope);
        case NODE_BOOLEAN:
            return type_checker_visit_boolean(checker, (BooleanNode*)node, scope);
        case NODE_STRING:
            return type_checker_visit_string(checker, (StringNode*)node, scope);
        case NODE_NUMBER:
            return type_checker_visit_number(checker, (NumberNode*)node, scope);
        case NODE_VAR:
            return type_checker_visit_var(checker, (VarNode*)node, scope);
        default:
            return NULL;
    }
}

// Implementación de cada función visit_* ...

void* type_checker_visit_program(TypeChecker* checker, ProgramNode* node, Scope* scope) {
    node->base.scope = scope;
    
    // Visitar declaraciones
    ListNode* current = node->declarations;
    while (current) {
        DeclarationNode* decl = (DeclarationNode*)current->data;
        Scope* child_scope = scope_create_child(scope);
        try {
            type_checker_visit(checker, (Node*)decl, child_scope);
        } catch (SemanticError* error) {
            list_append(checker->errors, error);
        }
        current = current->next;
    }
    
    // Visitar expresión principal
    checker->current_type = NULL;
    checker->current_method = NULL;
    return type_checker_visit(checker, node->expression, scope);
}

void* type_checker_visit_type_declaration(TypeChecker* checker, TypeDeclarationNode* node, Scope* scope) {
    node->base.scope = scope;
    
    if (context_has_type(checker->context, node->name)) {
        return NULL;
    }
    
    checker->current_type = context_get_type(checker->context, node->name);
    if (type_is_error(checker->current_type)) {
        return NULL;
    }
    
    // Procesar parámetros del tipo
    if (list_length(checker->current_type->param_types) > 0) {
        for (int i = 0; i < list_length(checker->current_type->param_names); i++) {
            const char* name = list_get(checker->current_type->param_names, i);
            Type* type = list_get(checker->current_type->param_types, i);
            scope_define_variable(scope, name, type, true);
        }
        
        if (!type_is_error(checker->current_type->parent)) {
            List* parent_param_types = checker->current_type->parent->param_types;
            
            if (list_length(node->parent_args) != list_length(parent_param_types)) {
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg),
                        "Invalid number of arguments for parent type '%s': expected %d, got %d",
                        checker->current_type->parent->name, 
                        list_length(parent_param_types), 
                        list_length(node->parent_args));
                
                HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
                list_append(checker->errors, error);
            } else {
                for (int i = 0; i < list_length(parent_param_types); i++) {
                    Type* param_type = list_get(parent_param_types, i);
                    Node* arg = list_get(node->parent_args, i);
                    Type* arg_type = type_checker_visit(checker, arg, scope);
                    
                    if (!assign_type(checker, param_type, arg_type)) {
                        char error_msg[256];
                        snprintf(error_msg, sizeof(error_msg),
                                "Invalid type for argument %d: expected '%s', got '%s'",
                                i, param_type->name, arg_type->name);
                        
                        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
                        list_append(checker->errors, error);
                    }
                }
            }
        }
    }
    
    // Procesar atributos y métodos
    current = node->attributes;
    while (current) {
        TypeAttributeNode* attr = (TypeAttributeNode*)current->data;
        try {
            type_checker_visit(checker, (Node*)attr, scope);
        } catch (SemanticError* error) {
            HulkSemanticError* hulk_error = hulk_semantic_error_create(error, node->base.row, node->base.column);
            list_append(checker->errors, hulk_error);
        }
        current = current->next;
    }
    
    current = node->methods;
    while (current) {
        MethodDeclarationNode* method = (MethodDeclarationNode*)current->data;
        Scope* method_scope = scope_create_child(scope);
        try {
            type_checker_visit(checker, (Node*)method, method_scope);
        } catch (SemanticError* error) {
            HulkSemanticError* hulk_error = hulk_semantic_error_create(error, node->base.row, node->base.column);
            list_append(checker->errors, hulk_error);
        }
        current = current->next;
    }
    
    return NULL;
}

void* type_checker_visit_type_attribute(TypeChecker* checker, TypeAttributeNode* node, Scope* scope) {
    node->base.scope = scope;
    checker->current_method = NULL;
    
    Attribute* attribute = type_get_attribute(checker->current_type, node->name);
    Type* expr_type = type_checker_visit(checker, node->value, scope);
    
    if (!attribute_is_error(attribute)) {
        Type* var_type = attribute->type;
        if (!assign_type(checker, var_type, expr_type)) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                    "Incompatible types in attribute '%s': expected '%s', got '%s'",
                    node->name, var_type->name, expr_type->name);
            
            HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
            list_append(checker->errors, error);
        } else {
            attribute_set_type(attribute, var_type);
        }
    }
    
    return NULL;
}

void* type_checker_visit_method_declaration(TypeChecker* checker, MethodDeclarationNode* node, Scope* scope) {
    node->base.scope = scope;
    checker->current_method = type_get_method(checker->current_type, node->name);
    
    // Verificar override válido
    if (!type_is_error(checker->current_type->parent)) {
        Method* parent_method = type_get_method(checker->current_type->parent, node->name);
        if (parent_method && !method_equals(checker->current_method, parent_method)) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                    "Invalid override of method '%s' in type '%s'",
                    node->name, checker->current_type->name);
            
            HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
            list_append(checker->errors, error);
        }
    }
    
    if (method_is_error(checker->current_method)) {
        return NULL;
    }
    
    // Definir parámetros en el scope
    for (int i = 0; i < list_length(checker->current_method->param_names); i++) {
        const char* name = list_get(checker->current_method->param_names, i);
        Type* type = list_get(checker->current_method->param_types, i);
        scope_define_variable(scope, name, type, true);
    }
    
    // Verificar tipo de retorno
    Type* expr_type = type_checker_visit(checker, node->body, scope);
    if (!assign_type(checker, checker->current_method->return_type, expr_type)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Wrong return type in method '%s': expected '%s', got '%s'",
                node->name, checker->current_method->return_type->name, expr_type->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
    } else {
        method_set_inferred_return_type(checker->current_method, expr_type);
    }
    
    return NULL;
}

void* type_checker_visit_function_declaration(TypeChecker* checker, FunctionDeclarationNode* node, Scope* scope) {
    node->base.scope = scope;
    checker->current_type = NULL;
    
    if (context_has_function(checker->context, node->name)) {
        return NULL;
    }
    
    checker->current_method = context_get_function(checker->context, node->name);
    if (method_is_error(checker->current_method)) {
        return NULL;
    }
    
    // Definir parámetros en el scope
    for (int i = 0; i < list_length(checker->current_method->param_names); i++) {
        const char* name = list_get(checker->current_method->param_names, i);
        Type* type = list_get(checker->current_method->param_types, i);
        scope_define_variable(scope, name, type, true);
    }
    
    // Verificar tipo de retorno
    Type* expr_type = type_checker_visit(checker, node->body, scope);
    if (!assign_type(checker, checker->current_method->return_type, expr_type)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Wrong return type in function '%s': expected '%s', got '%s'",
                node->name, checker->current_method->return_type->name, expr_type->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
    } else {
        method_set_inferred_return_type(checker->current_method, expr_type);
    }
    
    checker->current_method = NULL;
    return NULL;
}

void* type_checker_visit_var_declaration(TypeChecker* checker, VarDeclarationNode* node, Scope* scope) {
    node->base.scope = scope;
    
    if (scope_is_local(scope, node->name)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Variable '%s' is already defined in this scope", node->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
        return NULL;
    }
    
    Type* var_type;
    if (node->type) {
        try {
            var_type = context_get_type_or_protocol(checker->context, node->type);
        } catch (SemanticError* error) {
            HulkSemanticError* hulk_error = hulk_semantic_error_create(error, node->base.row, node->base.column);
            list_append(checker->errors, hulk_error);
            var_type = error_type();
        }
    } else {
        var_type = auto_type();
    }
    
    Type* expr_type = type_checker_visit(checker, node->value, scope);
    if (!assign_type(checker, var_type, expr_type)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Incompatible types in variable '%s': expected '%s', got '%s'",
                node->name, var_type->name, expr_type->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
    }
    
    scope_define_variable(scope, node->name, var_type, false);
    return NULL;
}

void* type_checker_visit_conditional(TypeChecker* checker, ConditionalNode* node, Scope* scope) {
    node->base.scope = scope;
    List* expression_types = list_create();
    
    // Verificar condiciones
    ListNode* cond_current = node->conditions;
    while (cond_current) {
        Type* cond_type = type_checker_visit(checker, (Node*)cond_current->data, scope);
        if (!type_is_error(cond_type) && !type_is_auto(cond_type) && !type_equals(cond_type, boolean_type())) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                    "Condition must be boolean, got '%s'", cond_type->name);
            
            HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
            list_append(checker->errors, error);
        }
        cond_current = cond_current->next;
    }
    
    // Verificar expresiones
    ListNode* expr_current = node->expressions;
    while (expr_current) {
        Type* expr_type = type_checker_visit(checker, (Node*)expr_current->data, scope);
        list_append(expression_types, expr_type);
        expr_current = expr_current->next;
    }
    
    // Verificar expresión por defecto
    Type* default_type = type_checker_visit(checker, node->default_expre, scope);
    list_append(expression_types, default_type);
    
    return get_lowest_common_ancestor(expression_types);
}

void* type_checker_visit_let_in(TypeChecker* checker, LetInNode* node, Scope* scope) {
    node->base.scope = scope;
    Scope* let_scope = scope_create_child(scope);
    
    // Procesar declaraciones de variables
    ListNode* var_current = node->variables;
    while (var_current) {
        type_checker_visit(checker, (Node*)var_current->data, let_scope);
        var_current = var_current->next;
    }
    
    // Procesar cuerpo
    return type_checker_visit(checker, node->body, let_scope);
}

void* type_checker_visit_while(TypeChecker* checker, WhileNode* node, Scope* scope) {
    node->base.scope = scope;
    
    // Verificar condición
    Type* cond_type = type_checker_visit(checker, node->condition, scope);
    if (!type_is_error(cond_type) && !type_is_auto(cond_type) && !type_equals(cond_type, boolean_type())) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "While condition must be boolean, got '%s'", cond_type->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
    }
    
    // Verificar cuerpo
    return type_checker_visit(checker, node->body, scope);
}

void* type_checker_visit_for(TypeChecker* checker, ForNode* node, Scope* scope) {
    node->base.scope = scope;
    Scope* for_scope = scope_create_child(scope);
    
    // Verificar iterable
    Type* iterable_type = type_checker_visit(checker, node->iterable, scope);
    Protocol* iterable_protocol = context_get_protocol(checker->context, "Iterable");
    
    if (!type_conforms_to(iterable_type, (Type*)iterable_protocol) && 
        !type_is_error(iterable_type) && !type_is_auto(iterable_type)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "For loop iterable must conform to Iterable protocol, got '%s'", iterable_type->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
    }
    
    // Definir variable de iteración
    Type* item_type = type_is_auto(iterable_type) ? auto_type() : 
                     type_get_method(iterable_type, "current")->inferred_return_type;
    scope_define_variable(for_scope, node->item, item_type, false);
    
    // Verificar cuerpo
    return type_checker_visit(checker, node->body, for_scope);
}

void* type_checker_visit_destr(TypeChecker* checker, DestrNode* node, Scope* scope) {
    node->base.scope = scope;
    
    // Verificar variable
    Type* var_type = type_checker_visit(checker, node->var, scope);
    
    // Verificar expresión
    Type* expr_type = type_checker_visit(checker, node->expr, scope);
    
    // Verificar compatibilidad
    if (!type_is_error(var_type) && !type_is_error(expr_type) && 
        !type_is_auto(var_type) && !type_is_auto(expr_type) && 
        !type_conforms_to(expr_type, var_type)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Cannot assign expression of type '%s' to variable of type '%s'",
                expr_type->name, var_type->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
    }
    
    return var_type;
}


void* type_checker_visit_equality_binary(TypeChecker* checker, EqualityBinaryNode* node, Scope* scope) {
    node->base.scope = scope;
    Type* left_type = type_checker_visit(checker, node->left, scope);
    Type* right_type = type_checker_visit(checker, node->right, scope);
    
    if (type_is_error(left_type) || type_is_error(right_type)) {
        return boolean_type();
    }
    
    // Verificar tipos compatibles para comparación
    if ((!type_equals(left_type, right_type) && !type_is_auto(left_type) && !type_is_auto(right_type)) ||
        (!type_equals(left_type, number_type()) && !type_equals(left_type, boolean_type()) && 
         !type_is_auto(left_type) && !type_is_auto(right_type))) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Cannot compare types '%s' and '%s'", left_type->name, right_type->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
    }
    
    return boolean_type();
}

void* type_checker_visit_comparison_binary(TypeChecker* checker, ComparisonBinaryNode* node, Scope* scope) {
    node->base.scope = scope;
    Type* left_type = type_checker_visit(checker, node->left, scope);
    Type* right_type = type_checker_visit(checker, node->right, scope);
    
    if (type_is_error(left_type) || type_is_error(right_type)) {
        return boolean_type();
    }
    
    // Verificar que sean números o auto (que se inferirá como número)
    if (!type_equals(left_type, number_type()) && !type_is_auto(left_type)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Comparison operator requires numbers, got '%s'", left_type->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
    }
    
    if (!type_equals(right_type, number_type()) && !type_is_auto(right_type)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Comparison operator requires numbers, got '%s'", right_type->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
    }
    
    return boolean_type();
}

void* type_checker_visit_arithmetic_binary(TypeChecker* checker, ArithmeticBinaryNode* node, Scope* scope) {
    node->base.scope = scope;
    Type* left_type = type_checker_visit(checker, node->left, scope);
    Type* right_type = type_checker_visit(checker, node->right, scope);
    
    if (type_is_error(left_type) || type_is_error(right_type)) {
        return number_type();
    }
    
    // Verificar que sean números o auto (que se inferirá como número)
    if (!type_equals(left_type, number_type()) && !type_is_auto(left_type)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Arithmetic operator requires numbers, got '%s'", left_type->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
    }
    
    if (!type_equals(right_type, number_type()) && !type_is_auto(right_type)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Arithmetic operator requires numbers, got '%s'", right_type->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
    }
    
    return number_type();
}

void* type_checker_visit_boolean_binary(TypeChecker* checker, BooleanBinaryNode* node, Scope* scope) {
    node->base.scope = scope;
    Type* left_type = type_checker_visit(checker, node->left, scope);
    Type* right_type = type_checker_visit(checker, node->right, scope);
    
    if (type_is_error(left_type) || type_is_error(right_type)) {
        return boolean_type();
    }
    
    // Verificar que sean booleanos o auto (que se inferirá como booleano)
    if (!type_equals(left_type, boolean_type()) && !type_is_auto(left_type)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Boolean operator requires booleans, got '%s'", left_type->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
    }
    
    if (!type_equals(right_type, boolean_type()) && !type_is_auto(right_type)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Boolean operator requires booleans, got '%s'", right_type->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
    }
    
    return boolean_type();
}

void* type_checker_visit_check_type(TypeChecker* checker, CheckTypeNode* node, Scope* scope) {
    node->base.scope = scope;
    
    // Verificar expresión izquierda
    Type* left_type = type_checker_visit(checker, node->left, scope);
    if (type_is_error(left_type)) {
        return boolean_type();
    }
    
    // Verificar que el tipo derecho exista
    try {
        context_get_type_or_protocol(checker->context, node->right);
    } catch (SemanticError* error) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Type '%s' in 'is' operation is not defined", node->right);
        
        HulkSemanticError* hulk_error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, hulk_error);
    }
    
    return boolean_type();
}

void* type_checker_visit_string_binary(TypeChecker* checker, StringBinaryNode* node, Scope* scope) {
    node->base.scope = scope;
    Type* left_type = type_checker_visit(checker, node->left, scope);
    Type* right_type = type_checker_visit(checker, node->right, scope);
    
    // Verificar que sean tipos que puedan convertirse a string
    if ((!type_equals(left_type, string_type()) && !type_equals(left_type, number_type()) && 
        !type_equals(left_type, boolean_type()) && !type_is_auto(left_type) && !type_is_error(left_type))) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Cannot concatenate type '%s'", left_type->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
    }
    
    if ((!type_equals(right_type, string_type()) && !type_equals(right_type, number_type()) && 
        !type_equals(right_type, boolean_type()) && !type_is_auto(right_type) && !type_is_error(right_type))) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Cannot concatenate type '%s'", right_type->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
    }
    
    return string_type();
}


void* type_checker_visit_expression_block(TypeChecker* checker, ExpressionBlockNode* node, Scope* scope) {
    node->base.scope = scope;
    Scope* block_scope = scope_create_child(scope);
    Type* last_type = void_type();
    
    ListNode* expr_current = node->expressions;
    while (expr_current) {
        last_type = type_checker_visit(checker, (Node*)expr_current->data, block_scope);
        expr_current = expr_current->next;
    }
    
    return last_type;
}

void* type_checker_visit_call_func(TypeChecker* checker, CallFuncNode* node, Scope* scope) {
    node->base.scope = scope;
    
    // Caso especial para 'base' sin argumentos
    if (strcmp(node->name, "base") == 0 && list_length(node->arguments) == 0 && 
        checker->current_type != NULL && checker->current_method != NULL) {
        try {
            Method* parent_method = type_get_method(checker->current_type->parent, checker->current_method->name);
            return parent_method->inferred_return_type;
        } catch (SemanticError* error) {
            HulkSemanticError* hulk_error = hulk_semantic_error_create(error, node->base.row, node->base.column);
            list_append(checker->errors, hulk_error);
            return error_type();
        }
    }
    
    // Obtener la función
    Function* function;
    try {
        function = context_get_function(checker->context, node->name);
    } catch (SemanticError* error) {
        HulkSemanticError* hulk_error = hulk_semantic_error_create(error, node->base.row, node->base.column);
        list_append(checker->errors, hulk_error);
        return error_type();
    }
    
    if (function_is_error(function)) {
        return error_type();
    }
    
    // Verificar número de argumentos
    if (list_length(node->arguments) != list_length(function->param_types)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Function '%s' expects %d arguments, got %d",
                node->name, list_length(function->param_types), list_length(node->arguments));
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
        return function->inferred_return_type;
    }
    
    // Verificar tipos de argumentos
    for (int i = 0; i < list_length(node->arguments); i++) {
        Node* arg = list_get(node->arguments, i);
        Type* arg_type = type_checker_visit(checker, arg, scope);
        Type* param_type = list_get(function->param_types, i);
        
        if (!assign_type(checker, param_type, arg_type)) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                    "Argument %d of function '%s' expects type '%s', got '%s'",
                    i + 1, node->name, param_type->name, arg_type->name);
            
            HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
            list_append(checker->errors, error);
        }
    }
    
    // Caso especial para 'print' que devuelve el tipo de su argumento
    if (strcmp(node->name, "print") == 0 && list_length(node->arguments) > 0) {
        return type_checker_visit(checker, list_get(node->arguments, 0), scope);
    }
    
    return function->inferred_return_type;
}

void* type_checker_visit_type_instantiation(TypeChecker* checker, TypeInstantiationNode* node, Scope* scope) {
    node->base.scope = scope;
    
    // Obtener el tipo que se está instanciando
    Type* type;
    try {
        type = context_get_type(checker->context, node->type_name);
    } catch (SemanticError* error) {
        HulkSemanticError* hulk_error = hulk_semantic_error_create(error, node->base.row, node->base.column);
        list_append(checker->errors, hulk_error);
        return error_type();
    }
    
    if (type_is_error(type)) {
        return error_type();
    }
    
    // Verificar número de argumentos
    if (list_length(node->arguments) != list_length(type->param_types)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Type '%s' expects %d type arguments, got %d",
                node->type_name, list_length(type->param_types), list_length(node->arguments));
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
        return error_type();
    }
    
    // Verificar tipos de argumentos
    for (int i = 0; i < list_length(node->arguments); i++) {
        Node* arg = list_get(node->arguments, i);
        Type* arg_type = type_checker_visit(checker, arg, scope);
        Type* param_type = list_get(type->param_types, i);
        
        if (!assign_type(checker, param_type, arg_type)) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                    "Type argument %d of '%s' expects type '%s', got '%s'",
                    i + 1, node->type_name, param_type->name, arg_type->name);
            
            HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
            list_append(checker->errors, error);
        }
    }
    
    return type;
}

void* type_checker_visit_explicit_vector(TypeChecker* checker, ExplicitVectorNode* node, Scope* scope) {
    node->base.scope = scope;
    
    // Verificar que el tipo base existe
    Type* base_type;
    try {
        base_type = context_get_type(checker->context, node->base_type);
    } catch (SemanticError* error) {
        HulkSemanticError* hulk_error = hulk_semantic_error_create(error, node->base.row, node->base.column);
        list_append(checker->errors, hulk_error);
        return error_type();
    }
    
    if (type_is_error(base_type)) {
        return error_type();
    }
    
    // Verificar que todos los elementos sean del mismo tipo o conformen al tipo base
    Type* common_type = base_type;
    ListNode* current = node->elements;
    while (current) {
        Node* element = (Node*)current->data;
        Type* element_type = type_checker_visit(checker, element, scope);
        
        if (!type_is_error(element_type) && !type_conforms_to(element_type, common_type)) {
            if (type_is_auto(common_type)) {
                common_type = element_type;
            } else {
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg),
                        "Vector element type '%s' does not conform to declared type '%s'",
                        element_type->name, common_type->name);
                
                HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
                list_append(checker->errors, error);
            }
        }
        
        current = current->next;
    }
    
    // Crear tipo de vector (simplificado - en realidad necesitarías un tipo Vector parametrizado)
    return vector_type(common_type);
}

void* type_checker_visit_implicit_vector(TypeChecker* checker, ImplicitVectorNode* node, Scope* scope) {
    node->base.scope = scope;
    
    // Determinar el tipo común de todos los elementos
    List* element_types = list_create();
    ListNode* current = node->elements;
    while (current) {
        Node* element = (Node*)current->data;
        Type* element_type = type_checker_visit(checker, element, scope);
        list_append(element_types, element_type);
        current = current->next;
    }
    
    Type* common_type = get_lowest_common_ancestor(element_types);
    
    // Verificar que todos los elementos sean compatibles con el tipo común
    for (int i = 0; i < list_length(element_types); i++) {
        Type* element_type = list_get(element_types, i);
        if (!type_is_error(element_type) && !type_conforms_to(element_type, common_type)) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                    "Incompatible vector element types: cannot unify '%s' and '%s'",
                    element_type->name, common_type->name);
            
            HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
            list_append(checker->errors, error);
        }
    }
    
    list_free(element_types);
    return vector_type(common_type);
}

void* type_checker_visit_index_object(TypeChecker* checker, IndexObjectNode* node, Scope* scope) {
    node->base.scope = scope;
    
    // Verificar el objeto que se está indexando
    Type* obj_type = type_checker_visit(checker, node->obj, scope);
    
    // Verificar que el objeto sea un vector o tenga un método 'get'
    Protocol* indexable_protocol = context_get_protocol(checker->context, "Indexable");
    if (!type_conforms_to(obj_type, (Type*)indexable_protocol) && !type_is_error(obj_type)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Type '%s' is not indexable", obj_type->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
        return error_type();
    }
    
    // Verificar el índice
    Type* index_type = type_checker_visit(checker, node->index, scope);
    if (!type_equals(index_type, number_type()) && !type_is_error(index_type)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Index must be a number, got '%s'", index_type->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
    }
    
    // Obtener el tipo de retorno (para vectores, el tipo de elemento)
    if (type_is_vector(obj_type)) {
        return vector_get_element_type(obj_type);
    } else {
        // Para otros tipos indexables, usar el tipo de retorno del método 'get'
        Method* get_method = type_get_method(obj_type, "get");
        return get_method ? get_method->inferred_return_type : error_type();
    }
}

void* type_checker_visit_call_method(TypeChecker* checker, CallMethodNode* node, Scope* scope) {
    node->base.scope = scope;
    
    // Verificar el objeto receptor
    Type* receiver_type = type_checker_visit(checker, node->receiver, scope);
    
    // Obtener el método
    Method* method;
    try {
        method = type_get_method(receiver_type, node->method_name);
    } catch (SemanticError* error) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Type '%s' has no method '%s'", receiver_type->name, node->method_name);
        
        HulkSemanticError* hulk_error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, hulk_error);
        return error_type();
    }
    
    if (method_is_error(method)) {
        return error_type();
    }
    
    // Verificar número de argumentos
    if (list_length(node->arguments) != list_length(method->param_types)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Method '%s' expects %d arguments, got %d",
                node->method_name, list_length(method->param_types), list_length(node->arguments));
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
        return method->inferred_return_type;
    }
    
    // Verificar tipos de argumentos
    for (int i = 0; i < list_length(node->arguments); i++) {
        Node* arg = list_get(node->arguments, i);
        Type* arg_type = type_checker_visit(checker, arg, scope);
        Type* param_type = list_get(method->param_types, i);
        
        if (!assign_type(checker, param_type, arg_type)) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                    "Argument %d of method '%s' expects type '%s', got '%s'",
                    i + 1, node->method_name, param_type->name, arg_type->name);
            
            HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
            list_append(checker->errors, error);
        }
    }
    
    return method->inferred_return_type;
}

void* type_checker_visit_call_type_attribute(TypeChecker* checker, CallTypeAttributeNode* node, Scope* scope) {
    node->base.scope = scope;
    
    // Verificar el objeto receptor
    Type* receiver_type = type_checker_visit(checker, node->receiver, scope);
    
    // Obtener el atributo
    Attribute* attribute;
    try {
        attribute = type_get_attribute(receiver_type, node->attribute_name);
    } catch (SemanticError* error) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Type '%s' has no attribute '%s'", receiver_type->name, node->attribute_name);
        
        HulkSemanticError* hulk_error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, hulk_error);
        return error_type();
    }
    
    if (attribute_is_error(attribute)) {
        return error_type();
    }
    
    return attribute->type;
}

void* type_checker_visit_cast_type(TypeChecker* checker, CastTypeNode* node, Scope* scope) {
    node->base.scope = scope;
    
    // Verificar la expresión a castear
    Type* expr_type = type_checker_visit(checker, node->expr, scope);
    
    // Verificar que el tipo destino exista
    Type* target_type;
    try {
        target_type = context_get_type(checker->context, node->target_type);
    } catch (SemanticError* error) {
        HulkSemanticError* hulk_error = hulk_semantic_error_create(error, node->base.row, node->base.column);
        list_append(checker->errors, hulk_error);
        return error_type();
    }
    
    if (type_is_error(target_type)) {
        return error_type();
    }
    
    // Verificar que el casteo sea posible
    if (!type_is_error(expr_type) && !type_conforms_to(expr_type, target_type)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Cannot cast from '%s' to '%s'", expr_type->name, target_type->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
    }
    
    return target_type;
}

void* type_checker_visit_arithmetic_unary(TypeChecker* checker, ArithmeticUnaryNode* node, Scope* scope) {
    node->base.scope = scope;
    Type* expr_type = type_checker_visit(checker, node->expr, scope);
    
    if (type_is_error(expr_type)) {
        return number_type();
    }
    
    // Verificar que la expresión sea un número o auto (que se inferirá como número)
    if (!type_equals(expr_type, number_type()) && !type_is_auto(expr_type)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Arithmetic unary operator requires a number, got '%s'", expr_type->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
    }
    
    return number_type();
}

void* type_checker_visit_boolean_unary(TypeChecker* checker, BooleanUnaryNode* node, Scope* scope) {
    node->base.scope = scope;
    Type* expr_type = type_checker_visit(checker, node->expr, scope);
    
    if (type_is_error(expr_type)) {
        return boolean_type();
    }
    
    // Verificar que la expresión sea un booleano o auto (que se inferirá como booleano)
    if (!type_equals(expr_type, boolean_type()) && !type_is_auto(expr_type)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Boolean unary operator requires a boolean, got '%s'", expr_type->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
    }
    
    return boolean_type();
}

void* type_checker_visit_boolean(TypeChecker* checker, BooleanNode* node, Scope* scope) {
    node->base.scope = scope;
    return boolean_type();
}

void* type_checker_visit_string(TypeChecker* checker, StringNode* node, Scope* scope) {
    node->base.scope = scope;
    return string_type();
}

void* type_checker_visit_number(TypeChecker* checker, NumberNode* node, Scope* scope) {
    node->base.scope = scope;
    return number_type();
}

void* type_checker_visit_var(TypeChecker* checker, VarNode* node, Scope* scope) {
    node->base.scope = scope;
    
    // Caso especial para 'self'
    if (strcmp(node->lex, "self") == 0) {
        if (checker->current_type != NULL && checker->current_method != NULL) {
            return checker->current_type;
        } else {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                    "'self' is not defined in this context");
            
            HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
            list_append(checker->errors, error);
            return error_type();
        }
    }
    
    // Buscar variable en el scope
    Variable* var = scope_find_variable(scope, node->lex);
    if (var == NULL) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Variable '%s' is not defined", node->lex);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->base.row, node->base.column);
        list_append(checker->errors, error);
        return error_type();
    }
    
    return var->type;
}
//------------------------- Public Interface -------------------------

TypeChecker* type_checker_create(Context* context, List* errors) {
    TypeChecker* checker = (TypeChecker*)malloc(sizeof(TypeChecker));
    if (!checker) return NULL;
    
    checker->context = context;
    checker->current_type = NULL;
    checker->current_method = NULL;
    checker->errors = errors;
    
    return checker;
}

void type_checker_free(TypeChecker* checker) {
    if (checker) {
        free(checker);
    }
}

void type_checker_run(TypeChecker* checker, ProgramNode* ast) {
    Scope* global_scope = scope_create();
    type_checker_visit(checker, (Node*)ast, global_scope);
    scope_free(global_scope);
}