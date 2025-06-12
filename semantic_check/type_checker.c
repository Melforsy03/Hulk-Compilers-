#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "type_checker.h"
#include "semantic_errors.h"
#include "semantic.h"
#include "../parser/ast_nodes.h"


// Función principal para realizar el type checking
void type_check_program(ProgramNode* ast, Context* context) {
    TypeChecker* tc = create_type_checker(context);
    Scope* global_scope = create_scope(NULL);
    
    visit_program(tc, ast, global_scope);
    
    // Print errors if any
    for (int i = 0; i < tc->error_count; i++) {
        char* error_str = HulkSemanticError_to_string(tc->errors[i]);
        printf("%s\n", error_str);
        free(error_str);
    }
    
    // Cleanup
    for (int i = 0; i < tc->error_count; i++) {
        HulkError_free((HulkError*)tc->errors[i]);
        free(tc->errors[i]);
    }
    free(tc->errors);
    free(tc);
}

