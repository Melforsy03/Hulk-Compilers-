#include "containerset.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define INITIAL_CAPACITY 16

ContainerSet* create_containerset() 
{
    ContainerSet* set = (ContainerSet*)malloc(sizeof(ContainerSet));
    if (!set) return NULL;
    
    set->symbols = (Symbol**)malloc(sizeof(Symbol*) * INITIAL_CAPACITY);
    if (!set->symbols) {
        free(set);
        return NULL;
    }
    
    // Inicialización explícita de memoria
    memset(set->symbols, 0, sizeof(Symbol*) * INITIAL_CAPACITY);
    set->size = 0;
    set->capacity = INITIAL_CAPACITY;
    set->contains_epsilon = 0;
    return set;
}

int set_contains_symbol(const ContainerSet* set, Symbol* sym)
{
    if (!set || !sym || !set->symbols) return 0;

    for (int i = 0; i < set->size; ++i) {
        if (set->symbols[i] && symbol_equals(set->symbols[i], sym)) {
            return 1;
        }
    }
    return 0;
}

ContainerSet* copy_containerset(const ContainerSet* original) {
    if (!original || !original->symbols) return NULL;
    
    ContainerSet* copy = create_containerset();
    if (!copy) return NULL;

    for (int i = 0; i < original->size; i++) {
        if (original->symbols[i]) {
            if (!add_symbol_to_set(copy, original->symbols[i])) {
                free_containerset(copy);
                return NULL;
            }
        }
    }
    copy->contains_epsilon = original->contains_epsilon;
    return copy;
}

int containerset_equals(const ContainerSet* a, const ContainerSet* b) {
    if (!a || !b || !a->symbols || !b->symbols) return 0;
    if (a->size != b->size) return 0;
    if (a->contains_epsilon != b->contains_epsilon) return 0;
    
    for (int i = 0; i < a->size; i++) {
        if (a->symbols[i] && !set_contains_symbol(b, a->symbols[i])) {
            return 0;
        }
    }
    return 1;
}

int add_symbol_to_set(ContainerSet* set, Symbol* sym) 
{
    if (!set || !sym || !set->symbols) {
        fprintf(stderr, "Error: Parámetros inválidos en add_symbol_to_set\n");
        return 0;
    }

    if (set_contains_symbol(set, sym)) return 0;

    if (set->size == set->capacity) {
        size_t new_capacity = set->capacity * 2;
        Symbol** new_symbols = (Symbol**)realloc(set->symbols, sizeof(Symbol*) * new_capacity);
        if (!new_symbols) {
            fprintf(stderr, "Error: No se pudo expandir el ContainerSet\n");
            return 0;
        }
        set->symbols = new_symbols;
        set->capacity = new_capacity;
    }

    set->symbols[set->size++] = sym;
    return 1;
}

void set_epsilon(ContainerSet* set) 
{
    if (set) {
        set->contains_epsilon = 1;
    }
}

int containerset_update(ContainerSet* target, ContainerSet* source) 
{
    if (!target || !source || !target->symbols || !source->symbols) {
        fprintf(stderr, "Error: Conjuntos no inicializados en containerset_update\n");
        return 0;
    }

    int changed = 0;
    for (int i = 0; i < source->size; ++i) {
        if (source->symbols[i]) {
            changed |= add_symbol_to_set(target, source->symbols[i]);
        }
    }
    return changed;
}

int containerset_hard_update(ContainerSet* target, ContainerSet* source) 
{
    if (!target || !source) return 0;
    
    int changed = containerset_update(target, source);
    if (source->contains_epsilon && !target->contains_epsilon) {
        set_epsilon(target);
        changed = 1;
    }
    return changed;
}

void print_containerset(ContainerSet* set, const char* label) 
{
    if (!set || !label) return;
    
    printf("%s: { ", label);
    if (set->symbols) {
        for (int i = 0; i < set->size; ++i) {
            if (set->symbols[i]) {
                printf("%s ", set->symbols[i]->name);
            }
        }
    }
    
    if (set->contains_epsilon) printf("epsilon ");
    printf("}\n");
}

void free_containerset(ContainerSet* set) 
{
    if (set) {
        if (set->symbols) {
            free(set->symbols);
        }
        free(set);
    }
}