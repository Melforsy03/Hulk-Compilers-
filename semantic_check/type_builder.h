#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>

#include "semantic.h" 
#include "semantic_errors.h" 
#include "../parser/ast_nodes.h"

// Estructura del TypeBuilder
typedef struct {
    Context* context;
    Type* current_type;
    HulkErrorList* errors;
    int error_count;
} TypeBuilder;

//Prototipos de funciones
void visit_program(TypeBuilder* builder, Node* node); //ok
void visit_type_declaration(TypeBuilder* builder, Node* node, char* node_parent); 
void visit_protocol_declaration(TypeBuilder* builder, Node* node, char* node_parent); 
void visit_function_declaration(TypeBuilder* builder, Node* node); //ok pero revisar conflicto type-protocol
void visit_type_attribute(TypeBuilder* builder, Node* node); //ok pero revisar conflicto type-protocol
void visit_method_declaration(TypeBuilder* builder, Node* node); //ok pero revisar conflicto type-protocol
void visit_method_signature(TypeBuilder* builder, Node* node); //ok pero revisar conflicto type-protocol

// Implementación de las funciones visitantes

void visit_program(TypeBuilder* builder, Node* node) {

    for (int i = 0; i < node->child_count; i++) {
        Node* child = node->children[i];
        switch (child->tipo) {
            case NODE_TYPE_DECLARATION:
                visit_type_declaration(builder, child, node->symbol->name);
                break;
            case NODE_PROTOCOL_DECLARATION:
                visit_protocol_declaration(builder, child, node->symbol->name);
                break;
            case NODE_FUNCTION_DECLARATION:
                visit_function_declaration(builder, child);
                break;
        }
    }
}

void visit_type_declaration(TypeBuilder* builder, Node* node, char* node_parent) {
    // Verificar si es un tipo built-in
    bool is_hulk_type = false;
    for (int i = 0; i < builder->context->hulk_type_count; i++) {
        if (strcmp(node->symbol->name, builder->context->hulk_types[i]) == 0) {
            is_hulk_type = true;
            break;
        }
    }
    if (is_hulk_type) return;

    // Obtener el tipo
    Type* current_type = context_get_type(builder->context, node->symbol->name);
    if (strcmp(current_type->name, "<error>") == 0) return;

    builder->current_type = current_type;

    // Procesar parámetros
    int param_count = node->child_count-1;
    char** param_names = malloc(sizeof(char*) * param_count);
    Type** param_types = malloc(sizeof(Type*) * param_count);
    
    for (int i = 0; i < param_count; i++) {
        Node* child = node->children[i];
        
        // Verificar nombres duplicados
        for (int j = 0; j < i; j++) {
            if (strcmp(child->lexeme, param_names[j]) == 0) {
                char* error_msg = format_string("Type %s has more than one parameter named %s", 
                                              node->symbol->name, node->lexeme);
                HulkSemanticError error;
                HulkSemanticError_init(&error, error_msg, node->row, node->column);
                HulkErrorList_add(builder->errors, (HulkError*)&error);
                free(error_msg);

                context_set_type_error(builder->context, node->lexeme);
                free(param_names);
                free(param_types);
                return;
            }
        }
        
        param_names[i] = strdup(child->lexeme);
        
        if (child->symbol->name != NULL) {
            param_types[i] = context_get_type_or_protocol(builder->context, child->symbol->name);

            if (strcmp(param_types[i]->name, "<error>") == 0) {
                char* error_msg = format_string(HULK_SEM_NOT_DEFINED_TYPE_CONSTRUCTOR_PARAM_TYPE,
                                              child->symbol->name, child->lexeme, current_type->name);
                HulkSemanticError error;
                HulkSemanticError_init(&error, error_msg, node->row, node->column);
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
    current_type->param_count = param_count;

    // Procesar métodos
    for (int i = 0; i < node->child_count; i++) {
        if(node->children[i]->tipo == NODE_METHOD_DECLARATION){
            visit_method_declaration(builder, node->children[i]);
        }
        
    }

    // Procesar herencia
    Type* parent;
    if (strcmp(node_parent, "String") == 0 || 
        strcmp(node_parent, "Boolean") == 0 || 
        strcmp(node_parent, "Number") == 0) {
        char* error_msg = format_string(HULK_SEM_INVALID_INHERITANCE_FROM_DEFAULT_TYPE,
                                      current_type->name, node_parent);
        HulkSemanticError error;
        HulkSemanticError_init(&error, error_msg, node->row, node->column);
        HulkErrorList_add(builder->errors, (HulkError*)&error);
        free(error_msg);
        parent = context_get_type(builder->context, "<error>");
    } else {
        parent = context_get_type(builder->context, node_parent);
        if (strcmp(parent->name, "<error>") == 0) {
            char* error_msg = format_string(HULK_SEM_NOT_DEFINED_PARENT_TYPE,
                                          node_parent, current_type->name);
            HulkSemanticError error;
            HulkSemanticError_init(&error, error_msg, node->row, node->column);
            HulkErrorList_add(builder->errors, (HulkError*)&error);
            free(error_msg);
        } else if (conforms_to(parent, current_type)) {
            char* error_msg = format_string(HULK_SEM_INVALID_CIRCULAR_INHERITANCE,
                                          current_type->name, node_parent);
            HulkSemanticError error;
            HulkSemanticError_init(&error, error_msg, node->row, node->column);
            HulkErrorList_add(builder->errors, (HulkError*)&error);
            free(error_msg);
            parent = context_get_type(builder->context, "<error>");
        }
    }
    current_type->parent = parent;

    // Procesar atributos
    for (int i = 0; i < node->child_count; i++) {
        if(node->children[i]->tipo == NODE_TYPE_ATTRIBUTE){
            visit_type_attribute(builder, node->children[i]);
        }
        
    }
}

void visit_protocol_declaration(TypeBuilder* builder, Node* node, char* node_parent) {
    // Verificar si es un protocolo built-in
    for (int i = 0; i < builder->context->hulk_protocol_count; i++) {
        if (strcmp(node->symbol->name, builder->context->hulk_protocols[i]) == 0) {
            return;
        }
    }

    Protocol* current_protocol = context_get_protocol(builder->context, node->symbol->name);
    if (strcmp(current_protocol->name, "<error>") == 0) return;

    builder->current_type = (Type*)current_protocol;

    // Procesar métodos
    for (int i = 0; i < node->child_count; i++) {
        if(node->children[i]->tipo == NODE_METHOD_SIGNATURE){
            visit_method_signature(builder, node->children[i]);
        }
        
    }

    // Procesar herencia
    Type* parent;
    if (node_parent == NULL) {
        parent = context_get_type(builder->context, "Object");
    } else {
        parent = (Type*)context_get_protocol(builder->context, node_parent);
        if (strcmp(node_parent, "<error>") == 0) {
            char* error_msg = format_string(HULK_SEM_NOT_DEFINED_PARENT_TYPE,
                                          node_parent, current_protocol->name);
            HulkSemanticError error;
            HulkSemanticError_init(&error, error_msg, node->row, node->column);
            HulkErrorList_add(builder->errors, (HulkError*)&error);
            free(error_msg);
        } else if (conforms_to(parent, (Type*)current_protocol)) {
            char* error_msg = format_string(HULK_SEM_INVALID_CIRCULAR_INHERITANCE,
                                          current_protocol->name, node_parent);
            HulkSemanticError error;
            HulkSemanticError_init(&error, error_msg, node->row, node->column);
            HulkErrorList_add(builder->errors, (HulkError*)&error);
            free(error_msg);
            parent = context_get_type(builder->context, "<error>");
        }
    }
    current_protocol->parent = context_get_protocol(builder->context, parent->name);
}

void visit_function_declaration(TypeBuilder* builder, Node* node) {
    builder->current_type = NULL;

    // Procesar tipo de retorno
    Type* return_type;
    if (node->symbol->name != NULL) {
        return_type = context_get_type_or_protocol(builder->context, node->symbol->name);
        if (strcmp(return_type->name, "<error>") == 0) {
            char* error_msg = format_string(HULK_SEM_NOT_DEFINDED_FUNCTION_RETURN_TYPE,
                                          node->symbol->name, node->lexeme);
            HulkSemanticError error;
            HulkSemanticError_init(&error, error_msg, node->row, node->column);
            HulkErrorList_add(builder->errors, (HulkError*)&error);
            free(error_msg);
            return_type = context_get_type(builder->context, "<error>");
        }
    } else {
        return_type = context_get_type(builder->context, "<auto>");
    }

    // Procesar parámetros
    int param_count = node->child_count-1;
    char** param_names = malloc(sizeof(char*) * param_count);
    Type** param_types = malloc(sizeof(Type*) * param_count);
    
    for (int i = 0; i < param_count; i++) {
        Node* child = node->children[i];
        
        // Verificar nombres duplicados
        for (int j = 0; j < i; j++) {
            if (strcmp(child->lexeme, param_names[j]) == 0) {
            char* error_msg = format_string("Function \"%s\" has more than one parameter named \"%s\"", 
                                                node->symbol->name, child->lexeme);
            HulkSemanticError error;
            HulkSemanticError_init(&error, error_msg, node->row, node->column);
            HulkErrorList_add(builder->errors, (HulkError*)&error);
            free(error_msg);
            context_set_function_error(builder->context, node->symbol->name);
            free(param_names);
            free(param_types);
            return;
            }
        }    
        
        param_names[i] = strdup(child->lexeme);
        
        if (child->symbol->name != NULL) {
            param_types[i] = context_get_type_or_protocol(builder->context, child->symbol->name);
            if (strcmp(param_types[i]->name, "<error>") == 0) {
                char* error_msg = format_string(HULK_SEM_NOT_DEFINED_FUNCTION_PARAM_TYPE,
                                              child->lexeme, child->symbol->name, node->symbol->name);
                HulkSemanticError error;
                HulkSemanticError_init(&error, error_msg, node->row, node->column);
                HulkErrorList_add(builder->errors, (HulkError*)&error);
                free(error_msg);
                param_types[i] = context_get_type(builder->context, "<error>");
            }
        } else {
            param_types[i] = context_get_type(builder->context, "<auto>");
        }
    }

    // Crear la función
    Method* func = create_function(builder->context, node->symbol->name, param_names, param_types, 
                                  param_count, return_type);
    if (func == NULL) {
        char* error_msg = format_string("Function \"%s\" already defined", node->symbol->name);
        HulkSemanticError error;
        HulkSemanticError_init(&error, error_msg, node->row, node->column);
        HulkErrorList_add(builder->errors, (HulkError*)&error);
        free(error_msg);
        
        // Verificar si no es una función built-in
        bool is_builtin = false;
        for (int i = 0; i < builder->context->hulk_function_count; i++) {
            if (strcmp(node->symbol->name, builder->context->hulk_functions[i]) == 0) {
                is_builtin = true;
                break;
            }
        }
        if (!is_builtin) {
            context_set_function_error(builder->context, node->symbol->name);
        }
    } else {
        func->node = node;
    }
}

void visit_type_attribute(TypeBuilder* builder, Node* node) {
    Type* attr_type;
    
    if (node->symbol->name != NULL) {
        attr_type = context_get_type_or_protocol(builder->context, node->symbol->name); //ok
        if (strcmp(attr_type->name, "<error>") == 0) {
            char* error_msg = format_string(HULK_SEM_NOT_DEFINED_ATTRIBUTE_TYPE,
                                         node->symbol->name, node->lexeme, builder->current_type->name);
            HulkSemanticError error;
            HulkSemanticError_init(&error, error_msg, node->row, node->column);
            HulkErrorList_add(builder->errors, (HulkError*)&error);
            free(error_msg);
            attr_type = context_get_type(builder->context, "<error>");
        }
    } else {
        attr_type = context_get_type(builder->context, "<auto>");
    }

    Attribute* attr = define_attribute(builder->current_type, node->symbol->name, attr_type);
    if (attr == NULL) {
        char* error_msg = format_string("Attribute \"%s\" already defined in type \"%s\"", 
                                      node->lexeme, builder->current_type->name);
        HulkSemanticError error;
        HulkSemanticError_init(&error, error_msg, node->row, node->column);
        HulkErrorList_add(builder->errors, (HulkError*)&error);
        free(error_msg);
        set_attribute_error(builder->current_type, node->symbol->name);
    } else {
        attr->node = node;
    }
}

void visit_method_declaration(TypeBuilder* builder, Node* node) {
    Type* return_type;
    
    if (node->symbol->name != NULL) {
        return_type = context_get_type_or_protocol(builder->context, node->symbol->name);
        if (strcmp(return_type->name, "<error>") == 0) {
            char* error_msg = format_string(HULK_SEM_NOT_DEFINED_METHOD_RETURN_TYPE,
                                         node->symbol->name, node->lexeme, builder->current_type->name);
            HulkSemanticError error;
            HulkSemanticError_init(&error, error_msg, node->row, node->column);
            HulkErrorList_add(builder->errors, (HulkError*)&error);
            free(error_msg);
            return_type = context_get_type(builder->context, "<error>");
        }
    } else {
        return_type = context_get_type(builder->context, "<auto>");
    }

    //procesado de parametros
    int param_count = node->child_count-1;
    char** param_names = malloc(sizeof(char*) * param_count);
    Type** param_types = malloc(sizeof(Type*) * param_count);
    

    for (int i = 0; i < param_count; i++) {
        Node* param = node->children[i];
        
        // Verificar nombres duplicados
        for (int j = 0; j < i; j++) {
            if (strcmp(param->lexeme, param_names[j]) == 0) {
                char* error_msg = format_string("Method \"%s\" in type \"%s\" has more than one parameter named \"%s\"", 
                                              node->lexeme, builder->current_type->name, param->lexeme);
                HulkSemanticError error;
                HulkSemanticError_init(&error, error_msg, node->row, node->column);
                HulkErrorList_add(builder->errors, (HulkError*)&error);
                free(error_msg);
                set_method_error(builder->current_type, node->lexeme);
                free(param_names);
                free(param_types);
                return;
            }
        }
        
        param_names[i] = strdup(param->lexeme);
        
        if (param->symbol->name != NULL) {
            param_types[i] = context_get_type_or_protocol(builder->context, param->symbol->name);
            if (strcmp(param_types[i]->name, "<error>") == 0) {
                char* error_msg = format_string(HULK_SEM_NOT_DEFINED_METHOD_PARAM_TYPE,
                                              param->symbol->name, param->lexeme, node->symbol->name, builder->current_type->name);
                HulkSemanticError error;
                HulkSemanticError_init(&error, error_msg, node->row, node->column);
                HulkErrorList_add(builder->errors, (HulkError*)&error);
                free(error_msg);
                param_types[i] = context_get_type(builder->context, "<error>");
            }
        } else {
            param_types[i] = context_get_type(builder->context, "<auto>");
        }
    }

    Method* method = define_method(builder->current_type, builder->context, node->lexeme, param_names, 
                                  param_types, param_count, return_type);
    if (method == NULL) {
        set_method_error(builder->current_type, node->symbol->name);
    } else {
        method->node = node;
    }
}

void visit_method_signature(TypeBuilder* builder, Node* node) {
    Type* return_type;
    
    if (node->symbol->name == NULL) {
        char* error_msg = format_string(HULK_SEM_NO_PROTOCOL_RETURN_TYPE,
                                      node->symbol->name, builder->current_type->name);
        HulkSemanticError error;
        HulkSemanticError_init(&error, error_msg, node->row, node->column);
        HulkErrorList_add(builder->errors, (HulkError*)&error);
        free(error_msg);
        return_type = context_get_type(builder->context, "<error>");
    } else {
        return_type = context_get_type_or_protocol(builder->context, node->symbol->name);
        if (strcmp(return_type->name, "<error>") == 0) {
            char* error_msg = format_string(HULK_SEM_NOT_DEFINED_METHOD_RETURN_TYPE,
                                         node->symbol->name, node->lexeme, builder->current_type->name);
            HulkSemanticError error;
            HulkSemanticError_init(&error, error_msg, node->row, node->column);
            HulkErrorList_add(builder->errors, (HulkError*)&error);
            free(error_msg);
            return_type = context_get_type(builder->context, "<error>");
        }
    }

    int param_count = node->child_count-1;
    char** param_names = malloc(sizeof(char*) * param_count);
    Type** param_types = malloc(sizeof(Type*) * param_count);
    
    for (int i = 0; i < param_count; i++) {
        Node* param = node->children[i];
        
        // Verificar nombres duplicados
        for (int j = 0; j < i; j++) {
            if (strcmp(param->lexeme, param_names[j]) == 0) {
                char* error_msg = format_string("Method \"%s\" in protocol \"%s\" has more than one parameter named \"%s\"", 
                                              node->lexeme, builder->current_type->name, param->lexeme);
                HulkSemanticError error;
                HulkSemanticError_init(&error, error_msg, node->row, node->column);
                HulkErrorList_add(builder->errors, (HulkError*)&error);
                free(error_msg);
                set_method_error(builder->current_type, node->lexeme);
                free(param_names);
                free(param_types);
                return;
            }
        }
        
        param_names[i] = strdup(param->lexeme);
        
        if (param->symbol->name == NULL) {
            char* error_msg = format_string(HULK_SEM_NO_PROTOCOL_PARAM_TYPE,
                                         param->lexeme, node->lexeme, builder->current_type->name);
            HulkSemanticError error;
            HulkSemanticError_init(&error, error_msg, node->row, node->column);
            HulkErrorList_add(builder->errors, (HulkError*)&error);
            free(error_msg);
            param_types[i] = context_get_type(builder->context, "<error>");
        } else {
            param_types[i] = context_get_type_or_protocol(builder->context, param->symbol->name);
            if (strcmp(param_types[i]->name, "<error>") == 0) {
                char* error_msg = format_string(HULK_SEM_NOT_DEFINED_METHOD_PARAM_TYPE,
                                             param->symbol->name, param->lexeme, node->lexeme, builder->current_type->name);
                HulkSemanticError error;
                HulkSemanticError_init(&error, error_msg, node->row, node->column);
                HulkErrorList_add(builder->errors, (HulkError*)&error);
                free(error_msg);
                param_types[i] = context_get_type(builder->context, "<error>");
            }
        }
    }

    // Verificar si el tipo de retorno es nulo
    if (return_type == NULL) {
        char* error_msg = format_string("Return type for method \"%s\" cannot be NULL", node->lexeme);
        HulkSemanticError error;
        HulkSemanticError_init(&error, error_msg, node->row, node->column);
        HulkErrorList_add(builder->errors, (HulkError*)&error);
        free(error_msg);
        return;
    }
    
    Method* method = define_method(builder->current_type, builder->context, node->lexeme, param_names, 
                                  param_types, param_count, return_type);
    if (method == NULL) {
        char* error_msg = format_string("Method \"%s\" already defined in protocol \"%s\"", 
                                      node->lexeme, builder->current_type->name);
        HulkSemanticError error;
        HulkSemanticError_init(&error, error_msg, node->row, node->column);
        HulkErrorList_add(builder->errors, (HulkError*)&error);
        free(error_msg);
        set_method_error(builder->current_type, node->lexeme);
    } else {
        method->node = node;
    }
}
