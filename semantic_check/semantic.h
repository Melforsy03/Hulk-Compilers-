typedef struct Context Context;
typedef struct Type Type;
typedef struct Protocol Protocol;

Context* context_create();
void context_free(Context* context);

Type* context_create_type(Context* context, const char* name);
Protocol* context_create_protocol(Context* context, const char* name);
bool context_has_type(Context* context, const char* name);
bool context_has_protocol(Context* context, const char* name);
void context_set_type_error(Context* context, const char* name);
void context_set_protocol_error(Context* context, const char* name);

Type* type_get(Context* context, const char* name);
void type_set_params(Type* type, const char** param_names, Type** param_types, int count);
void type_define_attribute(Type* type, const char* name, Type* attr_type);
void type_define_method(Type* type, const char* name, const char** param_names, int param_count, Type* return_type);

void protocol_define_method(Protocol* protocol, const char* name, const char** param_names, int param_count, Type* return_type);
void type_set_ast_node(Type* type, Node* node);
void function_set_ast_node(Function* func, Node* node);
const char* type_get_name(Type* type);
Type* type_get_parent(Type* type);
const char* protocol_get_name(Protocol* protocol);

Function* context_create_function(Context* context, const char* name, const char** param_names, int param_count, Type** param_types, Type* return_type);

// Tipos básicos
Type* object_type();
Type* number_type();
Type* boolean_type();
Type* string_type();
Type* auto_type();
Type* error_type();

Protocol* error_protocol();

// Funciones de comparación
bool type_equals(Type* a, Type* b);
bool protocol_equals(Protocol* a, Protocol* b);
bool type_conforms_to(Type* type, Type* other);

// Funciones para obtener tipos/protocolos
Type* context_get_type(Context* context, const char* name);
Protocol* context_get_protocol(Context* context, const char* name);
Type* context_get_type_or_protocol(Context* context, const char* name); // Lanza SemanticError si no existe

// Funciones para verificar existencia
bool context_has_type(Context* context, const char* name);
bool context_has_protocol(Context* context, const char* name);
bool context_has_function(Context* context, const char* name);

// Funciones para marcar errores
void context_set_type_error(Context* context, const char* name);
void context_set_protocol_error(Context* context, const char* name);
void context_set_function_error(Context* context, const char* name);
void type_set_attribute_error(Type* type, const char* name);
void type_set_method_error(Type* type, const char* name);

typedef struct Type {
    char* name;
    Type* parent;
    List* param_names;
    List* param_types;
    HashTable* attributes;
    HashTable* methods;
    Node* node;
} Type;

typedef struct Method {
    char* name;
    List* param_names;
    List* param_types;
    Type* return_type;
    Type* inferred_return_type;
    Node* node;
} Method;

typedef struct Attribute {
    char* name;
    Type* type;
    Node* node;
} Attribute;


bool type_is_auto(Type* type);
bool type_is_error(Type* type);
bool type_conforms_to(Type* type, Type* other);

Attribute* type_get_attribute(Type* type, const char* name);
Method* type_get_method(Type* type, const char* name);

void type_set_inferred_return_type(Method* method, Type* type);