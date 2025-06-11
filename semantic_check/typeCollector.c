#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils/semantic.h" 
#include "utils/semanticErrors.h" 
#include "../ast_nodes/ast_nodes.h"

typedef struct TypeCollector {
    Context* context;
    HulkErrorList* errors;
} TypeCollector;

// Prototipos de funciones
void TypeCollector_init(TypeCollector* collector, HulkErrorList* errors);
void TypeCollector_visit_program(TypeCollector* collector, ProgramNode* node);
void TypeCollector_visit_type_declaration(TypeCollector* collector, TypeDeclarationNode* node);
void TypeCollector_visit_protocol_declaration(TypeCollector* collector, ProtocolDeclarationNode* node);
void create_hulk_functions(Context* context);
void create_iterable_protocol(Context* context);

// Inicialización del TypeCollector
void TypeCollector_init(TypeCollector* collector, HulkErrorList* errors) {
    collector->context = create_context();
    collector->errors = errors;

    //Inicializar estructuras
    create_iterable_protocol(collector->context);
    create_hulk_functions(collector->context);
   
}

// Visita un nodo de programa
void TypeCollector_visit_program(TypeCollector* collector, ProgramNode* node) {
    
    DeclarationNode** declarations = (DeclarationNode**)node->declarations;

    for (int i = 0; i < node->base.child_count; i++) {
        DeclarationNode* declaration = declarations[i];
        
        if (declaration->base.tipo == NODE_TYPE_DECLARATION) {
            TypeCollector_visit_type_declaration(collector, (TypeDeclarationNode*)declaration);
        } 
        else if (declaration->base.tipo == NODE_PROTOCOL_DECLARATION) {
            TypeCollector_visit_protocol_declaration(collector, (ProtocolDeclarationNode*)declaration);
        }
    }
}

// Visita una declaración de tipo
void TypeCollector_visit_type_declaration(TypeCollector* collector, TypeDeclarationNode* node) {
    
    // Verificar si el tipo ya existe en los tipos built-in
    bool is_hulk_type = false;

    for (int i = 0; i < collector->context->hulk_type_count; i++) {
        if (strcmp(collector->context->hulk_types[i], node->name) == 0) {
            is_hulk_type = true;
            break;
        }
    }
    
    if (is_hulk_type) {
        char* error_msg = format_string("Type '%s' is a built-in type and cannot be redefined", node->name);
        HulkSemanticError error;
        HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
        HulkErrorList_add(collector->errors, (HulkError*)&error);
        free(error_msg);
        return;
    }
    
    // Verificar si el tipo ya existe
    Type* existing_type = context_get_type(collector->context, node->name);
    if (existing_type != NULL) {
        char* error_msg = format_string("Type '%s' is already defined", node->name);
        HulkSemanticError error;
        HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
        HulkErrorList_add(collector->errors, (HulkError*)&error);
        free(error_msg);
        return;
    }
    
    // Crear el nuevo tipo
    Type* new_type = create_type(collector->context, node->name);
    if (new_type == NULL) {
        char* error_msg = format_string("Could not create type '%s'", node->name);
        HulkSemanticError error;
        HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
        HulkErrorList_add(collector->errors, (HulkError*)&error);
        free(error_msg);
        return;
    }
    
    new_type->node = node;
}

// Visita una declaración de protocolo
void TypeCollector_visit_protocol_declaration(TypeCollector* collector, ProtocolDeclarationNode* node) {
    // Verificar si el protocolo ya existe en los protocolos built-in
    bool is_hulk_protocol = false;
    for (int i = 0; i < collector->context->hulk_protocol_count; i++) {
        if (strcmp(collector->context->hulk_protocols[i], node->name) == 0) {
            is_hulk_protocol = true;
            break;
        }
    }
    
    if (is_hulk_protocol) {
        char* error_msg = format_string("Protocol '%s' is a built-in protocol and cannot be redefined", node->name);
        HulkSemanticError error;
        HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
        HulkErrorList_add(collector->errors, (HulkError*)&error);
        free(error_msg);
        return;
    }
    
    // Verificar si el protocolo ya existe
    Protocol* existing_protocol = context_get_protocol(collector->context, node->name);
    if (existing_protocol != NULL) {
        char* error_msg = format_string("Protocol '%s' is already defined", node->name);
        HulkSemanticError error;
        HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
        HulkErrorList_add(collector->errors, (HulkError*)&error);
        free(error_msg);
        return;
    }
    
    // Crear el nuevo protocolo
    Protocol* new_protocol = create_protocol(collector->context, node->name);
    if (new_protocol == NULL) {
        char* error_msg = format_string("Could not create protocol '%s'", node->name);
        HulkSemanticError error;
        HulkSemanticError_init(&error, error_msg, node->base.base.row, node->base.base.column);
        HulkErrorList_add(collector->errors, (HulkError*)&error);
        free(error_msg);
        return;
    }
}

// Crea las funciones built-in de HULK
void create_hulk_functions(Context* context) {  
    // Crear tipo Range
    Type* range_type = create_type(context, "Range");
    if (range_type != NULL) {
        // Definir parámetros del tipo Range
        char** param_names = (char**)malloc(sizeof(char*) * 2);
        param_names[0] = strdup("min");
        param_names[1] = strdup("max");
        
        Type** param_types = (Type**)malloc(sizeof(Type*) * 2);
        param_types[0] = context_get_type(context, "Number");
        param_types[1] = context_get_type(context, "Number");
        
        range_type->param_names = param_names;
        range_type->param_types = param_types;
        range_type->param_count = 2;
        
        // Definir atributos
        Attribute* min_attr = (Attribute*)malloc(sizeof(Attribute));
        min_attr->name = strdup("min");
        min_attr->type = context_get_type(context, "Number");
        min_attr->value = NULL;
        min_attr->node = NULL;
        
        Attribute* max_attr = (Attribute*)malloc(sizeof(Attribute));
        max_attr->name = strdup("max");
        max_attr->type = context_get_type(context, "Number");
        max_attr->value = NULL;
        max_attr->node = NULL;
        
        Attribute* current_attr = (Attribute*)malloc(sizeof(Attribute));
        current_attr->name = strdup("current");
        current_attr->type = context_get_type(context, "Number");
        current_attr->value = NULL;
        current_attr->node = NULL;
        
        range_type->attributes = (Attribute**)malloc(sizeof(Attribute*) * 3);
        range_type->attributes[0] = min_attr;
        range_type->attributes[1] = max_attr;
        range_type->attributes[2] = current_attr;
        range_type->attribute_count = 3;
        
        // Definir métodos
        Method* next_method = (Method*)malloc(sizeof(Method));
        next_method->name = strdup("next");
        next_method->param_names = NULL;
        next_method->param_types = NULL;
        next_method->param_count = 0;
        next_method->return_type = context_get_type(context, "Boolean");
        next_method->inferred_return_type = context_get_type(context, "Boolean");
        next_method->node = NULL;
        
        Method* current_method = (Method*)malloc(sizeof(Method));
        current_method->name = strdup("current");
        current_method->param_names = NULL;
        current_method->param_types = NULL;
        current_method->param_count = 0;
        current_method->return_type = context_get_type(context, "Number");
        current_method->inferred_return_type = context_get_type(context, "Number");
        current_method->node = NULL;
        
        range_type->methods = (Method**)malloc(sizeof(Method*) * 2);
        range_type->methods[0] = next_method;
        range_type->methods[1] = current_method;
        range_type->method_count = 2;
    }
    
    // Crear funciones built-in
    char* sqrt_params[] = {strdup("value")};
    Type* number_type = context_get_type(context, "Number");
    Type* sqrt_param_types[] = {number_type};
    create_function(context, "sqrt", sqrt_params, sqrt_param_types, 1, number_type);
    
    char* sin_params[] = {strdup("angle")};
    Type* sin_param_types[] = {number_type};
    create_function(context, "sin", sin_params, sin_param_types, 1, number_type);
    
    char* cos_params[] = {strdup("angle")};
    Type* cos_param_types[] = {number_type};
    create_function(context, "cos", cos_params, cos_param_types, 1, number_type);
    
    char* exp_params[] = {strdup("value")};
    Type* exp_param_types[] = {number_type};
    create_function(context, "exp", exp_params, exp_param_types, 1, number_type);
    
    char* log_params[] = {strdup("base"), strdup("value")};
    Type* log_param_types[] = {number_type, number_type};
    create_function(context, "log", log_params, log_param_types, 2, number_type);
    
    create_function(context, "rand", NULL, NULL, 0, number_type);
    
    char* print_params[] = {strdup("obj")};
    Type* object_type = context_get_type(context, "Object");
    Type* print_param_types[] = {object_type};
    create_function(context, "print", print_params, print_param_types, 1, object_type);
    
    char* range_params[] = {strdup("begin"), strdup("end")};
    Type* range_param_types[] = {number_type, number_type};
    create_function(context, "range", range_params, range_param_types, 2, range_type);
}

// Crea el protocolo Iterable
void create_iterable_protocol(Context* context) {
    Protocol* iterable_protocol = create_protocol(context, "Iterable");
    if (iterable_protocol != NULL) {
        // Definir método next
        Method* next_method = (Method*)malloc(sizeof(Method));
        next_method->name = strdup("next");
        next_method->param_names = NULL;
        next_method->param_types = NULL;
        next_method->param_count = 0;
        next_method->return_type = context_get_type(context, "Boolean");
        next_method->inferred_return_type = context_get_type(context, "Boolean");
        next_method->node = NULL;
        
        // Definir método current
        Method* current_method = (Method*)malloc(sizeof(Method));
        current_method->name = strdup("current");
        current_method->param_names = NULL;
        current_method->param_types = NULL;
        current_method->param_count = 0;
        current_method->return_type = context_get_type(context, "Object");
        current_method->inferred_return_type = context_get_type(context, "Object");
        current_method->node = NULL;
        
        iterable_protocol->methods = (Method**)malloc(sizeof(Method*) * 2);
        iterable_protocol->methods[0] = next_method;
        iterable_protocol->methods[1] = current_method;
        iterable_protocol->method_count = 2;
    }
}