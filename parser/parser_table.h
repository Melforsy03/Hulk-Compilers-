#ifndef PARSE_TABLE_H
#define PARSE_TABLE_H

#include "../grammar/grammar.h"
#include "parser/first_follow.h"
#include "containerset.h"

typedef struct {
    Production* production;  // Producción a aplicar
} TableEntry;

typedef struct {
    TableEntry*** entries;   // Matriz 2D: [non_terminal][terminal]
    int non_term_count;
    int term_count;
} ParseTable;

ParseTable* build_parse_table(Grammar* g, ContainerSet** firsts, ContainerSet** follows);
void print_parse_table(ParseTable* table, Grammar* g);
void free_parse_table(ParseTable* table);

#endif