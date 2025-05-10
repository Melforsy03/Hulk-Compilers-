#ifndef SYMBOL_H
#define SYMBOL_H

typedef enum 
{
    TERMINAL,
    NON_TERMINAL,
    EPSILON,
    EOF_SYM
} SymbolType;

typedef struct Symbol 
{
    char* name;
    SymbolType type;
} Symbol;

// Crea un símbolo nuevo
Symbol* create_symbol(const char* name, SymbolType type);

// Compara dos símbolos por nombre
int symbol_equals(Symbol* a, Symbol* b);

// Imprime un símbolo
void print_symbol(Symbol* symbol);

void free_symbol(Symbol* s);

#endif
