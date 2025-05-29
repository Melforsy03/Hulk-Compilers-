
#ifndef AUTOMATON_H
#define AUTOMATON_H

#include "grammar.h"
#include "state.h"
#include "item.h"

// Solo exponemos las funciones públicas reales
Item** closure(Item** items, int* count, Grammar* grammar, ContainerSet** firsts);
State* find_existing_lr1_state(State** states, int state_count, Item** items, int item_count);

State* build_LR1_automaton(Grammar* grammar, ContainerSet** firsts);
Symbol** get_symbols_after_dot(State* state);

int compare_lr1_items(Item* a, Item* b);
void print_automaton(State* start);


#endif
