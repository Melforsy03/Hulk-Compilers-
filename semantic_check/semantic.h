#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

#define MAX_BASES 4

typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_TYPE
} SymbolKind;

typedef struct Symbol {
    char name[32];
    char type[32];   // int, string, vector, Object, nombre de tipo
    SymbolKind kind;
    struct Symbol* next;
} Symbol;

typedef struct Member {
    char name[32];
    char type[32];
    struct Member* next;
} Member;

typedef struct TypeEntry {
    char name[32];
    char bases[MAX_BASES][32];
    int num_bases;
    Member* members;
    struct TypeEntry* next;
} TypeEntry;

typedef struct SymbolTable {
    Symbol* head;
} SymbolTable;

typedef struct TypeTable {
    TypeEntry* head;
} TypeTable;

// Símbolos
void init_symbol_table(SymbolTable* table);
Symbol* lookup(SymbolTable* table, const char* name);
void insert_symbol(SymbolTable* table, const char* name, const char* type, SymbolKind kind);

// Tipos
void init_type_table(TypeTable* table);
TypeEntry* lookup_type(TypeTable* table, const char* name);
void insert_type(TypeTable* table, const char* name, const char** bases, int num_bases);
void add_member_to_type(TypeTable* table, const char* type_name, const char* member_name, const char* member_type);
const char* lookup_member_type(TypeTable* table, const char* type_name, const char* member_name);

// Subtipo / conformidad
int conforms(TypeTable* table, const char* child, const char* parent);

// Semántica
void check_semantics(ASTNode* node, SymbolTable* sym_table, TypeTable* type_table, const char* expected_return);

// Inferencia de tipos
const char* infer_type(ASTNode* node, SymbolTable* sym_table, TypeTable* type_table);

#endif
