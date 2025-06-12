#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "type_builder.h"
#include "semantic.h" 
#include "semantic_errors.h" 
#include "../parser/ast_nodes.h"


// Función principal para construir los tipos
void build_types(Context* context, ProgramNode* ast, HulkErrorList* output_errors) {
    TypeBuilder builder;
    builder.context = context;
    builder.current_type = NULL;
    
    // Inicializamos la lista de errores directamente
    output_errors = HulkErrorList_create();
    builder.errors = output_errors;
    builder.errors->count = 0;

    tb_visit_program(&builder, ast);
}