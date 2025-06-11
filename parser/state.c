#include "state.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

State* create_state(Item** items, int item_count) {
    if (!items || item_count <= 0) {
        fprintf(stderr, "Error: Intento de crear estado con items inválidos\n");
        return NULL;
    }

    static int state_counter = 0;
    State* state = malloc(sizeof(State));
    state->items = malloc(sizeof(Item*) * item_count);
    
    for (int i = 0; i < item_count; i++) {
        if (!items[i]) {
            fprintf(stderr, "Error: Item nulo en creación de estado\n");
            free(state->items);
            free(state);
            return NULL;
        }
        state->items[i] = items[i];
    }

    state->item_count = item_count;
    state->transitions = NULL;
    state->id = state_counter++;
    state->is_final = 0;
    return state;
}

void add_transition(State* from, Symbol* symbol, State* to) {
    // Verificar si la transición ya existe
    Transition* t = from->transitions;
    while (t) {
        if (symbol_equals(t->symbol, symbol) && t->next_state == to) {
            return; // La transición ya existe
        }
        t = t->next;
    }
    
    // Crear nueva transición
    Transition* new_t = (Transition*)malloc(sizeof(Transition));
    new_t->symbol = symbol;
    new_t->next_state = to;
    new_t->next = from->transitions;
    from->transitions = new_t;
}

State* get_transition(State* from, Symbol* symbol) {
    Transition* t = from->transitions;
    while (t) {
        if (strcmp(t->symbol->name, symbol->name) == 0) {
            printf("Transición válida: %s -> estado %d\n", 
                  symbol->name, t->next_state->id);
            return t->next_state;
        }
        t = t->next;
    }
    //printf("ERROR: No hay transición para %s desde estado %d\n", symbol->name, from->id);
    return NULL;  // Devuelve NULL en lugar de fallar
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
        printf("  con el simbolo '%s' -> siguente estado %p\n", t->symbol->name, (void*)t->next_state);
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