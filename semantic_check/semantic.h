#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include <stdbool.h>
#define MAX_BASES 4
#define MAX_ERRORS 256
#define MAX_METHODS 32
typedef struct {
    char* message;   // Mensaje dinámico
    int line;        // Línea del error
    int column;      // Columna del error
} SemanticError;

typedef struct {
    SemanticError errors[MAX_ERRORS];
    int count;
} ErrorList;


typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_TYPE
} SymbolKind;

typedef struct Symbol {
    char name[64];
    char type[64];   // int, string, vector, Object, nombre de tipo
    char dynamic_type[64]; 
    char last_branch_dynamic_type[64];
    char last_branch_dynamic_type_else[64]; 
    SymbolKind kind;
    struct Symbol* next;
    int last_temp_id;
    ASTNode* func_decl_node; 
} Symbol;

typedef struct Member {
  char name[32];
  char type[32];
  struct Member* next;
   ASTNode* body; 
   int default_value;
} Member;

typedef struct TypeEntry {
  char name[32];         // "Box"
  char bases[8][32];     // Base types para herencia
  int num_bases;
  Member* members;
  char type_params[8][32];  // E.g. T, U, ...
  int num_params;           // Número de parámetros genéricos

   int method_count;
  char method_names[MAX_METHODS][32];
  char method_signatures[MAX_METHODS][64];
  char method_impls[MAX_METHODS][32];  

  struct TypeEntry* next;
} TypeEntry;

typedef struct TypeInstance {
  char name[64];         // "Box[Number]"
  char base_template[32];  // "Box"
  char type_args[8][32];   // "Number"
  int num_args;
  Member* concretized_members; // Miembros con tipos concretos
  struct TypeInstance* next;
} TypeInstance;

typedef struct {
  TypeEntry* head;         // Todos los tipos base
  TypeInstance* instances; // Todas las instancias concretas
} TypeTable;

typedef struct SymbolTable {
    Symbol* head;
} SymbolTable;


// Símbolos
void init_symbol_table(SymbolTable* table);
Symbol* lookup(SymbolTable* table, const char* name);
void insert_symbol(SymbolTable* table, const char* name, const char* type, SymbolKind kind);
void add_error(ErrorList* list, int line, int column, const char* fmt, ...);
// Tipos
void init_type_table(TypeTable* table);
TypeEntry* lookup_type(TypeTable* table, const char* name);
// semantic.h
void insert_type(
    TypeTable* table,
    const char* name,
    const char** bases,
    int num_bases,
    ASTNode* bases_node,  
    ASTNode* members_node, 
    ErrorList* error_list
);
int has_circular_dependency(TypeTable* table, const char* type_name, const char* current);
void add_member_to_type(TypeTable* table, const char* type_name, const char* name, const char* type, ASTNode* body, int default_value);
const char* lookup_member_type(TypeTable* table, const char* type_name, const char* member_name);
static void process_members(ASTNode* node, SymbolTable* sym_table, TypeTable* type_table, const char* type_name);
// Subtipo / conformidad
int conforms(TypeTable* table, const char* child, const char* parent);
void parse_type_members(ASTNode* node, const char* type_name, TypeTable* type_table);
// Semántica
void check_semantics(ASTNode* node, SymbolTable* sym_table, TypeTable* type_table, const char* expected_return, ErrorList* error_list);
const char* infer_type(ASTNode* node, SymbolTable* sym_table, TypeTable* type_table, ErrorList* error_list);
// Inferencia de tipos
const char* infer_type(ASTNode* node, SymbolTable* sym_table, TypeTable* type_table, ErrorList* error_list);
void emit_method_signature(char* buffer, size_t buf_size,
                           const char* return_type,
                           const char* class_name);
void build_vtable_info(TypeTable* table);
void print_vtable_info(TypeTable* table);
int get_method_index(TypeTable* table, const char* type_name, const char* method_name);
TypeEntry* lookup_class(TypeTable* table, const char* name);
const char* map_type_to_llvm(const char* type);

#endif
