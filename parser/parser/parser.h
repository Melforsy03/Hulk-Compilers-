#ifndef PARSER_H
#define PARSER_H

#include "lr1_table.h"
#include "grammar.h"
#include "ast_nodes.h"

ASTNode* parser(LR1Table* table, Symbol** input_tokens, int token_count, ActionEntryLR1** actions, int* action_count);
// Función principal para parsear una entrada
// Devuelve 1 si acepta, 0 si rechaza
int parser(LR1Table* table, Symbol** input_tokens, int token_count, ActionEntryLR1** actions, int* action_count);
#endif
