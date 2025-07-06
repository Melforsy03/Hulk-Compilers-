#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "semantic.h"
#include <stdio.h>

typedef struct {
    FILE* out;      // Archivo de salida
    int temp_count; // Para generar t0, t1, t2...
    int indent;     // Para identar bloques
} CodeGenContext;

void init_codegen(CodeGenContext* ctx, FILE* out);
void generate_code(CodeGenContext* ctx, ASTNode* node);
void free_codegen(CodeGenContext* ctx);
static void codegen_stmt(CodeGenContext* ctx, ASTNode* node);

#endif
