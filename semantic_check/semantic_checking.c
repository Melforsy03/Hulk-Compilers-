#include <stdio.h>
#include <stdlib.h>
#include "../parser/ast_nodes.h"
#include "semantic_errors.h"
#include "type_collector.c"
#include "type_builder.c"
#include "type_checker.c"
#include "semantic.h"


// Función principal para el análisis semántico
HulkErrorList* semantic_analysis(ProgramNode* ast) {
    // 1. Crear lista de errores
    HulkErrorList* errors = HulkErrorList_create();
    
    // 2. Fase de recolección de tipos
    TypeCollector collector = collect_types(errors, ast);
    
    // Si hay errores en la recolección, terminar
    if (errors->count > 0) {
        return errors;
    }
    
    // 3. Fase de construcción de tipos
    HulkErrorList builder_errors;
    build_types(collector.context, ast, &builder_errors);

    // Los errores ya están en builder_errors, no necesitamos conversiones
    // Si hay errores, terminar
    if (builder_errors.count > 0) {
        // Mover los errores a la lista principal si es necesario
        for (int i = 0; i < builder_errors.count; i++) {
            HulkErrorList_add(errors, builder_errors.errors[i]);
        }
        HulkErrorList_destroy(&builder_errors);
        return errors;
    }
        
    
    // 4. Fase de verificación de tipos y anotaciones
    HulkErrorList checker_errors;
    type_check_program(ast, collector.context, &checker_errors);
    
    if (checker_errors.count > 0) {
        // Mover los errores a la lista principal si es necesario
        for (int i = 0; i < checker_errors.count; i++) {
            HulkErrorList_add(errors, checker_errors.errors[i]);
        }
        HulkErrorList_destroy(&checker_errors);
        return errors;
    }
    // 5. Limpieza
   
    free(collector.context);
    
    return errors;
}

// Ejemplo de uso desde el main
int main() {
    // Suponiendo que ya tienes el AST del parser
    ProgramNode* ast = NULL; // Esta función sería la del parser
    
    // Realizar análisis semántico
    HulkErrorList* semantic_errors = semantic_analysis(ast);
    
    // Mostrar errores si los hay
    if (semantic_errors->count > 0) {
        printf("Errores semánticos encontrados:\n");
        for (int i = 0; i < semantic_errors->count; i++) {
            char* error_str = semantic_errors->errors[i]->text;
            printf("%s\n", error_str);
            free(error_str);
        }
        HulkErrorList_destroy(semantic_errors);
        return 1;
    }
    
    printf("El programa es semánticamente correcto.\n");
    HulkErrorList_destroy(semantic_errors);
    
    return 0;
}