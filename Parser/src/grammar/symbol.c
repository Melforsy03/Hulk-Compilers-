#include "symbol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Symbol* create_symbol(const char* name, SymbolType type) 
{
    Symbol* s = (Symbol*)malloc(sizeof(Symbol));
    s -> name = strdup(name);
    s -> type = type;
    return s;
}

int symbol_equals(Symbol* a, Symbol* b) 
{
    return strcmp(a->name, b->name) == 0;
}

void print_symbol(Symbol* symbol) 
{
    const char* type_str[] = { "TERMINAL", "NON_TERMINAL", "EPSILON", "EOF" };
    printf("%s (%s)", symbol->name, type_str[symbol->type]);
}

void free_symbol(Symbol* s) 
{
    if (s) 
    {
        free(s->name);
        free(s);
    }
}