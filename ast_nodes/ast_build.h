#include "parser/parser.h"
#include "../grammar/symbol.h"
#include "ast_nodes/ast_nodes.h" 
#include "ast_nodes/ast_builder.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

NodeType get_operator_node_type(const char* op_name);
Node* build_binary_operation(Production* p, Node** children, const char* operator_name);
Node* build_unary_operation(Production* p, Node** children, const char* operator_name);