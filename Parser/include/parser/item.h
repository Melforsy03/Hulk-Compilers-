#ifndef ITEM_H
#define ITEM_H

#include "production.h"

// Definición del struct Item
typedef struct 
{
    Production* production; // Producción A → αβ
    int pos;                // Posición del punto (cuántos símbolos ya hemos "leído")
} Item;

// Crear un nuevo item
Item* create_item(Production* production, int pos);

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

#endif
