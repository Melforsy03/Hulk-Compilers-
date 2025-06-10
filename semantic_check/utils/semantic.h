#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Forward declarations
typedef struct Type Type;
typedef struct Protocol Protocol;
typedef struct Method Method;
typedef struct Attribute Attribute;
typedef struct Context Context;
typedef struct Scope Scope;
typedef struct VariableInfo VariableInfo;

// Error types
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

// Attribute structures
struct Attribute {
    char* name;
    Type* type;
    void* value;
    void* node;  // Assuming this is some AST node pointer
};

typedef struct AttributeError {
    Attribute base;
} AttributeError;

// Method structures
struct Method {
    char* name;
    char** param_names;
    Type** param_types;
    int param_count;
    Type* return_type;
    Type* inferred_return_type;
    void* node;  // Assuming this is some AST node pointer
};

typedef struct MethodError {
    Method base;
} MethodError;

// Protocol structure
struct Protocol {
    char* name;
    Protocol* parent;
    Method** methods;
    int method_count;
};

// Type structure
struct Type {
    char* name;
    Attribute** attributes;
    int attribute_count;
    Method** methods;
    int method_count;
    Type* parent;
    char** param_names;
    Type** param_types;
    int param_count;
    void* node;  // Assuming this is some AST node pointer
};

// Context structure
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

// Variable info structure
struct VariableInfo {
    char* name;
    Type* type;
    bool is_error;
    bool is_parameter;
    void* value;
};

// Scope structure
struct Scope {
    VariableInfo** locals;
    int local_count;
    Scope* parent;
    Scope** children;
    int child_count;
    int index;
};

// Function prototypes
Type* create_type(Context* context, const char* name);
Type* get_type(Context* context, const char* name);
Protocol* create_protocol(Context* context, const char* name);
Protocol* get_protocol(Context* context, const char* name);
Method* create_function(Context* context, const char* name, char** param_names, Type** param_types, int param_count, Type* return_type);
Method* get_function(Context* context, const char* name);

// Helper functions
char* strdup(const char* s) {
    size_t len = strlen(s) + 1;
    char* p = (char*)malloc(len);
    if (p) {
        memcpy(p, s, len);
    }
    return p;
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

// Get type by name
Type* get_type(Context* context, const char* name) {
    for (int i = 0; i < context->type_count; i++) {
        if (strcmp(context->types[i]->name, name) == 0) {
            return context->types[i];
        }
    }
    return NULL;  // Type not found
}

// Create a new protocol
Protocol* create_protocol(Context* context, const char* name) {
    // Check if type or protocol with this name already exists
    for (int i = 0; i < context->type_count; i++) {
        if (strcmp(context->types[i]->name, name) == 0) {
            return NULL;  // Type with same name exists
        }
    }
    
    for (int i = 0; i < context->protocol_count; i++) {
        if (strcmp(context->protocols[i]->name, name) == 0) {
            return NULL;  // Protocol already exists
        }
    }
    
    // Create new protocol
    Protocol* new_protocol = (Protocol*)malloc(sizeof(Protocol));
    new_protocol->name = strdup(name);
    new_protocol->parent = NULL;
    new_protocol->methods = NULL;
    new_protocol->method_count = 0;
    
    // Add to context
    context->protocols = (Protocol**)realloc(context->protocols, sizeof(Protocol*) * (context->protocol_count + 1));
    context->protocols[context->protocol_count++] = new_protocol;
    
    return new_protocol;
}

// Get protocol by name
Protocol* get_protocol(Context* context, const char* name) {
    for (int i = 0; i < context->protocol_count; i++) {
        if (strcmp(context->protocols[i]->name, name) == 0) {
            return context->protocols[i];
        }
    }
    return NULL;  // Protocol not found
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

// Get function by name
Method* get_function(Context* context, const char* name) {
    for (int i = 0; i < context->function_count; i++) {
        if (strcmp(context->functions[i]->name, name) == 0) {
            return context->functions[i];
        }
    }
    return NULL;  // Function not found
}

// Create a new scope
Scope* create_scope(Scope* parent) {
    Scope* new_scope = (Scope*)malloc(sizeof(Scope));
    new_scope->locals = NULL;
    new_scope->local_count = 0;
    new_scope->parent = parent;
    new_scope->children = NULL;
    new_scope->child_count = 0;
    new_scope->index = (parent == NULL) ? 0 : parent->local_count;
    
    if (parent != NULL) {
        parent->children = (Scope**)realloc(parent->children, sizeof(Scope*) * (parent->child_count + 1));
        parent->children[parent->child_count++] = new_scope;
    }
    
    return new_scope;
}

// Define variable in scope
VariableInfo* define_variable(Scope* scope, const char* vname, Type* vtype, bool is_parameter) {
    VariableInfo* info = (VariableInfo*)malloc(sizeof(VariableInfo));
    info->name = strdup(vname);
    info->type = vtype;
    info->is_error = false;
    info->is_parameter = is_parameter;
    info->value = NULL;
    
    scope->locals = (VariableInfo**)realloc(scope->locals, sizeof(VariableInfo*) * (scope->local_count + 1));
    scope->locals[scope->local_count++] = info;
    
    return info;
}

// Find variable in scope hierarchy
VariableInfo* find_variable(Scope* scope, const char* vname, int index) {
    // Search in current scope
    int start = (index == -1) ? 0 : index;
    int end = (index == -1) ? scope->local_count : index;
    
    for (int i = start; i < end; i++) {
        if (strcmp(scope->locals[i]->name, vname) == 0) {
            return scope->locals[i];
        }
    }
    
    // Search in parent scope if not found
    if (scope->parent != NULL) {
        return find_variable(scope->parent, vname, scope->index);
    }
    
    return NULL;  // Variable not found
}

// Get lowest common ancestor type
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
        // Implement conforms_to logic here (simplified for example)
        // In a real implementation, this would check the type hierarchy
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
                lca = get_type(NULL, "Object");  // Default to Object
                break;
            }
            i = 0;  // Start over with new LCA candidate
        }
    }
    
    return lca;
}

// Main function for testing
int main() {
    Context* context = create_context();
    
    // Create a new type
    Type* person_type = create_type(context, "Person");
    Type* object_type = get_type(context, "Object");
    person_type->parent = object_type;
    
    // Create a method for the type
    Method* speak_method = (Method*)malloc(sizeof(Method));
    speak_method->name = strdup("speak");
    speak_method->param_names = NULL;
    speak_method->param_types = NULL;
    speak_method->param_count = 0;
    speak_method->return_type = get_type(context, "String");
    speak_method->inferred_return_type = get_type(context, "String");
    speak_method->node = NULL;
    
    person_type->methods = (Method**)realloc(person_type->methods, sizeof(Method*) * (person_type->method_count + 1));
    person_type->methods[person_type->method_count++] = speak_method;
    
    // Create a scope
    Scope* global_scope = create_scope(NULL);
    Scope* function_scope = create_scope(global_scope);
    
    // Define some variables
    define_variable(global_scope, "x", get_type(context, "Number"), false);
    define_variable(function_scope, "y", get_type(context, "String"), false);
    
    // Find variables
    VariableInfo* x_info = find_variable(global_scope, "x", -1);
    VariableInfo* y_info = find_variable(function_scope, "y", -1);
    
    if (x_info) {
        printf("Found variable %s of type %s\n", x_info->name, x_info->type->name);
    }
    
    if (y_info) {
        printf("Found variable %s of type %s\n", y_info->name, y_info->type->name);
    }
    
   
    return 0;
}