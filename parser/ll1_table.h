#ifndef LL1_TABLE_H
#define LL1_TABLE_H

#include "grammar.h"
#include "first_follow.h" // Se necesita para FirstFollowTable y funciones relacionadas

#define MAX_CELLS 1048 // Ajusta este valor si tu tabla LL(1) es más grande

typedef struct {
    char non_terminal[MAX_SYMBOL_LEN];  // A (non-terminal)
    char terminal[MAX_SYMBOL_LEN];      // t (terminal)
    Production* production;           // P (production A ::= alpha)
} LL1TableEntry;

typedef struct {
    LL1TableEntry entries[MAX_CELLS];
    int num_entries;
} LL1Table;

void init_ll1_table(LL1Table* table);
void generate_ll1_table(const Grammar* g, const FirstFollowTable* ff_table, LL1Table* table);
void print_ll1_table(const LL1Table* table);
void infer_produces_ast(Grammar* g, FirstFollowTable* ff_table);
#endif