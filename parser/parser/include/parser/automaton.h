
#ifndef AUTOMATON_H
#define AUTOMATON_H

#include "grammar.h"
#include "state.h"
#include "item.h"

// Solo exponemos las funciones públicas reales
Item** closure(Item** items, int* count, Grammar* grammar);
State* find_existing_state(State** states, int state_count, Item** items, int item_count);
State* build_LR0_automaton(Grammar* grammar);
void print_automaton(State* start);


#endif
