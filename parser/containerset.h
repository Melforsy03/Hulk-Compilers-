#ifndef CONTAINERSET_H
#define CONTAINERSET_H

#include "../grammar/symbol.h"

typedef struct ContainerSet 
{
    Symbol** symbols;
    int size;
    int capacity;
    int contains_epsilon;
} ContainerSet;

// Crear un nuevo conjunto vacío
ContainerSet* create_containerset();

// Agregar símbolo (evita duplicados)
int add_symbol_to_set(ContainerSet* set, Symbol* sym);

// Verifica si contiene un símbolo
int set_contains_symbol(const ContainerSet* set, Symbol* sym);

// Unir otro conjunto
int containerset_update(ContainerSet* target, ContainerSet* source);

// Unir y actualizar epsilon
int containerset_hard_update(ContainerSet* target, ContainerSet* source);

ContainerSet* copy_containerset(const ContainerSet* original);

int containerset_equals(const ContainerSet* a, const ContainerSet* b);

// Establecer epsilon
void set_epsilon(ContainerSet* set);

// Liberar memoria
void free_containerset(ContainerSet* set);

// Mostrar conjunto
void print_containerset(ContainerSet* set, const char* label);

#endif
