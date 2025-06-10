#include <stdio.h>
#include <stdlib.h>
#include "ast_nodes.h"
#include "semanticErrors.h"
#include "typeCollector.c"
#include "semantic.h"


// Función principal para el análisis semántico
HulkErrorList* semantic_analysis(ProgramNode* ast) {
    // 1. Crear lista de errores
    HulkErrorList* errors = HulkErrorList_create();
    
    // 2. Fase de recolección de tipos
    TypeCollector collector;
    TypeCollector_init(&collector, errors);
    TypeCollector_visit_program(&collector, ast);
    
    // Si hay errores en la recolección, terminar
    if (errors->count > 0) {
        return errors;
    }
    
    // 3. Fase de construcción de tipos
    HulkSemanticError* builder_errors;
    int builder_error_count;
    build_types(collector.context, ast, &builder_errors, &builder_error_count);
    
    // Agregar errores del builder a la lista
    for (int i = 0; i < builder_error_count; i++) {
        HulkErrorList_add(errors, (HulkError*)&builder_errors[i]);
    }
    
    // Si hay errores en la construcción, terminar
    if (errors->count > 0) {
        free(builder_errors);
        return errors;
    }
    
    // 4. Fase de verificación de tipos
    type_check_program(ast, collector.context);
    
    // (Nota: type_check_program ya maneja sus propios errores)
    
    // 5. Limpieza
    free(builder_errors);
    destroy_context(collector.context);
    
    return errors;
}

// Ejemplo de uso desde el main
int main() {
    // Suponiendo que ya tienes el AST del parser
    ProgramNode* ast = parse_program(); // Esta función sería la del parser
    
    // Realizar análisis semántico
    HulkErrorList* semantic_errors = semantic_analysis(ast);
    
    // Mostrar errores si los hay
    if (semantic_errors->count > 0) {
        printf("Errores semánticos encontrados:\n");
        for (int i = 0; i < semantic_errors->count; i++) {
            char* error_str = HulkError_to_string(semantic_errors->errors[i]);
            printf("%s\n", error_str);
            free(error_str);
        }
        HulkErrorList_destroy(semantic_errors);
        return 1;
    }
    
    printf("El programa es semánticamente correcto.\n");
    HulkErrorList_destroy(semantic_errors);
    
    // Liberar memoria del AST
    free_ast(ast);
    
    return 0;
}