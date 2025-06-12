#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "type_checker.h"
#include "semantic_errors.h"
#include "semantic.h"
#include "../parser/ast_nodes.h"


// Función principal para realizar el type checking
void type_check_program(Node* ast, Context* context, HulkErrorList* output_errors) {
    TypeChecker* tc = create_type_checker(context);
    Scope* global_scope = create_scope(NULL);
    
    output_errors = HulkErrorList_create();
    tc->errors = output_errors;
    tc->errors->count = 0;
    
    visit_program(tc, ast, global_scope);
    
}

