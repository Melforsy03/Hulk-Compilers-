#ifndef PARSER_H
#define PARSER_H

#include "lr1_table.h"
#include "ast_nodes/ast_optimize.h"
#include "grammar/grammar.h"
#include "ast_nodes/ast_build.h"
#include "ast_nodes/ast_nodes.h"

typedef struct {
    Node* node;
    Symbol* symbol;
    int is_node; // 1=node, 0=symbol
} SemanticEntry;

typedef struct {
    SemanticEntry* items;
    int top;
    int capacity;
} SemanticStack;


Node* parser(LR1Table* table, Symbol** input_tokens, int token_count, ActionEntryLR1** actions, int* action_count);

#endif
