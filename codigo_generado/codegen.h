#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "semantic.h"
#include <stdio.h>

typedef struct {
    char func_names[100][64];   // hasta 100 funciones
    char return_types[100][8];  // "i32" o "void"
    int count;
} ReturnTypeTable;

typedef struct {
    FILE* out;      // Archivo de salida
    int temp_count; // Para generar t0, t1, t2...
    int indent; 
    SymbolTable* sym_table;  // para variables
    TypeTable* type_table;   // para structs y miembros    // Para identar bloques
    ReturnTypeTable return_table; // Tabla de tipos de retorno
     Symbol* last_call_args[8];
} CodeGenContext;

typedef struct {
  ASTNode** funcs;
  int count;
  int capacity;
} FuncBuffer;
Symbol* create_symbol(const char* name, const char* type, SymbolKind kind);
void init_codegen(CodeGenContext* ctx, FILE* out);
void generate_code(CodeGenContext* ctx, ASTNode* node);
void free_codegen(CodeGenContext* ctx);
static void codegen_stmt(CodeGenContext* ctx, ASTNode* node);
void codegen_function_decl(CodeGenContext* ctx, ASTNode* node);
int get_member_index(TypeTable* table, const char* type_name, const char* member_name);
void emit_structs(CodeGenContext* ctx, TypeTable* type_table) ;
TypeInstance* instantiate_generic(TypeTable* table, const char* template_name, const char** args, int num_args);
void resolve_virtual_methods(TypeTable* table);
// Emite las vtables globales para cada tipo registrado.
void emit_vtables(CodeGenContext* ctx, TypeTable* table);
int get_method_count(TypeTable* table, const char* type_name);
//Despacho dinámico: carga y llama método desde la vtable.
char* emit_virtual_call(CodeGenContext* ctx, Symbol* s_obj, const char* method_name);

#endif
