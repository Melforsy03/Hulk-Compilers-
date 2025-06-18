#ifndef SEMANTIC_H
#define SEMANTIC_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>


// Estructuras básicas
typedef struct Type Type;
typedef struct Protocol Protocol;
typedef struct Attribute Attribute;
typedef struct Method Method;
typedef struct Context Context;
typedef struct VariableInfo VariableInfo;
typedef struct Scope Scope;
typedef struct Node Node;

struct Type {

    char* name;
    Type* parent;
    
    // Para tipos personalizados y protocolos
    Attribute** attributes;
    int attribute_count;
    
    Method** methods;
    int method_count;
    
    // Para vectores
    Type* element_type;
    
    // Para tipos con parámetros
    char** param_names;
    Type** param_types;
    int param_count;
    
    // Referencia para SelfType
    Type* referred_type;

    Node* node;
};


typedef struct ErrorType {
    Type base;
} ErrorType;

typedef struct AutoType {
    Type base;
} AutoType;

typedef struct StringType {
    Type base;
} StringType;

typedef struct BooleanType {
    Type base;
} BooleanType;

typedef struct NumberType {
    Type base;
} NumberType;

typedef struct ObjectType {
    Type base;
} ObjectType;

typedef struct SelfType {
    Type base;
    Type* referred_type;
} SelfType;

typedef struct VectorType {
    Type base;
    Type* element_type;
}VectorType;


struct Protocol {
    Type* base;
    char* name;
    Protocol* parent;
    Method** methods;
    int method_count;
};

typedef union {
    Type* type;
    Protocol* protocol;
} TypeOrProtocol;

struct Attribute {
    char* name;
    Type* type;
    void* value; // Podría ser un puntero a algún valor
    void* node;  // Para referencia al nodo AST
};

struct Method {
    char* name;
    char** param_names;
    Type** param_types;
    int param_count;
    Type* return_type;
    Type* inferred_return_type;
    Node* node; // Para referencia al nodo AST
};

struct Context {
    Type** types;
    int type_count;
    
    Protocol** protocols;
    int protocol_count;
    
    Method** functions;
    int function_count;
    
    char** hulk_types;
    int hulk_type_count;
    
    char** hulk_protocols;
    int hulk_protocol_count;
    
    char** hulk_functions;
    int hulk_function_count;
};

struct VariableInfo {
    char* name;
    Type* type;
    bool is_error;
    bool is_parameter;
    void* value;
};

struct Scope {
    VariableInfo** locals;
    int locals_count;
    Scope* parent;
    Scope** children;
    int children_count;
    int index;
};

char* strdup(const char* s);

// Create a new type
Type* create_type(Context* context, const char* name);

Protocol* create_protocol(Context* context, const char* name);

Attribute* create_attribute(char* name, Type* type);
// Create a new function
Method* create_function(Context* context, const char* name, char** param_names, Type** param_types, int param_count, Type* return_type);
Context* create_context();

// Initialize built-in types
void initialize_builtin_types(Context* context);
Scope* create_scope(Scope* parent);

// ---------------------------Funciones de utilidad------------------------------

char* format_string(const char* format, ...);

Type* get_lowest_common_ancestor(Type** types, int type_count);
bool conforms_to(Type* type, Type* other);
bool method_implements(Method* method, Method* other);
// ---------------------------Funciones para manejar atributos----------------------------
Attribute* get_attribute(Type* type, char* name);

Attribute* define_attribute(Type* type, const char* name, Type* typex);

void set_attribute_error(Type* type, const char* attr_name);

//-------------------------------Funciones para manejar metodos-----------------------------
Method* get_method(Type* type, char* name);

Method* define_method(Type* type, Context* ctx, const char* name, char** param_names, Type** param_types, int param_count, Type* return_type);
void set_method_error(Type* type, const char* method_name);
Method* protocol_get_method(Protocol* protocol, char* name);
// ----------------------------Funciones para manejar el contexto--------------------------------------

Type* context_get_type(Context* ctx, char* name);

Protocol* context_get_protocol(Context* ctx, char* name);

Type* context_get_type_or_protocol(Context* ctx, char* name);
void context_set_type_error(Context* ctx, const char* type_name);
Method* context_get_function(Context* ctx, char* name);

void context_set_function_error(Context* ctx, const char* function_name);

//--------------------------------Funciones para manejar el scope-------------------------------------
VariableInfo* scope_find_variable(Scope* scope, char* name, int index);

VariableInfo* scope_define_variable(Scope* scope, char* name, Type* type, bool is_parameter);


VectorType* create_vector_type(Context* context, Type* element_type);
Type* get_vector_element_type(VectorType* vector_type);
bool vector_conforms_to(VectorType* self, Type* other);
Type* get_vector_type(Context* context, Type** item_types, int item_count);
//--------------------------------- Funciones para liberar memoria-----------------------------------
void free_type(Type* type);
void free_protocol(Protocol* protocol);
void free_context(Context* ctx);
void free_scope(Scope* scope);
#endif