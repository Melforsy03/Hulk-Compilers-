#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type_collector.h"
#include "semantic.h" 
#include "semantic_errors.h" 
#include "../ast_nodes/ast_nodes.h"

void collect_types(HulkErrorList* errors, ProgramNode* ast) {
    
    TypeCollector collector;
    TypeCollector_init(&collector, errors);
    TypeCollector_visit_program(&collector, ast);
}
