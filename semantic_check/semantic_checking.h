#include <stdio.h>
#include <stdlib.h>
#include "../ast_nodes/ast_nodes.h"
#include "semantic_errors.h"


HulkErrorList* semantic_analysis(ProgramNode* ast);