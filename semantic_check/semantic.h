
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

//Prototipos de func
Type* context_get_type(Context* ctx, char* name);
void initialize_builtin_types(Context* context);
VariableInfo* scope_define_variable(Scope* scope, char* name, Type* type, bool is_parameter);
Method* define_method(Type* type, Context* ctx, const char* name, char** param_names, Type** param_types, int param_count, Type* return_type);
void free_type(Type* type);
void free_scope(Scope* scope);
void free_context(Context* ctx);
void free_protocol(Protocol* protocol);

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
} VectorType;

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

char* strdup(const char* s) {
    size_t len = strlen(s) + 1;
    char* p = (char*)malloc(len);
    if (p) {
        memcpy(p, s, len);
    }
    return p;
}

// Create a new type
Type* create_type(Context* context, const char* name) {
    // Check if type or protocol with this name already exists
    for (int i = 0; i < context->type_count; i++) {
        if (strcmp(context->types[i]->name, name) == 0) {
            return NULL;  // Type already exists
        }
    }
    
    for (int i = 0; i < context->protocol_count; i++) {
        if (strcmp(context->protocols[i]->name, name) == 0) {
            return NULL;  // Protocol with same name exists
        }
    }
    
    // Create new type
    Type* new_type = (Type*)malloc(sizeof(Type));
    new_type->name = strdup(name);
    new_type->attributes = NULL;
    new_type->attribute_count = 0;
    new_type->methods = NULL;
    new_type->method_count = 0;
    new_type->parent = NULL;
    new_type->param_names = NULL;
    new_type->param_types = NULL;
    new_type->param_count = 0;
    new_type->node = NULL;
    
    // Add to context
    context->types = (Type**)realloc(context->types, sizeof(Type*) * (context->type_count + 1));
    context->types[context->type_count++] = new_type;
    
    return new_type;
}

Protocol* create_protocol(Context* context, const char* name) {
    // Check if type or protocol with this name already exists
    for (int i = 0; i < context->type_count; i++) {
        if (strcmp(context->types[i]->name, name) == 0) {
            return NULL;  // Type with same name exists
        }
    
    for (int i = 0; i < context->protocol_count; i++) {
        if (strcmp(context->protocols[i]->name, name) == 0) {
            return NULL;  // Protocol already exists
        }
    }
    
    // Create new protocol
    Protocol* new_protocol = (Protocol*)malloc(sizeof(Protocol));
    new_protocol->base = create_type(context, name);
    new_protocol->name = strdup(name);
    new_protocol->parent = NULL;
    new_protocol->methods = NULL;
    new_protocol->method_count = 0;
    
    // Add to context
    context->protocols = (Protocol**)realloc(context->protocols, sizeof(Protocol*) * (context->protocol_count + 1));
    context->protocols[context->protocol_count++] = new_protocol;
    
    return new_protocol;
    }
}

Attribute* create_attribute(char* name, Type* type) {
    Attribute* attr = malloc(sizeof(Attribute));
    attr->name = strdup(name);
    attr->type = type;
    attr->value = NULL;
    attr->node = NULL;
    return attr;
}

// Create a new function
Method* create_function(Context* context, const char* name, char** param_names, Type** param_types, int param_count, Type* return_type) {
    // Check if function with this name already exists
    for (int i = 0; i < context->function_count; i++) {
        if (strcmp(context->functions[i]->name, name) == 0) {
            return NULL;  // Function already exists
        }
    }
    
    // Create new method/function
    Method* new_function = (Method*)malloc(sizeof(Method));
    new_function->name = strdup(name);
    new_function->param_names = (char**)malloc(sizeof(char*) * param_count);
    for (int i = 0; i < param_count; i++) {
        new_function->param_names[i] = strdup(param_names[i]);
    }
    new_function->param_types = (Type**)malloc(sizeof(Type*) * param_count);
    memcpy(new_function->param_types, param_types, sizeof(Type*) * param_count);
    new_function->param_count = param_count;
    new_function->return_type = return_type;
    new_function->inferred_return_type = return_type;
    new_function->node = NULL;
    
    // Add to context
    context->functions = (Method**)realloc(context->functions, sizeof(Method*) * (context->function_count + 1));
    context->functions[context->function_count++] = new_function;
    
    return new_function;
}
// Initialize context
Context* create_context() {
    Context* context = (Context*)malloc(sizeof(Context));
    context->types = NULL;
    context->type_count = 0;
    context->protocols = NULL;
    context->protocol_count = 0;
    context->functions = NULL;
    context->function_count = 0;
    
    // Initialize Hulk built-ins
    context->hulk_types = (char**)malloc(sizeof(char*) * 5);
    context->hulk_types[0] = strdup("String");
    context->hulk_types[1] = strdup("Boolean");
    context->hulk_types[2] = strdup("Number");
    context->hulk_types[3] = strdup("Object");
    context->hulk_types[4] = strdup("Range");
    context->hulk_type_count = 5;
    
    context->hulk_protocols = (char**)malloc(sizeof(char*) * 1);
    context->hulk_protocols[0] = strdup("Iterable");
    context->hulk_protocol_count = 1;
    
    context->hulk_functions = (char**)malloc(sizeof(char*) * 8);
    context->hulk_functions[0] = strdup("sqrt");
    context->hulk_functions[1] = strdup("sin");
    context->hulk_functions[2] = strdup("cos");
    context->hulk_functions[3] = strdup("exp");
    context->hulk_functions[4] = strdup("log");
    context->hulk_functions[5] = strdup("rand");
    context->hulk_functions[6] = strdup("print");
    context->hulk_functions[7] = strdup("range");
    context->hulk_function_count = 8;
    
    initialize_builtin_types(context);
    
    return context;
}

// Initialize built-in types
void initialize_builtin_types(Context* context) {
    // Initialize Object type
    ObjectType* object_type = (ObjectType*)malloc(sizeof(ObjectType));
    object_type->base.name = strdup("Object");
    object_type->base.parent = NULL;
    object_type->base.attributes = NULL;
    object_type->base.attribute_count = 0;
    object_type->base.methods = NULL;
    object_type->base.method_count = 0;
    object_type->base.param_names = NULL;
    object_type->base.param_types = NULL;
    object_type->base.param_count = 0;
    object_type->base.node = NULL;
    
    // Add to context
    context->types = (Type**)realloc(context->types, sizeof(Type*) * (context->type_count + 1));
    context->types[context->type_count++] = (Type*)object_type;
    
    // Initialize Number type
    NumberType* number_type = (NumberType*)malloc(sizeof(NumberType));
    number_type->base.name = strdup("Number");
    number_type->base.parent = (Type*)object_type;
    number_type->base.attributes = NULL;
    number_type->base.attribute_count = 0;
    number_type->base.methods = NULL;
    number_type->base.method_count = 0;
    number_type->base.param_names = NULL;
    number_type->base.param_types = NULL;
    number_type->base.param_count = 0;
    number_type->base.node = NULL;
    
    context->types = (Type**)realloc(context->types, sizeof(Type*) * (context->type_count + 1));
    context->types[context->type_count++] = (Type*)number_type;
    
    // Initialize Boolean type
    BooleanType* boolean_type = (BooleanType*)malloc(sizeof(BooleanType));
    boolean_type->base.name = strdup("Boolean");
    boolean_type->base.parent = (Type*)object_type;
    boolean_type->base.attributes = NULL;
    boolean_type->base.attribute_count = 0;
    boolean_type->base.methods = NULL;
    boolean_type->base.method_count = 0;
    boolean_type->base.param_names = NULL;
    boolean_type->base.param_types = NULL;
    boolean_type->base.param_count = 0;
    boolean_type->base.node = NULL;
    
    context->types = (Type**)realloc(context->types, sizeof(Type*) * (context->type_count + 1));
    context->types[context->type_count++] = (Type*)boolean_type;
    
    // Initialize String type
    StringType* string_type = (StringType*)malloc(sizeof(StringType));
    string_type->base.name = strdup("String");
    string_type->base.parent = (Type*)object_type;
    string_type->base.attributes = NULL;
    string_type->base.attribute_count = 0;
    string_type->base.methods = NULL;
    string_type->base.method_count = 0;
    string_type->base.param_names = NULL;
    string_type->base.param_types = NULL;
    string_type->base.param_count = 0;
    string_type->base.node = NULL;
    
    context->types = (Type**)realloc(context->types, sizeof(Type*) * (context->type_count + 1));
    context->types[context->type_count++] = (Type*)string_type;
    
    // Initialize Error type
    ErrorType* error_type = (ErrorType*)malloc(sizeof(ErrorType));
    error_type->base.name = strdup("<error>");
    error_type->base.parent = NULL;
    error_type->base.attributes = NULL;
    error_type->base.attribute_count = 0;
    error_type->base.methods = NULL;
    error_type->base.method_count = 0;
    error_type->base.param_names = NULL;
    error_type->base.param_types = NULL;
    error_type->base.param_count = 0;
    error_type->base.node = NULL;
    
    context->types = (Type**)realloc(context->types, sizeof(Type*) * (context->type_count + 1));
    context->types[context->type_count++] = (Type*)error_type;
    
    // Initialize Auto type
    AutoType* auto_type = (AutoType*)malloc(sizeof(AutoType));
    auto_type->base.name = strdup("<auto>");
    auto_type->base.parent = NULL;
    auto_type->base.attributes = NULL;
    auto_type->base.attribute_count = 0;
    auto_type->base.methods = NULL;
    auto_type->base.method_count = 0;
    auto_type->base.param_names = NULL;
    auto_type->base.param_types = NULL;
    auto_type->base.param_count = 0;
    auto_type->base.node = NULL;
    
    context->types = (Type**)realloc(context->types, sizeof(Type*) * (context->type_count + 1));
    context->types[context->type_count++] = (Type*)auto_type;

}

Scope* create_scope(Scope* parent) {
    Scope* scope = malloc(sizeof(Scope));
    scope->locals = NULL;
    scope->locals_count = 0;
    scope->parent = parent;
    scope->children = NULL;
    scope->children_count = 0;
    scope->index = (parent == NULL) ? 0 : parent->locals_count;
    
    if (parent != NULL) {
        parent->children_count++;
        parent->children = realloc(parent->children, parent->children_count * sizeof(Scope*));
        parent->children[parent->children_count - 1] = scope;
    }
    
    return scope;
}

// ---------------------------Funciones de utilidad------------------------------
bool conforms_to(Type* type, Type* other);
bool method_implements(Method* method, Method* other);

char* format_string(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    // Primera pasada: determinar el tamaño necesario
    int length = vsnprintf(NULL, 0, format, args);
    if (length < 0) {
        va_end(args);
        return NULL;
    }
    
    va_end(args);
    va_start(args, format);  // Reiniciamos los argumentos
    
    // Reservar memoria (incluyendo espacio para el '\0')
    char* buffer = (char*)malloc(length + 1);
    if (!buffer) {
        va_end(args);
        return NULL;
    }
    
    // Segunda pasada: escribir el string formateado
    vsnprintf(buffer, length + 1, format, args);
    va_end(args);
    
    return buffer;
}

Type* get_lowest_common_ancestor(Type** types, int type_count) {
    if (type_count == 0) {
        return NULL;
    }
    
    // Check for error types
    for (int i = 0; i < type_count; i++) {
        if (strcmp(types[i]->name, "<error>") == 0) {
            return types[i];  // Return error type
        }
    }
    
    // Check for auto types
    for (int i = 0; i < type_count; i++) {
        if (strcmp(types[i]->name, "<auto>") == 0) {
            return types[i];  // Return auto type
        }
    }
    
    Type* lca = types[0];
    for (int i = 1; i < type_count; i++) {
        bool found = false;
        Type* current = types[i];
        while (current != NULL) {
            if (current == lca) {
                found = true;
                break;
            }
            current = current->parent;
        }
        
        if (!found) {
            lca = lca->parent;  // Move up the hierarchy
            if (lca == NULL) {
                lca = context_get_type(NULL, "Object");  // Default to Object
                break;
            }
            i = 0;  // Start over with new LCA candidate
        }
    }
    
    return lca;
}

bool conforms_to(Type* type, Type* other) {
    if (other->name == "<auto>" || other->name == "<error>") {
        return true;
    }
    
    if (type->name == "<error>") {
        return true;
    }
    
    if (type->name == other->name && strcmp(type->name, other->name) == 0) {
        return true;
    }
    
    if (type->parent != NULL) {
        return conforms_to(type->parent, other);
    }
    
    return false;
}

bool method_implements(Method* method, Method* other) {
    if (!conforms_to(method->return_type, other->return_type)) {
        return false;
    }
    
    if (method->param_count != other->param_count) {
        return false;
    }
    
    for (int i = 0; i < method->param_count; i++) {
        if (!conforms_to(other->param_types[i], method->param_types[i])) {
            return false;
        }
    }
    
    return true;
}

// ---------------------------Funciones para manejar atributos----------------------------
Attribute* get_attribute(Type* type, char* name) {
    for (int i = 0; i < type->attribute_count; i++) {
        if (strcmp(type->attributes[i]->name, name) == 0) {
            return type->attributes[i];
        }
    }
    
    if (type->parent != NULL) {
        return  get_attribute(type->parent, name);
    }
    
    return NULL;
}

Attribute* define_attribute(Type* type, const char* name, Type* typex) {
    // Primero verificamos si el atributo ya existe
    for (int i = 0; i < type->attribute_count; i++) {
        if (strcmp(type->attributes[i]->name, name) == 0) {
            // Atributo ya existe, generamos error semántico
            return NULL;
        }
    }

    // Si no existe, lo creamos
    Attribute* attribute = (Attribute*)malloc(sizeof(Attribute));
    attribute->name = strdup(name);
    attribute->type = typex;
    attribute->value = NULL;
    attribute->node = NULL;

    // Añadimos el atributo al tipo
    type->attribute_count++;
    type->attributes = (Attribute**)realloc(type->attributes, sizeof(Attribute*) * type->attribute_count);
    type->attributes[type->attribute_count - 1] = attribute;

    return attribute;
}

void set_attribute_error(Type* type, const char* attr_name) {
    // Primero buscamos si el atributo ya existe
    for (int i = 0; i < type->attribute_count; i++) {
        if (strcmp(type->attributes[i]->name, attr_name) == 0) {
            // Convertir el atributo existente en un AttributeError
            free(type->attributes[i]->name);
            
            // Mantener la estructura Attribute pero marcarla como error
            type->attributes[i]->name = strdup(attr_name);
            type->attributes[i]->type = context_get_type(NULL, "<error>");
            return;
        }
    }

    // Si el atributo no existe, crear uno nuevo marcado como error
    Attribute* error_attr = (Attribute*)malloc(sizeof(Attribute));
    error_attr->name = strdup(attr_name);
    error_attr->type = context_get_type(NULL, "<error>");
    error_attr->value = NULL;
    error_attr->node = NULL;

    // Agregar al tipo
    type->attribute_count++;
    type->attributes = (Attribute**)realloc(type->attributes, sizeof(Attribute*) * type->attribute_count);
    type->attributes[type->attribute_count - 1] = error_attr;
}

//-------------------------------Funciones para manejar metodos-----------------------------
Method* get_method(Type* type, char* name) {
    for (int i = 0; i < type->method_count; i++) {
        if (strcmp(type->methods[i]->name, name) == 0) {
            return type->methods[i];
        }
    }
    
    if (type->parent != NULL) {
        return get_method(type->parent, name);
    }
    
    return NULL;
}

Method* define_method(Type* type, Context* ctx, const char* name, char** param_names, Type** param_types, int param_count, Type* return_type) {
    // Verificar si el método ya existe
    for (int i = 0; i < type->method_count; i++) {
        if (strcmp(type->methods[i]->name, name) == 0) {
            return NULL;
        }
    }

    // Crear el nuevo método
    Method* method = (Method*)malloc(sizeof(Method));
    method->name = strdup(name);
    method->param_names = (char**)malloc(sizeof(char*) * param_count);
    for (int i = 0; i < param_count; i++) {
        method->param_names[i] = strdup(param_names[i]);
    }
    method->param_types = (Type**)malloc(sizeof(Type*) * param_count);
    memcpy(method->param_types, param_types, sizeof(Type*) * param_count);
    method->param_count = param_count;
    method->return_type = return_type;
    method->inferred_return_type = return_type;
    method->node = NULL;

    // Agregar el método al tipo
    type->method_count++;
    type->methods = (Method**)realloc(type->methods, sizeof(Method*) * type->method_count);
    type->methods[type->method_count - 1] = method;

    // También agregarlo al contexto
    ctx->function_count++;
    ctx->functions = (Method**)realloc(ctx->functions, sizeof(Method*) * ctx->function_count);
    ctx->functions[ctx->function_count - 1] = method;

    return method;
}

void set_method_error(Type* type, const char* method_name) {
    // Primero buscamos si el método ya existe
    for (int i = 0; i < type->method_count; i++) {
        if (strcmp(type->methods[i]->name, method_name) == 0) {
            // Reemplazar el método existente con un MethodError
            free(type->methods[i]->name);
            for (int j = 0; j < type->methods[i]->param_count; j++) {
                free(type->methods[i]->param_names[j]);
            }
            free(type->methods[i]->param_names);
            free(type->methods[i]->param_types);
            
            // Convertir a MethodError (en este caso, sería similar a un Method normal pero marcado como error)
            type->methods[i]->name = strdup(method_name);
            type->methods[i]->param_names = NULL;
            type->methods[i]->param_types = NULL;
            type->methods[i]->param_count = 0;
            type->methods[i]->return_type = context_get_type(NULL, "<error>");
            type->methods[i]->inferred_return_type = context_get_type(NULL, "<error>");
            return;
        }
    }

    // Si el método no existe, crear uno nuevo marcado como error
    Method* error_method = (Method*)malloc(sizeof(Method));
    error_method->name = strdup(method_name);
    error_method->param_names = NULL;
    error_method->param_types = NULL;
    error_method->param_count = 0;
    error_method->return_type = context_get_type(NULL, "<error>");
    error_method->inferred_return_type = context_get_type(NULL, "<error>");
    error_method->node = NULL;

    // Agregar al tipo
    type->method_count++;
    type->methods = (Method**)realloc(type->methods, sizeof(Method*) * type->method_count);
    type->methods[type->method_count - 1] = error_method;
}

Method* protocol_get_method(Protocol* protocol, char* name) {
    for (int i = 0; i < protocol->method_count; i++) {
        if (strcmp(protocol->methods[i]->name, name) == 0) {
            return protocol->methods[i];
        }
    }
    
    if (protocol->parent != NULL) {
        return protocol_get_method(protocol->parent, name);
    }
    
    return NULL;
}

// ----------------------------Funciones para manejar el contexto--------------------------------------

Type* context_get_type(Context* ctx, char* name) {
    for (int i = 0; i < ctx->type_count; i++) {
        if (strcmp(ctx->types[i]->name, name) == 0) {
            return ctx->types[i];
        }
    }
    
    return NULL;
}

Protocol* context_get_protocol(Context* ctx, char* name) {
    for (int i = 0; i < ctx->protocol_count; i++) {
        if (strcmp(ctx->protocols[i]->name, name) == 0) {
            return ctx->protocols[i];
        }
    }
    
    return NULL;
}

Type* context_get_type_or_protocol(Context* ctx, char* name) {
    for (int i = 0; i < ctx->type_count; i++) {
        if (strcmp(ctx->types[i]->name, name) == 0) {
            return ctx->types[i];
        }
    }
    
    for (int i = 0; i < ctx->protocol_count; i++) {
        if (strcmp(ctx->protocols[i]->name, name) == 0) {
            return (Type*)ctx->protocols[i];
        }
    }

    return NULL;
}
void context_set_type_error(Context* ctx, const char* type_name) {
    // Buscar el tipo en el contexto
    for (int i = 0; i < ctx->type_count; i++) {
        if (strcmp(ctx->types[i]->name, type_name) == 0) {
            // Crear un nuevo ErrorType para reemplazar el tipo existente
            ErrorType* error_type = (ErrorType*)malloc(sizeof(ErrorType));
            error_type->base.name = strdup("<error>");
            error_type->base.parent = NULL;
            error_type->base.attributes = NULL;
            error_type->base.attribute_count = 0;
            error_type->base.methods = NULL;
            error_type->base.method_count = 0;
            error_type->base.param_names = NULL;
            error_type->base.param_types = NULL;
            error_type->base.param_count = 0;
            error_type->base.node = NULL;
            
            // Liberar el tipo existente
            free_type(ctx->types[i]);
            
            // Reemplazar con el ErrorType
            ctx->types[i] = (Type*)error_type;
            return;
        }
    }
    
    // Si el tipo no existe, crear uno nuevo con ErrorType
    ErrorType* error_type = (ErrorType*)malloc(sizeof(ErrorType));
    error_type->base.name = strdup(type_name);  // Mantener el nombre original para referencia
    error_type->base.parent = NULL;
    error_type->base.attributes = NULL;
    error_type->base.attribute_count = 0;
    error_type->base.methods = NULL;
    error_type->base.method_count = 0;
    error_type->base.param_names = NULL;
    error_type->base.param_types = NULL;
    error_type->base.param_count = 0;
    error_type->base.node = NULL;
    
    // Agregar al contexto
    ctx->types = (Type**)realloc(ctx->types, sizeof(Type*) * (ctx->type_count + 1));
    ctx->types[ctx->type_count++] = (Type*)error_type;
}

Method* context_get_function(Context* ctx, char* name) {
    for (int i = 0; i < ctx->function_count; i++) {
        if (strcmp(ctx->functions[i]->name, name) == 0) {
            return ctx->functions[i];
        }
    }
    
    return NULL;
}

void context_set_function_error(Context* ctx, const char* function_name) {
    // Buscar la función en el contexto
    for (int i = 0; i < ctx->function_count; i++) {
        if (strcmp(ctx->functions[i]->name, function_name) == 0) {
            // Convertir la función existente en un "MethodError"
            free(ctx->functions[i]->name);
            
            // Liberar parámetros si existen
            for (int j = 0; j < ctx->functions[i]->param_count; j++) {
                free(ctx->functions[i]->param_names[j]);
            }
            free(ctx->functions[i]->param_names);
            free(ctx->functions[i]->param_types);
            
            // Configurar como función error
            ctx->functions[i]->name = strdup(function_name);
            ctx->functions[i]->param_names = NULL;
            ctx->functions[i]->param_types = NULL;
            ctx->functions[i]->param_count = 0;
            ctx->functions[i]->return_type = context_get_type(ctx, "<error>");
            ctx->functions[i]->inferred_return_type = context_get_type(ctx, "<error>");
            return;
        }
    }

    // Si la función no existe, crear una nueva marcada como error
    Method* error_function = (Method*)malloc(sizeof(Method));
    error_function->name = strdup(function_name);
    error_function->param_names = NULL;
    error_function->param_types = NULL;
    error_function->param_count = 0;
    error_function->return_type = context_get_type(ctx, "<error>");
    error_function->inferred_return_type = context_get_type(ctx, "<error>");
    error_function->node = NULL;

    // Agregar al contexto
    ctx->function_count++;
    ctx->functions = (Method**)realloc(ctx->functions, sizeof(Method*) * (ctx->function_count + 1));
    ctx->functions[ctx->function_count - 1] = error_function;
}

//--------------------------------Funciones para manejar el scope-------------------------------------
VariableInfo* scope_find_variable(Scope* scope, char* name, int index) {
    int start = (index == -1) ? 0 : index;
    int end = (index == -1) ? scope->locals_count : index + 1;
    
    for (int i = start; i < end; i++) {
        if (strcmp(scope->locals[i]->name, name) == 0) {
            return scope->locals[i];
        }
    }
    
    if (scope->parent != NULL) {
        return scope_find_variable(scope->parent, name, scope->index);
    }
    
    return NULL;
}

VariableInfo* scope_define_variable(Scope* scope, char* name, Type* type, bool is_parameter) {
    VariableInfo* info = malloc(sizeof(VariableInfo));
    info->name = strdup(name);
    info->type = type;
    info->is_error = false;
    info->is_parameter = is_parameter;
    info->value = NULL;
    
    scope->locals_count++;
    scope->locals = realloc(scope->locals, scope->locals_count * sizeof(VariableInfo*));
    scope->locals[scope->locals_count - 1] = info;
    
    return info;
}

//--------------------------------- Funciones para liberar memoria-----------------------------------
void free_type(Type* type) {
    if (type == NULL) return;
    
    free(type->name);
    
    for (int i = 0; i < type->attribute_count; i++) {
        free(type->attributes[i]->name);
        free(type->attributes[i]);
    }
    free(type->attributes);
    
    for (int i = 0; i < type->method_count; i++) {
        free(type->methods[i]->name);
        for (int j = 0; j < type->methods[i]->param_count; j++) {
            free(type->methods[i]->param_names[j]);
        }
        free(type->methods[i]->param_names);
        free(type->methods[i]->param_types);
        free(type->methods[i]);
    }
    free(type->methods);
    
    for (int i = 0; i < type->param_count; i++) {
        free(type->param_names[i]);
    }
    free(type->param_names);
    free(type->param_types);
    
    free(type);
}

void free_protocol(Protocol* protocol) {
    if (protocol == NULL) return;
    
    free(protocol->name);
    
    for (int i = 0; i < protocol->method_count; i++) {
        free(protocol->methods[i]->name);
        for (int j = 0; j < protocol->methods[i]->param_count; j++) {
            free(protocol->methods[i]->param_names[j]);
        }
        free(protocol->methods[i]->param_names);
        free(protocol->methods[i]->param_types);
        free(protocol->methods[i]);
    }
    free(protocol->methods);
    
    free(protocol);
}

void free_context(Context* ctx) {
    for (int i = 0; i < ctx->type_count; i++) {
        free_type(ctx->types[i]);
    }
    free(ctx->types);
    
    for (int i = 0; i < ctx->protocol_count; i++) {
        free_protocol(ctx->protocols[i]);
    }
    free(ctx->protocols);
    
    for (int i = 0; i < ctx->function_count; i++) {
        free(ctx->functions[i]->name);
        for (int j = 0; j < ctx->functions[i]->param_count; j++) {
            free(ctx->functions[i]->param_names[j]);
        }
        free(ctx->functions[i]->param_names);
        free(ctx->functions[i]->param_types);
        free(ctx->functions[i]);
    }
    free(ctx->functions);
    
    for (int i = 0; i < ctx->hulk_type_count; i++) {
        free(ctx->hulk_types[i]);
    }
    free(ctx->hulk_types);
    
    for (int i = 0; i < ctx->hulk_protocol_count; i++) {
        free(ctx->hulk_protocols[i]);
    }
    free(ctx->hulk_protocols);
    
    for (int i = 0; i < ctx->hulk_function_count; i++) {
        free(ctx->hulk_functions[i]);
    }
    free(ctx->hulk_functions);
    
    free(ctx);
}

void free_scope(Scope* scope) {
    for (int i = 0; i < scope->locals_count; i++) {
        free(scope->locals[i]->name);
        free(scope->locals[i]);
    }
    free(scope->locals);
    
    for (int i = 0; i < scope->children_count; i++) {
        free_scope(scope->children[i]);
    }
    free(scope->children);
    
    free(scope);
}
#endif