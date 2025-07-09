#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#define MAX_BASES 4
#define MAX_ERRORS 256

typedef struct {
    char* messages[MAX_ERRORS];
    int count;
} ErrorList;

void add_error(ErrorList* list, const char* fmt, ...);

typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_TYPE
} SymbolKind;

typedef struct Symbol {
    char name[64];
    char type[64];   // int, string, vector, Object, nombre de tipo
    char dynamic_type[64]; 
    SymbolKind kind;
    struct Symbol* next;
    int last_temp_id;
} Symbol;

typedef struct Member {
  char name[32];
  char type[32];
  struct Member* next;
   ASTNode* body; 
} Member;

typedef struct TypeEntry {
  char name[32];         // "Box"
  char bases[8][32];     // Base types para herencia
  int num_bases;
  Member* members;
  char type_params[8][32];  // E.g. T, U, ...
  int num_params;           // Número de parámetros genéricos
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

// Tipos
void init_type_table(TypeTable* table);
TypeEntry* lookup_type(TypeTable* table, const char* name);
// semantic.h
void insert_type(TypeTable* table, const char* name, const char** bases, int num_bases, ASTNode* members_node);

void add_member_to_type(TypeTable* table, const char* type_name, const char* name, const char* type ,ASTNode* body);
const char* lookup_member_type(TypeTable* table, const char* type_name, const char* member_name);
static void process_members(ASTNode* node, SymbolTable* sym_table, TypeTable* type_table, const char* type_name);
// Subtipo / conformidad
int conforms(TypeTable* table, const char* child, const char* parent);
void parse_type_members(ASTNode* node, const char* type_name, TypeTable* type_table);
// Semántica
void check_semantics(ASTNode* node, SymbolTable* sym_table, TypeTable* type_table, const char* expected_return, ErrorList* error_list);

// Inferencia de tipos
const char* infer_type(ASTNode* node, SymbolTable* sym_table, TypeTable* type_table, ErrorList* error_list);

#endif
