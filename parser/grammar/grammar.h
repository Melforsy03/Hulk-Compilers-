#ifndef GRAMMAR_H
#define GRAMMAR_H

#include "symbol.h"
#include "production.h"

#define MAX_SYMBOLS 300
#define MAX_PRODUCTIONS 1050

typedef struct {
    Symbol* symbols[MAX_SYMBOLS];
    Symbol* terminals[MAX_SYMBOLS];
    Symbol* nonterminals[MAX_SYMBOLS];
    Production* productions[MAX_PRODUCTIONS];

    int symbol_count;
    int terminals_count;
    int nonterminals_count;
    int production_count;
    
    Symbol* start_symbol;
    Symbol* epsilon;
    Symbol* eof;
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
