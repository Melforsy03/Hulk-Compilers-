#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast_nodes.h"
#include "semantic.h"
#include "errors.h"

//------------------------- TypeBuilder -------------------------

typedef struct {
    Context* context;
    Type* current_type;
    List* errors;
} TypeBuilder;

//------------------------- Visitor Implementation -------------------------

void* type_builder_visit(TypeBuilder* builder, Node* node) {
    if (!node || !builder) return NULL;
    
    switch (node->tipo) {
        case NODE_PROGRAM:
            return type_builder_visit_program(builder, (ProgramNode*)node);
        case NODE_TYPE_DEF:
            return type_builder_visit_type_declaration(builder, (TypeDeclarationNode*)node);
        case NODE_PROTOCOL_DECLARATION:
            return type_builder_visit_protocol_declaration(builder, (ProtocolDeclarationNode*)node);
        case NODE_FUNCTION_DEF:
            return type_builder_visit_function_declaration(builder, (FunctionDeclarationNode*)node);
        case NODE_TYPE_ATTRIBUTE:
            return type_builder_visit_type_attribute(builder, (TypeAttributeNode*)node);
        case NODE_METHOD_DECLARATION:
            return type_builder_visit_method_declaration(builder, (MethodDeclarationNode*)node);
        case NODE_METHOD_SIGNATURE:
            return type_builder_visit_method_signature(builder, (MethodSignatureNode*)node);
        default:
            return NULL;
    }
}

void* type_builder_visit_program(TypeBuilder* builder, ProgramNode* node) {
    ListNode* current = node->declarations;
    while (current) {
        DeclarationNode* decl = (DeclarationNode*)current->data;
        try {
            type_builder_visit(builder, (Node*)decl);
        } catch (SemanticError* error) {
            HulkSemanticError* hulk_error = hulk_semantic_error_create(error, node->row, node->column);
            list_append(builder->errors, hulk_error);
        }
        current = current->next;
    }
    return NULL;
}

void* type_builder_visit_type_declaration(TypeBuilder* builder, TypeDeclarationNode* node) {
    // Verificar si el tipo ya está definido o es un error
    if (context_has_type(builder->context, node->name) || 
        type_equals(context_get_type(builder->context, node->name), error_type())) {
        return NULL;
    }
    
    builder->current_type = context_get_type(builder->context, node->name);
    
    // Procesar parámetros del tipo
    List* param_names = list_create();
    List* param_types = list_create();
    HashTable* names_count = hash_table_create();
    
    ListNode* param_current = node->params;
    while (param_current) {
        ParamNode* param = (ParamNode*)param_current->data;
        
        // Verificar nombres duplicados
        if (hash_table_contains(names_count, param->name)) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), 
                    "Type %s has more than one parameter named %s", node->name, param->name);
            
            HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->row, node->column);
            list_append(builder->errors, error);
            
            context_set_type_error(builder->context, node->name);
            
            hash_table_free(names_count);
            list_free(param_names, free);
            list_free(param_types, NULL); // Los tipos no se liberan aquí
            return NULL;
        }
        hash_table_insert(names_count, param->name, (void*)1);
        
        // Obtener tipo del parámetro
        Type* param_type;
        if (param->type_name) {
            try {
                param_type = context_get_type_or_protocol(builder->context, param->type_name);
            } catch (SemanticError* error) {
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg),
                        "Undefined type '%s' for parameter '%s' in type '%s'", 
                        param->type_name, param->name, builder->current_type->name);
                
                HulkSemanticError* hulk_error = hulk_semantic_error_create_str(error_msg, node->row, node->column);
                list_append(builder->errors, hulk_error);
                param_type = error_type();
            }
        } else {
            param_type = auto_type();
        }
        
        list_append(param_names, strdup(param->name));
        list_append(param_types, param_type);
        param_current = param_current->next;
    }
    
    // Establecer parámetros del tipo
    type_set_params(builder->current_type, param_names, param_types);
    
    // Procesar métodos
    ListNode* method_current = node->methods;
    while (method_current) {
        MethodDeclarationNode* method = (MethodDeclarationNode*)method_current->data;
        try {
            type_builder_visit(builder, (Node*)method);
        } catch (SemanticError* error) {
            HulkSemanticError* hulk_error = hulk_semantic_error_create(error, node->row, node->column);
            list_append(builder->errors, hulk_error);
        }
        method_current = method_current->next;
    }
    
    // Procesar herencia
    Type* parent;
    if (strcmp(node->parent, "String") == 0 || 
        strcmp(node->parent, "Boolean") == 0 || 
        strcmp(node->parent, "Number") == 0) {
        
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Invalid inheritance from default type '%s' in type '%s'", 
                node->parent, builder->current_type->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->row, node->column);
        list_append(builder->errors, error);
        parent = error_type();
    } else {
        try {
            parent = context_get_type(builder->context, node->parent);
            
            // Verificar herencia circular
            if (type_conforms_to(parent, builder->current_type)) {
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg),
                        "Invalid circular inheritance between '%s' and '%s'", 
                        builder->current_type->name, node->parent);
                
                HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->row, node->column);
                list_append(builder->errors, error);
                parent = error_type();
            }
        } catch (SemanticError* error) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                    "Undefined parent type '%s' for type '%s'", 
                    node->parent, builder->current_type->name);
            
            HulkSemanticError* hulk_error = hulk_semantic_error_create_str(error_msg, node->row, node->column);
            list_append(builder->errors, hulk_error);
            parent = error_type();
        }
    }
    
    type_set_parent(builder->current_type, parent);
    
    // Procesar atributos
    ListNode* attr_current = node->attributes;
    while (attr_current) {
        TypeAttributeNode* attr = (TypeAttributeNode*)attr_current->data;
        try {
            type_builder_visit(builder, (Node*)attr);
        } catch (SemanticError* error) {
            HulkSemanticError* hulk_error = hulk_semantic_error_create(error, node->row, node->column);
            list_append(builder->errors, hulk_error);
        }
        attr_current = attr_current->next;
    }
    
    // Liberar recursos
    hash_table_free(names_count);
    list_free(param_names, free);
    list_free(param_types, NULL);
    
    builder->current_type = NULL;
    return NULL;
}

void* type_builder_visit_protocol_declaration(TypeBuilder* builder, ProtocolDeclarationNode* node) {
    if (context_has_protocol(builder->context, node->name) || 
        protocol_equals(context_get_protocol(builder->context, node->name), error_protocol())) {
        return NULL;
    }
    
    builder->current_type = (Type*)context_get_protocol(builder->context, node->name);
    
    // Procesar firmas de métodos
    ListNode* method_current = node->methods_signature;
    while (method_current) {
        MethodSignatureNode* method = (MethodSignatureNode*)method_current->data;
        try {
            type_builder_visit(builder, (Node*)method);
        } catch (SemanticError* error) {
            HulkSemanticError* hulk_error = hulk_semantic_error_create(error, node->row, node->column);
            list_append(builder->errors, hulk_error);
        }
        method_current = method_current->next;
    }
    
    // Procesar herencia
    Type* parent;
    if (!node->parent) {
        parent = object_type();
    } else {
        try {
            parent = (Type*)context_get_protocol(builder->context, node->parent);
            
            // Verificar herencia circular
            if (type_conforms_to(parent, builder->current_type)) {
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg),
                        "Invalid circular inheritance between '%s' and '%s'", 
                        builder->current_type->name, node->parent);
                
                HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->row, node->column);
                list_append(builder->errors, error);
                parent = error_protocol();
            }
        } catch (SemanticError* error) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                    "Undefined parent protocol '%s' for protocol '%s'", 
                    node->parent, builder->current_type->name);
            
            HulkSemanticError* hulk_error = hulk_semantic_error_create_str(error_msg, node->row, node->column);
            list_append(builder->errors, hulk_error);
            parent = error_protocol();
        }
    }
    
    type_set_parent(builder->current_type, parent);
    builder->current_type = NULL;
    return NULL;
}

void* type_builder_visit_function_declaration(TypeBuilder* builder, FunctionDeclarationNode* node) {
    builder->current_type = NULL;
    
    // Obtener tipo de retorno
    Type* return_type;
    if (node->returnType) {
        try {
            return_type = context_get_type_or_protocol(builder->context, node->returnType);
        } catch (SemanticError* error) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                    "Undefined return type '%s' for function '%s'", 
                    node->returnType, node->name);
            
            HulkSemanticError* hulk_error = hulk_semantic_error_create_str(error_msg, node->row, node->column);
            list_append(builder->errors, hulk_error);
            return_type = error_type();
        }
    } else {
        return_type = auto_type();
    }
    
    // Procesar parámetros
    List* param_names = list_create();
    List* param_types = list_create();
    HashTable* names_count = hash_table_create();
    
    ListNode* param_current = node->params;
    while (param_current) {
        ParamNode* param = (ParamNode*)param_current->data;
        
        // Verificar nombres duplicados
        if (hash_table_contains(names_count, param->name)) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                    "Function '%s' has more than one parameter named '%s'", 
                    node->name, param->name);
            
            HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->row, node->column);
            list_append(builder->errors, error);
            
            context_set_function_error(builder->context, node->name);
            
            hash_table_free(names_count);
            list_free(param_names, free);
            list_free(param_types, NULL);
            return NULL;
        }
        hash_table_insert(names_count, param->name, (void*)1);
        
        // Obtener tipo del parámetro
        Type* param_type;
        if (param->type_name) {
            try {
                param_type = context_get_type_or_protocol(builder->context, param->type_name);
            } catch (SemanticError* error) {
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg),
                        "Undefined type '%s' for parameter '%s' in function '%s'", 
                        param->type_name, param->name, node->name);
                
                HulkSemanticError* hulk_error = hulk_semantic_error_create_str(error_msg, node->row, node->column);
                list_append(builder->errors, hulk_error);
                param_type = error_type();
            }
        } else {
            param_type = auto_type();
        }
        
        list_append(param_names, strdup(param->name));
        list_append(param_types, param_type);
        param_current = param_current->next;
    }
    
    // Crear la función
    try {
        Function* func = context_create_function(builder->context, node->name, param_names, param_types, return_type);
        func->node = node;
    } catch (SemanticError* error) {
        HulkSemanticError* hulk_error = hulk_semantic_error_create(error, node->row, node->column);
        list_append(builder->errors, hulk_error);
        
        if (!context_has_function(builder->context, node->name)) {
            context_set_function_error(builder->context, node->name);
        }
    }
    
    // Liberar recursos
    hash_table_free(names_count);
    list_free(param_names, free);
    list_free(param_types, NULL);
    
    return NULL;
}

void* type_builder_visit_type_attribute(TypeBuilder* builder, TypeAttributeNode* node) {
    // Obtener el tipo del atributo
    Type* attr_type;
    if (node->type) {
        try {
            attr_type = context_get_type_or_protocol(builder->context, node->type);
        } catch (SemanticError* error) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                    "Undefined type '%s' for attribute '%s' in type '%s'", 
                    node->type, node->name, builder->current_type->name);
            
            HulkSemanticError* hulk_error = hulk_semantic_error_create_str(error_msg, node->row, node->column);
            list_append(builder->errors, hulk_error);
            attr_type = error_type();
        }
    } else {
        attr_type = auto_type();
    }

    // Definir el atributo en el tipo actual
    try {
        Attribute* attribute = type_define_attribute(builder->current_type, node->name, attr_type);
        attribute->node = node;  // Guardar referencia al nodo AST
    } catch (SemanticError* error) {
        HulkSemanticError* hulk_error = hulk_semantic_error_create(error, node->row, node->column);
        list_append(builder->errors, hulk_error);
        type_set_attribute_error(builder->current_type, node->name);
    }

    return NULL;
}

void* type_builder_visit_method_declaration(TypeBuilder* builder, MethodDeclarationNode* node) {
    // Obtener tipo de retorno
    Type* return_type;
    if (node->returnType) {
        try {
            return_type = context_get_type_or_protocol(builder->context, node->returnType);
        } catch (SemanticError* error) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                    "Undefined return type '%s' for method '%s' in type '%s'", 
                    node->returnType, node->name, builder->current_type->name);
            
            HulkSemanticError* hulk_error = hulk_semantic_error_create_str(error_msg, node->row, node->column);
            list_append(builder->errors, hulk_error);
            return_type = error_type();
        }
    } else {
        return_type = auto_type();
    }

    // Procesar parámetros
    List* param_names = list_create();
    List* param_types = list_create();
    HashTable* names_count = hash_table_create();
    
    ListNode* param_current = node->params;
    while (param_current) {
        ParamNode* param = (ParamNode*)param_current->data;
        
        // Verificar nombres duplicados
        if (hash_table_contains(names_count, param->name)) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                    "Method '%s' in type '%s' has more than one parameter named '%s'", 
                    node->name, builder->current_type->name, param->name);
            
            HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->row, node->column);
            list_append(builder->errors, error);
            
            type_set_method_error(builder->current_type, node->name);
            
            hash_table_free(names_count);
            list_free(param_names, free);
            list_free(param_types, NULL);
            return NULL;
        }
        hash_table_insert(names_count, param->name, (void*)1);
        
        // Obtener tipo del parámetro
        Type* param_type;
        if (param->type_name) {
            try {
                param_type = context_get_type_or_protocol(builder->context, param->type_name);
            } catch (SemanticError* error) {
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg),
                        "Undefined type '%s' for parameter '%s' in method '%s' of type '%s'", 
                        param->type_name, param->name, node->name, builder->current_type->name);
                
                HulkSemanticError* hulk_error = hulk_semantic_error_create_str(error_msg, node->row, node->column);
                list_append(builder->errors, hulk_error);
                param_type = error_type();
            }
        } else {
            param_type = auto_type();
        }
        
        list_append(param_names, strdup(param->name));
        list_append(param_types, param_type);
        param_current = param_current->next;
    }

    // Definir el método en el tipo actual
    try {
        Method* method = type_define_method(builder->current_type, node->name, param_names, param_types, return_type);
        method->node = node;  // Guardar referencia al nodo AST
    } catch (SemanticError* error) {
        HulkSemanticError* hulk_error = hulk_semantic_error_create(error, node->row, node->column);
        list_append(builder->errors, hulk_error);
        type_set_method_error(builder->current_type, node->name);
    }

    // Liberar recursos
    hash_table_free(names_count);
    list_free(param_names, free);
    list_free(param_types, NULL);

    return NULL;
}

void* type_builder_visit_method_signature(TypeBuilder* builder, MethodSignatureNode* node) {
    // Verificar que el método tenga tipo de retorno (obligatorio en protocolos)
    if (!node->returnType) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Method '%s' in protocol '%s' must declare a return type", 
                node->name, builder->current_type->name);
        
        HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->row, node->column);
        list_append(builder->errors, error);
        return NULL;
    }

    // Obtener tipo de retorno
    Type* return_type;
    try {
        return_type = context_get_type_or_protocol(builder->context, node->returnType);
    } catch (SemanticError* error) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                "Undefined return type '%s' for method '%s' in protocol '%s'", 
                node->returnType, node->name, builder->current_type->name);
        
        HulkSemanticError* hulk_error = hulk_semantic_error_create_str(error_msg, node->row, node->column);
        list_append(builder->errors, hulk_error);
        return_type = error_type();
    }

    // Procesar parámetros
    List* param_names = list_create();
    List* param_types = list_create();
    HashTable* names_count = hash_table_create();
    
    ListNode* param_current = node->params;
    while (param_current) {
        ParamNode* param = (ParamNode*)param_current->data;
        
        // Verificar nombres duplicados
        if (hash_table_contains(names_count, param->name)) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                    "Method '%s' in protocol '%s' has more than one parameter named '%s'", 
                    node->name, builder->current_type->name, param->name);
            
            HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->row, node->column);
            list_append(builder->errors, error);
            
            type_set_method_error(builder->current_type, node->name);
            
            hash_table_free(names_count);
            list_free(param_names, free);
            list_free(param_types, NULL);
            return NULL;
        }
        hash_table_insert(names_count, param->name, (void*)1);
        
        // Verificar que el parámetro tenga tipo (obligatorio en protocolos)
        if (!param->type_name) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                    "Parameter '%s' in method '%s' of protocol '%s' must declare a type", 
                    param->name, node->name, builder->current_type->name);
            
            HulkSemanticError* error = hulk_semantic_error_create_str(error_msg, node->row, node->column);
            list_append(builder->errors, error);
            list_append(param_names, strdup(param->name));
            list_append(param_types, error_type());
        } else {
            // Obtener tipo del parámetro
            try {
                Type* param_type = context_get_type_or_protocol(builder->context, param->type_name);
                list_append(param_names, strdup(param->name));
                list_append(param_types, param_type);
            } catch (SemanticError* error) {
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg),
                        "Undefined type '%s' for parameter '%s' in method '%s' of protocol '%s'", 
                        param->type_name, param->name, node->name, builder->current_type->name);
                
                HulkSemanticError* hulk_error = hulk_semantic_error_create_str(error_msg, node->row, node->column);
                list_append(builder->errors, hulk_error);
                list_append(param_names, strdup(param->name));
                list_append(param_types, error_type());
            }
        }
        param_current = param_current->next;
    }

    // Definir la firma del método en el protocolo actual
    try {
        protocol_define_method((Protocol*)builder->current_type, node->name, param_names, param_types, return_type);
    } catch (SemanticError* error) {
        HulkSemanticError* hulk_error = hulk_semantic_error_create(error, node->row, node->column);
        list_append(builder->errors, hulk_error);
        type_set_method_error(builder->current_type, node->name);
    }

    // Liberar recursos
    hash_table_free(names_count);
    list_free(param_names, free);
    list_free(param_types, NULL);

    return NULL;
}
//------------------------- Public Interface -------------------------

TypeBuilder* type_builder_create(Context* context, List* errors) {
    TypeBuilder* builder = (TypeBuilder*)malloc(sizeof(TypeBuilder));
    if (!builder) return NULL;
    
    builder->context = context;
    builder->current_type = NULL;
    builder->errors = errors;
    
    return builder;
}

void type_builder_free(TypeBuilder* builder) {
    if (builder) {
        free(builder);
    }
}

void type_builder_run(TypeBuilder* builder, ProgramNode* ast) {
    type_builder_visit(builder, (Node*)ast);
}