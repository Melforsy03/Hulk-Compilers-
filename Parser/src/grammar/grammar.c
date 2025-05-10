#include "grammar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Grammar* create_grammar() 
{
    Grammar* g = (Grammar*)malloc(sizeof(Grammar));
    g->symbol_count = 0;
    g->production_count = 0;
    g->start_symbol = NULL;

    g->epsilon = create_symbol("epsilon", EPSILON);
    g->eof = create_symbol("$", EOF_SYM);
    return g;
}

Symbol* add_symbol(Grammar* g, const char* name, SymbolType type) 
{
    for (int i = 0; i < g->symbol_count; ++i) 
        if (strcmp(g->symbols[i]->name, name) == 0) 
            return g->symbols[i];

    Symbol* s = create_symbol(name, type);
    g->symbols[g->symbol_count++] = s;

    if (type == TERMINAL)
        g->terminals[g->terminals_count++] = s;
    else if (type == NON_TERMINAL)
        g->nonterminals[g->nonterminals_count++] = s;

    return s;
}


Symbol* find_symbol(Grammar* g, const char* name) 
{
    for (int i = 0; i < g->symbol_count; ++i){
        printf("g = '%s'\n", g->symbols[i]->name);
        if (strcmp(g->symbols[i]->name, name) == 0) 
            return g->symbols[i];
    }
    return NULL;
}

Symbol* get_terminal(Grammar* grammar, const char* name) 
{
    for (int i = 0; i < grammar->symbol_count; ++i) 
    {
        Symbol* sym = grammar->symbols[i];
        if (sym->type == TERMINAL && strcmp(sym->name, name) == 0) 
            return sym;
    }
    return NULL;
}


void add_production(Grammar* g, Symbol* left, Symbol** right, int right_len) 
{
    g->productions[g->production_count++] = create_production(left, right, right_len);
}

void print_grammar(Grammar* g) 
{
    printf("Símbolo inicial: %s\n", g->start_symbol ? g->start_symbol->name : "(ninguno)\n");
    printf("\nSímbolos (%d):\n", g->symbol_count);
    for (int i = 0; i < g->symbol_count; ++i) 
    {
        print_symbol(g->symbols[i]);
        printf("\n");
    }

    printf("\nProducciones (%d):\n", g->production_count);
    
    for (int i = 0; i < g->production_count; ++i) 
    {
        print_production(g->productions[i]);
    }
}

void free_grammar(Grammar* g)
{
    if (!g) return;

    for (int i = 0; i < g->symbol_count; ++i) 
        free_symbol(g->symbols[i]);
    
    for (int i = 0; i < g->production_count; ++i) 
        free_production(g->productions[i]);

    free_symbol(g->epsilon);
    free_symbol(g->eof);
    free(g);
}
