#ifndef AUTOMATON_H
#define AUTOMATON_H

#include "grammar.h"
#include "state.h"

// Construye el autómata LR(0) a partir de una gramática
State* build_LR0_automaton(Grammar* grammar);

// Función auxiliar para imprimir todos los estados del autómata
void print_automaton(State* start);

#endif
