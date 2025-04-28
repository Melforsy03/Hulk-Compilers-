#ifndef PRODUCTION_H
#define PRODUCTION_H

#include "symbol.h"

typedef struct Production 
{
    Symbol* left;
    Symbol** right;
    int right_len;
} Production;

// Crea una producción nueva (left -> right)
Production* create_production(Symbol* left, Symbol** right, int right_len);

// Imprime una producción
void print_production(Production* p);

#endif
