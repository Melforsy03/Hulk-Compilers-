#include "containerset.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define INITIAL_CAPACITY 16

ContainerSet* create_containerset() 
{
    ContainerSet* set = (ContainerSet*)malloc(sizeof(ContainerSet));
    set -> symbols = (Symbol**)malloc(sizeof(Symbol*) * INITIAL_CAPACITY);
    set -> size = 0;
    set -> capacity = INITIAL_CAPACITY;
    set -> contains_epsilon = 0;
    return set;
}

int set_contains_symbol(ContainerSet* set, Symbol* sym) 
{
    for (int i = 0; i < set->size; ++i) 
        if (symbol_equals(set->symbols[i], sym)) 
            return 1;
   
    return 0;
}

int add_symbol_to_set(ContainerSet* set, Symbol* sym) 
{
    if (set_contains_symbol(set, sym)) return 0;

    if (set -> size == set -> capacity) 
    {
        set -> capacity *= 2;
        set -> symbols = (Symbol**)realloc(set -> symbols, sizeof(Symbol*) * set -> capacity);
    }

    set -> symbols[set->size++] = sym;
    return 1;
}

void set_epsilon(ContainerSet* set) 
{
    set->contains_epsilon = 1;
}

int containerset_update(ContainerSet* target, ContainerSet* source) 
{
    int changed = 0;
    for (int i = 0; i < source->size; ++i) 
        changed |= add_symbol_to_set(target, source->symbols[i]);

    return changed;
}

int containerset_hard_update(ContainerSet* target, ContainerSet* source) 
{
    int changed = containerset_update(target, source);
    if (source->contains_epsilon && !target->contains_epsilon) 
    {
        set_epsilon(target);
        changed = 1;
    }
    return changed;
}

void print_containerset(ContainerSet* set, const char* label) 
{
    printf("%s: { ", label);
    for (int i = 0; i < set->size; ++i) 
        printf("%s ", set->symbols[i]->name);
    
    if (set->contains_epsilon) printf("ε ");
    printf("}\n");
}

void free_containerset(ContainerSet* set) 
{
    if (set) 
    {
        free(set->symbols);
        free(set);
    }
}
