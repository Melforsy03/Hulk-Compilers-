#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast_nodes.h"
#include "semantic.h"  // definiciones de Context, Type, Protocol, etc.
#include "errors.h"    // Para HulkSemanticError

//------------------------- TypeCollector -------------------------

typedef struct {
    Context* context;
    List* errors;  // Lista de HulkSemanticError*
} TypeCollector;

// Funciones auxiliares
void create_iterable_protocol(TypeCollector* collector);
void create_hulk_functions(TypeCollector* collector);

//------------------------- Visitor Implementation -------------------------

void* type_collector_visit(TypeCollector* collector, Node* node) {
    if (!node || !collector) return NULL;
    
    switch (node->tipo) {
        case NODE_PROGRAM:
            return type_collector_visit_program(collector, (ProgramNode*)node);
        case NODE_TYPE_DEF:
            return type_collector_visit_type_declaration(collector, (TypeDeclarationNode*)node);
        case NODE_PROTOCOL_DECLARATION:
            return type_collector_visit_protocol_declaration(collector, (ProtocolDeclarationNode*)node);
        // Otros casos pueden ser manejados aquí si es necesario
        default:
            return NULL;
    }
}

void* type_collector_visit_program(TypeCollector* collector, ProgramNode* node) {
    // Inicializar el contexto
    collector->context = context_create();
    
    // Crear protocolo Iterable y funciones built-in
    create_iterable_protocol(collector);
    create_hulk_functions(collector);
    
    // Visitar todas las declaraciones
    ListNode* current = node->declarations;
    while (current) {
        DeclarationNode* decl = (DeclarationNode*)current->data;
        try {
            type_collector_visit(collector, (Node*)decl);
        } catch (SemanticError* error) {
            HulkSemanticError* hulk_error = hulk_semantic_error_create(error, node->row, node->column);
            list_append(collector->errors, hulk_error);
        }
        current = current->next;
    }
    
    return NULL;
}

void* type_collector_visit_type_declaration(TypeCollector* collector, TypeDeclarationNode* node) {
    try {
        Type* new_type = context_create_type(collector->context, node->name);
        new_type->node = node;  // Guardar referencia al nodo AST
        
    } catch (SemanticError* error) {
        HulkSemanticError* hulk_error = hulk_semantic_error_create(error, node->row, node->column);
        list_append(collector->errors, hulk_error);
        
        // Verificar si el tipo no existe en el contexto
        if (!context_has_type(collector->context, node->name)) {
            context_set_type_error(collector->context, node->name);
        }
    }
    
    return NULL;
}

void* type_collector_visit_protocol_declaration(TypeCollector* collector, ProtocolDeclarationNode* node) {
    try {
        Protocol* new_protocol = context_create_protocol(collector->context, node->name);
        
    } catch (SemanticError* error) {
        HulkSemanticError* hulk_error = hulk_semantic_error_create(error, node->row, node->column);
        list_append(collector->errors, hulk_error);
        
        // Verificar si el protocolo no existe en el contexto
        if (!context_has_protocol(collector->context, node->name)) {
            context_set_protocol_error(collector->context, node->name);
        }
    }
    
    return NULL;
}

//------------------------- Helper Functions -------------------------

void create_iterable_protocol(TypeCollector* collector) {
    Protocol* iterable_protocol = context_create_protocol(collector->context, "Iterable");
    
    // Definir métodos del protocolo Iterable
    protocol_define_method(iterable_protocol, "next", NULL, 0, type_get(collector->context, "Boolean"));
    protocol_define_method(iterable_protocol, "current", NULL, 0, type_get(collector->context, "Object"));
}

void create_hulk_functions(TypeCollector* collector) {
    // Crear tipo Range
    Type* range_type = context_create_type(collector->context, "Range");
    
    // Definir parámetros, atributos y métodos de Range
    const char* range_params[] = {"min", "max"};
    Type* range_param_types[] = {
        type_get(collector->context, "Number"),
        type_get(collector->context, "Number")
    };
    type_set_params(range_type, range_params, range_param_types, 2);
    
    type_define_attribute(range_type, "min", type_get(collector->context, "Number"));
    type_define_attribute(range_type, "max", type_get(collector->context, "Number"));
    type_define_attribute(range_type, "current", type_get(collector->context, "Number"));
    
    type_define_method(range_type, "next", NULL, 0, type_get(collector->context, "Boolean"));
    type_define_method(range_type, "current", NULL, 0, type_get(collector->context, "Number"));
    
    // Crear funciones built-in
    context_create_function(collector->context, "sqrt", (const char*[]){"value"}, 1, 
                          (Type*[]){type_get(collector->context, "Number")}, 
                          type_get(collector->context, "Number"));
    
    context_create_function(collector->context, "sin", (const char*[]){"angle"}, 1,
                          (Type*[]){type_get(collector->context, "Number")},
                          type_get(collector->context, "Number"));
    
    context_create_function(collector->context, "cos", (const char*[]){"angle"}, 1,
                          (Type*[]){type_get(collector->context, "Number")},
                          type_get(collector->context, "Number"));
    
    context_create_function(collector->context, "exp", (const char*[]){"value"}, 1,
                          (Type*[]){type_get(collector->context, "Number")},
                          type_get(collector->context, "Number"));
    
    context_create_function(collector->context, "log", (const char*[]){"base", "value"}, 2,
                          (Type*[]){type_get(collector->context, "Number"), 
                                   type_get(collector->context, "Number")},
                          type_get(collector->context, "Number"));
    
    context_create_function(collector->context, "rand", NULL, 0, NULL,
                          type_get(collector->context, "Number"));
    
    context_create_function(collector->context, "print", (const char*[]){"obj"}, 1,
                          (Type*[]){type_get(collector->context, "Object")},
                          type_get(collector->context, "Object"));
    
    context_create_function(collector->context, "range", (const char*[]){"begin", "end"}, 2,
                          (Type*[]){type_get(collector->context, "Number"), 
                                   type_get(collector->context, "Number")},
                          range_type);
}

//------------------------- Public Interface -------------------------

TypeCollector* type_collector_create(List* errors) {
    TypeCollector* collector = (TypeCollector*)malloc(sizeof(TypeCollector));
    if (!collector) return NULL;
    
    collector->context = NULL;
    collector->errors = errors;
    
    return collector;
}

void type_collector_free(TypeCollector* collector) {
    if (collector) {
        // El contexto debe ser liberado por quien lo creó
        free(collector);
    }
}

Context* type_collector_run(TypeCollector* collector, ProgramNode* ast) {
    type_collector_visit(collector, (Node*)ast);
    return collector->context;
}