typedef struct Variable {
    char* name;
    Type* type;
    bool is_param;
} Variable;

typedef struct Scope {
    HashTable* variables;
    struct Scope* parent;
} Scope;

Scope* scope_create();
Scope* scope_create_child(Scope* parent);
void scope_free(Scope* scope);

Variable* scope_define_variable(Scope* scope, const char* name, Type* type, bool is_param);
Variable* scope_find_variable(Scope* scope, const char* name);
bool scope_is_local(Scope* scope, const char* name);