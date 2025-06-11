#ifndef LR1_TABLE_H
#define LR1_TABLE_H

#include "state.h"
#include "../grammar/grammar.h"
#include "containerset.h"

// Tipos de acciones en la tabla
typedef enum 
{
    ACTION_ERROR,
    ACTION_SHIFT,
    ACTION_REDUCE,
    ACTION_ACCEPT
} ActionTypeLR1;

// Entrada de la tabla ACTION
typedef struct 
{
    ActionTypeLR1 action;
    int value;  // Estado destino o número de producción
} ActionEntryLR1;

// Estructura general de la tabla LR(1)
typedef struct 
{
    ActionEntryLR1** action;  // [estado][terminal]
    int** goto_table;         // [estado][no_terminal]
    int state_count;
    int terminal_count;
    int nonterminal_count;
    Grammar* grammar;
} LR1Table;

// Construir la tabla LR(1)
LR1Table* build_lr1_table(State* start, Grammar* grammar);

void collect_states_lr1(State* start, State** states, int* state_count);

int index_of_symbol(Symbol** list, int count, Symbol* s);

int is_operator(Symbol* sym);

// Imprimir la tabla
void print_lr1_table(LR1Table* table);

// Liberar memoria de la tabla
void free_lr1_table(LR1Table* table);

#endif
