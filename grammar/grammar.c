#include "grammar.h"
#include "lexer/lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Grammar* create_grammar() {
    Grammar* g = malloc(sizeof(Grammar));
    if (!g) return NULL;

    // Inicializar todos los campos
    g->symbol_count = 0;
    g->production_count = 0;
    g->terminals_count = 0;
    g->nonterminals_count = 0;
    g->start_symbol = NULL;

    // Inicializar arrays
    for (int i = 0; i < MAX_SYMBOLS; i++) {
        g->symbols[i] = NULL;
        g->terminals[i] = NULL;
        g->nonterminals[i] = NULL;
    }
    
    for (int i = 0; i < MAX_PRODUCTIONS; i++) {
        g->productions[i] = NULL;
    }

    // Símbolos especiales
    g->epsilon = create_symbol("epsilon", EPSILON);
    g->eof = create_symbol("EOF", EOF_SYM);
    
    if (!g->epsilon || !g->eof) {
        free(g);
        return NULL;
    }

    return g;
}

Symbol* add_symbol(Grammar* g, const char* name, SymbolType type) 
{
    if (g->symbol_count >= MAX_SYMBOLS) {
    fprintf(stderr, "Error: Máximo número de símbolos alcanzado.\n");
    return NULL;
    }

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

Symbol* find_symbol(Grammar* g, const char* name) {
    if (!g || !name) return NULL;

    for (int i = 0; i < g->symbol_count; ++i) {
        if (g->symbols[i] && strcmp(g->symbols[i]->name, name) == 0) {
            return g->symbols[i];
        }
    }
    printf("Falta el simbolo en los no terminales o los terminales de la gramatia");
    return NULL;
}

Symbol* get_terminal(Grammar* grammar, const char* name, int row, int colum) 
{
    for (int i = 0; i < grammar->symbol_count; ++i) 
    {
        Symbol* sym = grammar->symbols[i];
        if (sym->type == TERMINAL && strcmp(sym->name, name) == 0) 
        {
            sym->row = row;
            sym->colum = colum;
            return sym;
        }       
    }
    printf("No se a encontrado el simbolo %s en la gramatica", name);
    return NULL;
}


void add_production(Grammar* g, Symbol* left, Symbol** right, int right_len) 
{
    g->productions[g->production_count++] = create_production(left, right, right_len);
}


int get_non_term_index(Grammar* g, Symbol* nt) {
    if (!g || !nt || nt->type != NON_TERMINAL) return -1;
    
    for (int i = 0; i < g->nonterminals_count; i++) {
        if (g->nonterminals[i] && symbol_equals(g->nonterminals[i], nt)) {
            return i;
        }
    }
    return -1;
}

int get_term_index(Grammar* g, Symbol* term) {
    if (!g || !term) return -1;
    if (term->type != TERMINAL) return -1;
    // Buscar entre los terminales normales
    for (int i = 0; i < g->terminals_count; i++) {
        if (strcmp(g->terminals[i]->name, term->name) == 0) {
            return i;
        }
    }
    
    if (term->type == TOKEN_EOF || strcmp(term->name, "$") == 0) {
        return g->terminals_count;
    }
    
    return -1; // No encontrado
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

void free_grammar(Grammar* g) {
    if (!g) return;

    // Liberar símbolos (pero no el array symbols[])
    for (int i = 0; i < g->symbol_count; ++i) {
        free_symbol(g->symbols[i]);
    }

    // Liberar producciones (pero no el array productions[])
    for (int i = 0; i < g->production_count; ++i) {
        free_production(g->productions[i]);
    }

    free_symbol(g->epsilon);
    free_symbol(g->eof);
    free(g);  // Liberar la estructura Grammar
}
