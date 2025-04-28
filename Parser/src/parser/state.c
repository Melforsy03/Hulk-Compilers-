#include "state.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

State* create_state(Item** items, int item_count) 
{
    State* state = (State*)malloc(sizeof(State));
    state->items = (Item**)malloc(sizeof(Item*) * item_count);
    for (int i = 0; i < item_count; ++i) 
        state->items[i] = items[i];
    
    state->item_count = item_count;
    state->transitions = NULL;
    state->is_final = 0;
    return state;
}

void add_transition(State* from, Symbol* symbol, State* to) 
{
    Transition* t = (Transition*)malloc(sizeof(Transition));
    t->symbol = symbol;
    t->next_state = to;
    t->next = from->transitions;
    from->transitions = t;
}

State* get_transition(State* from, Symbol* symbol) 
{
    Transition* t = from->transitions;
    while (t) 
    {
        if (strcmp(t->symbol->name, symbol->name) == 0) 
            return t->next_state;
        t = t->next;
    }
    return NULL;
}

void print_state(State* state) 
{
    printf("State:\n");
    for (int i = 0; i < state->item_count; ++i) 
        print_item(state->items[i]);

    printf("Transitions:\n");
    Transition* t = state->transitions;
    while (t) 
    {
        printf("  with symbol '%s' -> next state %p\n", t->symbol->name, (void*)t->next_state);
        t = t->next;
    }
    printf("\n");
}

void free_state(State* state) 
{
    if (state) 
    {
        for (int i = 0; i < state->item_count; ++i) 
            free_item(state->items[i]);
        
        free(state->items);

        Transition* t = state->transitions;
        while (t) 
        {
            Transition* next = t->next;
            free(t);
            t = next;
        }
        
        free(state);
    }
}