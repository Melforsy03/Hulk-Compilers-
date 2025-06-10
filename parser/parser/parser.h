#ifndef PARSER_H
#define PARSER_H

#include "lr1_table.h"
#include "grammar/grammar.h"

Node* parser(LR1Table* table, Symbol** input_tokens, int token_count, ActionEntryLR1** actions, int* action_count);

#endif
