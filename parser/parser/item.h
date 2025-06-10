#ifndef ITEM_H
#define ITEM_H

#include "grammar/production.h"
#include "containerset.h"

typedef struct Item {
    Production* production;
    int pos;
    ContainerSet* lookaheads;  
} Item;


// Crear un nuevo item
Item* create_item(Production* production, int pos, ContainerSet* lookaheads);

// Avanzar el punto en un item
Item* next_item(Item* item);

// Verificar si un item es de tipo "reduce" (el punto está al final)
int is_reduce_item(Item* item);

// Comparar dos items (para saber si son iguales)
int compare_items(Item* a, Item* b);

// Imprimir un item
void print_item(Item* item);

// Liberar memoria de un item
void free_item(Item* item);
Symbol* get_next_symbol(Item* item);



#endif
