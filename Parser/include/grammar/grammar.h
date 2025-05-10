#ifndef GRAMMAR_H
#define GRAMMAR_H

#include "symbol.h"
#include "production.h"

#define MAX_SYMBOLS 300
#define MAX_PRODUCTIONS 1050

typedef struct Grammar 
{
    Symbol* symbols[MAX_SYMBOLS];
    int symbol_count;

    Symbol* terminals[MAX_SYMBOLS];
    int terminals_count;

    Symbol* nonterminals[MAX_SYMBOLS];
    int nonterminals_count;

    Production* productions[MAX_PRODUCTIONS];
    int production_count;

    Symbol* start_symbol;
    Symbol* epsilon;  // símbolo epsilon
    Symbol* eof;      // símbolo fin de archivo $
} Grammar;

// Crear una gramática vacía
Grammar* create_grammar();

// Agregar un símbolo
Symbol* add_symbol(Grammar* grammar, const char* name, SymbolType type);

// Buscar un símbolo por nombre
Symbol* find_symbol(Grammar* grammar, const char* name);

// Agregar una producción
void add_production(Grammar* grammar, Symbol* left, Symbol** right, int right_len);

// Imprimir gramática
void print_grammar(Grammar* grammar);

Symbol* get_terminal(Grammar* grammar, const char* name);

// Cargar gramática desde archivo
void load_grammar_from_file(Grammar* grammar, const char* filename);

void free_grammar(Grammar* g);

#endif
