#ifndef PARSER_H
#define PARSER_H

#include "slr1_table.h"
#include "grammar.h"

// Función principal para parsear una entrada
// Devuelve 1 si acepta, 0 si rechaza
int parse(SLR1Table* table, Symbol** input_tokens, int token_count);

#endif
