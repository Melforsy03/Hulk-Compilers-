#ifndef STATE_H
#define STATE_H

#include "item.h"
#include "symbol.h"

typedef struct State State;
typedef struct Transition Transition;

// Definición de una transición
struct Transition 
{
    Symbol* symbol;     // Símbolo de la transición
    State* next_state;  // Estado al que se llega
    Transition* next;   // Transiciones siguientes (lista enlazada)
};

// Definición del struct State
struct State {
    int id;  
    Item** items;
    int item_count;
    Transition* transitions;
    int is_final;
};
// Crear un nuevo estado con un conjunto de items
State* create_state(Item** items, int item_count);

// Agregar una transición al estado
void add_transition(State* from, Symbol* symbol, State* to);

// Buscar la transición por un símbolo
State* get_transition(State* from, Symbol* symbol);

// Imprimir el estado (items + transiciones)
void print_state(State* state);

// Liberar memoria de un estado (y sus items y transiciones)
void free_state(State* state);

#endif
