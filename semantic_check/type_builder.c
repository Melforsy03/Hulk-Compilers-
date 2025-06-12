#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "type_builder.h"
#include "semantic.h" 
#include "semantic_errors.h" 
#include "../ast_nodes/ast_nodes.h"


// Función principal para construir los tipos
void build_types(Context* context, Node* program, HulkErrorList* output_errors) {
    TypeBuilder builder;
    builder.context = context;
    builder.current_type = NULL;
    
    // Inicializamos la lista de errores directamente
    output_errors = HulkErrorList_create();
    builder.errors = output_errors;
    builder.error_count = 0;

    visit_program(&builder, program);
}