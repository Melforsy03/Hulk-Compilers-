#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type_collector.h"
#include "semantic.h" 
#include "semantic_errors.h" 
#include "../parser/ast_nodes.h"

TypeCollector collect_types(HulkErrorList* errors, Node* ast) {
    
    TypeCollector collector;
    TypeCollector_init(&collector, errors);
    TypeCollector_visit_program(&collector, ast);

    return collector;
}
