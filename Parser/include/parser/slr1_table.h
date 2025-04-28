#ifndef SLR1_TABLE_H
#define SLR1_TABLE_H

#include "state.h"
#include "grammar.h"
#include "containerset.h"

// Tipos de acciones en la tabla
typedef enum 
{
    ACTION_ERROR,
    ACTION_SHIFT,
    ACTION_REDUCE,
    ACTION_ACCEPT
} ActionTypeSLR;

// Entrada de la tabla ACTION
typedef struct 
{
    ActionTypeSLR action;
    int value;  // Estado destino o número de producción
} ActionEntrySLR;

// Estructura general de la tabla SLR(1)
typedef struct 
{
    ActionEntrySLR** action;  // [estado][terminal]
    int** goto_table;         // [estado][no_terminal]
    int state_count;
    int terminal_count;
    int nonterminal_count;
    Grammar* grammar;
} SLR1Table;

// Construir la tabla SLR(1)
SLR1Table* build_slr1_table(State* start, Grammar* grammar, ContainerSet** follows);

// Imprimir la tabla
void print_slr1_table(SLR1Table* table);

// Liberar memoria de la tabla
void free_slr1_table(SLR1Table* table);

#endif
