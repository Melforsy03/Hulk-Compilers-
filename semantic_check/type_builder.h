#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>

#include "semantic.h" 
#include "semanticErrors.h" 
#include "../ast_nodes/ast_nodes.h"

// Estructura del TypeBuilder
typedef struct {
    Context* context;
    Type* current_type;
    HulkErrorList* errors;
    int error_count;
} TypeBuilder;

//Prototipos de funciones
void visit_program(TypeBuilder* builder, ProgramNode* node);
void visit_type_declaration(TypeBuilder* builder, TypeDeclarationNode* node);
void visit_protocol_declaration(TypeBuilder* builder, ProtocolDeclarationNode* node);
void visit_function_declaration(TypeBuilder* builder, FunctionDeclarationNode* node);
void visit_type_attribute(TypeBuilder* builder, TypeAttributeNode* node);
void visit_method_declaration(TypeBuilder* builder, MethodDeclarationNode* node);
void visit_method_signature(TypeBuilder* builder, MethodSignatureNode* node);

// Implementación de las funciones visitantes

void visit_program(TypeBuilder* builder, ProgramNode* node) {
    for (int i = 0; i < node->declaration_count; i++) {
        DeclarationNode* decl = node->declarations[i];
        switch (decl->base.tipo) {
            case NODE_TYPE_DECLARATION:
                visit_type_declaration(builder, (TypeDeclarationNode*)decl);
                break;
            case NODE_PROTOCOL_DECLARATION:
                visit_protocol_declaration(builder, (ProtocolDeclarationNode*)decl);
                break;
            case NODE_FUNCTION_DECLARATION:
                visit_function_declaration(builder, (FunctionDeclarationNode*)decl);
                break;
        }
    }
}

void visit_type_attribute(TypeBuilder* builder, TypeAttributeNode* node) {
    Type* attr_type;
    
    if (node->type != NULL) {
        attr_type = context_get_type_or_protocol(builder->context, node->type).type; //ok
        if (strcmp(attr_type->name, "<error>") == 0) {
            char* error_msg = format_string(HULK_SEM_NOT_DEFINED_ATTRIBUTE_TYPE,
                                         node->type, node->name, builder->current_type->name);
            HulkSemanticError error;
            HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
            HulkErrorList_add(builder->errors, (HulkError*)&error);
            free(error_msg);
            attr_type = context_get_type(builder->context, "<error>");
        }
    } else {
        attr_type = context_get_type(builder->context, "<auto>");
    }

    Attribute* attr = define_attribute(builder->current_type, node->name, attr_type);
    if (attr == NULL) {
        char* error_msg = format_string("Attribute \"%s\" already defined in type \"%s\"", 
                                      node->name, builder->current_type->name);
        HulkSemanticError error;
        HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
        HulkErrorList_add(builder->errors, (HulkError*)&error);
        free(error_msg);
        set_attribute_error(builder->current_type, node->name);
    } else {
        attr->node = node;
    }
}

void visit_method_declaration(TypeBuilder* builder, MethodDeclarationNode* node) {
    Type* return_type;
    
    if (node->returnType != NULL) {
        return_type = context_get_type_or_protocol(builder->context, node->returnType).type;
        if (strcmp(return_type->name, "<error>") == 0) {
            char* error_msg = format_string(HULK_SEM_NOT_DEFINED_METHOD_RETURN_TYPE,
                                         node->returnType, node->name, builder->current_type->name);
            HulkSemanticError error;
            HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
            HulkErrorList_add(builder->errors, (HulkError*)&error);
            free(error_msg);
            return_type = context_get_type(builder->context, "<error>");
        }
    } else {
        return_type = context_get_type(builder->context, "<auto>");
    }

    char** param_names = malloc(sizeof(char*) * node->param_count);
    Type** param_types = malloc(sizeof(Type*) * node->param_count);
    
    for (int i = 0; i < node->param_count; i++) {
        Param* param = node->params[i];
        
        // Verificar nombres duplicados
        for (int j = 0; j < i; j++) {
            if (strcmp(param->name, param_names[j]) == 0) {
                char* error_msg = format_string("Method \"%s\" in type \"%s\" has more than one parameter named \"%s\"", 
                                              node->name, builder->current_type->name, param->name);
                HulkSemanticError error;
                HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
                HulkErrorList_add(builder->errors, (HulkError*)&error);
                free(error_msg);
                set_method_error(builder->current_type, node->name);
                free(param_names);
                free(param_types);
                return;
            }
        }
        
        param_names[i] = strdup(param->name);
        
        if (param->type_name != NULL) {
            param_types[i] = context_get_type_or_protocol(builder->context, param->type_name).type;
            if (strcmp(param_types[i]->name, "<error>") == 0) {
                char* error_msg = format_string(HULK_SEM_NOT_DEFINED_METHOD_PARAM_TYPE,
                                              param->type_name, param->name, node->name, builder->current_type->name);
                HulkSemanticError error;
                HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
                HulkErrorList_add(builder->errors, (HulkError*)&error);
                free(error_msg);
                param_types[i] = context_get_type(builder->context, "<error>");
            }
        } else {
            param_types[i] = context_get_type(builder->context, "<auto>");
        }
    }

    Method* method = define_method(builder->current_type, builder->context, node->name, param_names, 
                                  param_types, node->param_count, return_type);
    if (method == NULL) {
        set_method_error(builder->current_type, node->name);
    } else {
        method->node = node;
    }
}

void visit_method_signature(TypeBuilder* builder, MethodSignatureNode* node) {
    Type* return_type;
    
    if (node->returnType == NULL) {
        char* error_msg = format_string(HULK_SEM_NO_PROTOCOL_RETURN_TYPE,
                                      node->name, builder->current_type->name);
        HulkSemanticError error;
        HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
        HulkErrorList_add(builder->errors, (HulkError*)&error);
        free(error_msg);
        return_type = context_get_type(builder->context, "<error>");
    } else {
        return_type = context_get_type_or_protocol(builder->context, node->returnType).type;
        if (strcmp(return_type->name, "<error>") == 0) {
            char* error_msg = format_string(HULK_SEM_NOT_DEFINED_METHOD_RETURN_TYPE,
                                         node->returnType, node->name, builder->current_type->name);
            HulkSemanticError error;
            HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
            HulkErrorList_add(builder->errors, (HulkError*)&error);
            free(error_msg);
            return_type = context_get_type(builder->context, "<error>");
        }
    }

    char** param_names = malloc(sizeof(char*) * node->param_count);
    Type** param_types = malloc(sizeof(Type*) * node->param_count);
    
    for (int i = 0; i < node->param_count; i++) {
        Param* param = &node->params[i];
        
        // Verificar nombres duplicados
        for (int j = 0; j < i; j++) {
            if (strcmp(param->name, param_names[j]) == 0) {
                char* error_msg = format_string("Method \"%s\" in protocol \"%s\" has more than one parameter named \"%s\"", 
                                              node->name, builder->current_type->name, param->name);
                HulkSemanticError error;
                HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
                HulkErrorList_add(builder->errors, (HulkError*)&error);
                free(error_msg);
                set_method_error(builder->current_type, node->name);
                free(param_names);
                free(param_types);
                return;
            }
        }
        
        param_names[i] = strdup(param->name);
        
        if (param->type_name == NULL) {
            char* error_msg = format_string(HULK_SEM_NO_PROTOCOL_PARAM_TYPE,
                                         param->name, node->name, builder->current_type->name);
            HulkSemanticError error;
            HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
            HulkErrorList_add(builder->errors, (HulkError*)&error);
            free(error_msg);
            param_types[i] = context_get_type(builder->context, "<error>");
        } else {
            param_types[i] = context_get_type_or_protocol(builder->context, param->type_name).type;
            if (strcmp(param_types[i]->name, "<error>") == 0) {
                char* error_msg = format_string(HULK_SEM_NOT_DEFINED_METHOD_PARAM_TYPE,
                                             param->type_name, param->name, node->name, builder->current_type->name);
                HulkSemanticError error;
                HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
                HulkErrorList_add(builder->errors, (HulkError*)&error);
                free(error_msg);
                param_types[i] = context_get_type(builder->context, "<error>");
            }
        }
    }

    // Verificar si el tipo de retorno es nulo
    if (return_type == NULL) {
        char* error_msg = format_string("Return type for method \"%s\" cannot be NULL", node->name);
        HulkSemanticError error;
        HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
        HulkErrorList_add(builder->errors, (HulkError*)&error);
        free(error_msg);
        return;
    }
    
    Method* method = define_method(builder->current_type, builder->context, node->name, param_names, 
                                  param_types, node->param_count, return_type);
    if (method == NULL) {
        char* error_msg = format_string("Method \"%s\" already defined in protocol \"%s\"", 
                                      node->name, builder->current_type->name);
        HulkSemanticError error;
        HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
        HulkErrorList_add(builder->errors, (HulkError*)&error);
        free(error_msg);
        set_method_error(builder->current_type, node->name);
    } else {
        method->node = node;
    }
}

void visit_type_declaration(TypeBuilder* builder, TypeDeclarationNode* node) {
    // Verificar si es un tipo built-in
    bool is_hulk_type = false;
    for (int i = 0; i < builder->context->hulk_type_count; i++) {
        if (strcmp(node->name, builder->context->hulk_types[i]) == 0) {
            is_hulk_type = true;
            break;
        }
    }
    if (is_hulk_type) return;

    // Obtener o crear el tipo
    Type* current_type = context_get_type(builder->context, node->name);
    if (strcmp(current_type->name, "<error>") == 0) return;

    builder->current_type = current_type;

    // Procesar parámetros
    char** param_names = malloc(sizeof(char*) * node->param_count);
    Type** param_types = malloc(sizeof(Type*) * node->param_count);
    
    for (int i = 0; i < node->param_count; i++) {
        Param* param = node->params[i];
        
        // Verificar nombres duplicados
        for (int j = 0; j < i; j++) {
            if (strcmp(param->name, param_names[j]) == 0) {
                char* error_msg = format_string("Type %s has more than one parameter named %s", 
                                              node->name, param->name);
                HulkSemanticError error;
                HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
                HulkErrorList_add(builder->errors, (HulkError*)&error);
                free(error_msg);

                context_set_type_error(builder->context, node->name);
                free(param_names);
                free(param_types);
                return;
            }
        }
        
        param_names[i] = strdup(param->name);
        
        if (param->type_name != NULL) {
            param_types[i] = context_get_type_or_protocol(builder->context, param->type_name).type;

            if (strcmp(param_types[i]->name, "<error>") == 0) {
                char* error_msg = format_string(HULK_SEM_NOT_DEFINED_TYPE_CONSTRUCTOR_PARAM_TYPE,
                                              param->type_name, param->name, current_type->name);
                HulkSemanticError error;
                HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
                HulkErrorList_add(builder->errors, (HulkError*)&error);
                free(error_msg);
                param_types[i] = context_get_type(builder->context, "<error>");
            }
        } else {
            param_types[i] = context_get_type(builder->context, "<auto>");
        }
    }
    
    current_type->param_names = param_names;
    current_type->param_types = param_types;
    current_type->param_count = node->param_count;

    // Procesar métodos
    for (int i = 0; i < node->method_count; i++) {
        visit_method_declaration(builder, node->methods[i]);
    }

    // Procesar herencia
    Type* parent;
    if (strcmp(node->parent, "String") == 0 || 
        strcmp(node->parent, "Boolean") == 0 || 
        strcmp(node->parent, "Number") == 0) {
        char* error_msg = format_string(HULK_SEM_INVALID_INHERITANCE_FROM_DEFAULT_TYPE,
                                      current_type->name, node->parent);
        HulkSemanticError error;
        HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
        HulkErrorList_add(builder->errors, (HulkError*)&error);
        free(error_msg);
        parent = context_get_type(builder->context, "<error>");
    } else {
        parent = context_get_type(builder->context, node->parent);
        if (strcmp(parent->name, "<error>") == 0) {
            char* error_msg = format_string(HULK_SEM_NOT_DEFINED_PARENT_TYPE,
                                          node->parent, current_type->name);
            HulkSemanticError error;
            HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
            HulkErrorList_add(builder->errors, (HulkError*)&error);
            free(error_msg);
        } else if (conforms_to(parent, current_type)) {
            char* error_msg = format_string(HULK_SEM_INVALID_CIRCULAR_INHERITANCE,
                                          current_type->name, node->parent);
            HulkSemanticError error;
            HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
            HulkErrorList_add(builder->errors, (HulkError*)&error);
            free(error_msg);
            parent = context_get_type(builder->context, "<error>");
        }
    }
    current_type->parent = parent;

    // Procesar atributos
    for (int i = 0; i < node->attribute_count; i++) {
        visit_type_attribute(builder, node->attributes[i]);
    }
}

void visit_protocol_declaration(TypeBuilder* builder, ProtocolDeclarationNode* node) {
    // Verificar si es un protocolo built-in
    for (int i = 0; i < builder->context->hulk_protocol_count; i++) {
        if (strcmp(node->name, builder->context->hulk_protocols[i]) == 0) {
            return;
        }
    }

    Protocol* current_protocol = context_get_protocol(builder->context, node->name);
    if (strcmp(current_protocol->name, "<error>") == 0) return;

    builder->current_type = (Type*)current_protocol;

    // Procesar métodos
    for (int i = 0; i < node->method_count; i++) {
        visit_method_signature(builder, node->methods_signature[i]);
    }

    // Procesar herencia
    Type* parent;
    if (node->parent == NULL) {
        parent = context_get_type(builder->context, "Object");
    } else {
        parent = (Type*)context_get_protocol(builder->context, node->parent);
        if (strcmp(parent->name, "<error>") == 0) {
            char* error_msg = format_string(HULK_SEM_NOT_DEFINED_PARENT_TYPE,
                                          node->parent, current_protocol->name);
            HulkSemanticError error;
            HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
            HulkErrorList_add(builder->errors, (HulkError*)&error);
            free(error_msg);
        } else if (conforms_to(parent, (Type*)current_protocol)) {
            char* error_msg = format_string(HULK_SEM_INVALID_CIRCULAR_INHERITANCE,
                                          current_protocol->name, node->parent);
            HulkSemanticError error;
            HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
            HulkErrorList_add(builder->errors, (HulkError*)&error);
            free(error_msg);
            parent = context_get_type(builder->context, "<error>");
        }
    }
    current_protocol->parent = (Protocol*)parent;
}

void visit_function_declaration(TypeBuilder* builder, FunctionDeclarationNode* node) {
    builder->current_type = NULL;

    // Procesar tipo de retorno
    Type* return_type;
    if (node->returnType != NULL) {
        return_type = context_get_type_or_protocol(builder->context, node->returnType).type;
        if (strcmp(return_type->name, "<error>") == 0) {
            char* error_msg = format_string(HULK_SEM_NOT_DEFINDED_FUNCTION_RETURN_TYPE,
                                          node->returnType, node->name);
            HulkSemanticError error;
            HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
            HulkErrorList_add(builder->errors, (HulkError*)&error);
            free(error_msg);
            return_type = context_get_type(builder->context, "<error>");
        }
    } else {
        return_type = context_get_type(builder->context, "<auto>");
    }

    // Procesar parámetros
    char** param_names = malloc(sizeof(char*) * node->param_count);
    Type** param_types = malloc(sizeof(Type*) * node->param_count);
    
    for (int i = 0; i < node->param_count; i++) {
        Param* param = &node->params[i];
        
        // Verificar nombres duplicados
        for (int j = 0; j < i; j++) {
            if (strcmp(param->name, param_names[j]) == 0) {
                char* error_msg = format_string("Function \"%s\" has more than one parameter named \"%s\"", 
                                              node->name, param->name);
                HulkSemanticError error;
                HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
                HulkErrorList_add(builder->errors, (HulkError*)&error);
                free(error_msg);
                context_set_function_error(builder->context, node->name);
                free(param_names);
                free(param_types);
                return;
            }
        }
        
        param_names[i] = strdup(param->name);
        
        if (param->type_name != NULL) {
            param_types[i] = context_get_type_or_protocol(builder->context, param->type_name).type;
            if (strcmp(param_types[i]->name, "<error>") == 0) {
                char* error_msg = format_string(HULK_SEM_NOT_DEFINED_FUNCTION_PARAM_TYPE,
                                              param->type_name, param->name, node->name);
                HulkSemanticError error;
                HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
                HulkErrorList_add(builder->errors, (HulkError*)&error);
                free(error_msg);
                param_types[i] = context_get_type(builder->context, "<error>");
            }
        } else {
            param_types[i] = context_get_type(builder->context, "<auto>");
        }
    }

    // Crear la función
    Method* func = create_function(builder->context, node->name, param_names, param_types, 
                                  node->param_count, return_type);
    if (func == NULL) {
        char* error_msg = format_string("Function \"%s\" already defined", node->name);
        HulkSemanticError error;
        HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
        HulkErrorList_add(builder->errors, (HulkError*)&error);
        free(error_msg);
        
        // Verificar si no es una función built-in
        bool is_builtin = false;
        for (int i = 0; i < builder->context->hulk_function_count; i++) {
            if (strcmp(node->name, builder->context->hulk_functions[i]) == 0) {
                is_builtin = true;
                break;
            }
        }
        if (!is_builtin) {
            context_set_function_error(builder->context, node->name);
        }
    } else {
        func->node = node;
    }
}
