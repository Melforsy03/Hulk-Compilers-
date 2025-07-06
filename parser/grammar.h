#ifndef GRAMMAR_H
#define GRAMMAR_H

#define MAX_SYMBOL_LEN 50
#define MAX_TERMINALS 100
#define MAX_NON_TERMINALS 100
#define MAX_PRODUCTIONS 200
#define MAX_RHS 20
#define MAX_RHS_SYMBOLS 20
typedef struct {
   char lhs[MAX_SYMBOL_LEN];
   char rhs[MAX_RHS][MAX_SYMBOL_LEN];
  int rhs_len;
  int produces_ast[MAX_RHS];
} Production;


typedef struct {
    char start_symbol[MAX_SYMBOL_LEN];
    char terminals[MAX_TERMINALS][MAX_SYMBOL_LEN];
    char non_terminals[MAX_NON_TERMINALS][MAX_SYMBOL_LEN];
    Production productions[MAX_PRODUCTIONS];
    int num_terminals;
    int num_non_terminals;
    int num_productions;
} Grammar;

void init_grammar(Grammar* g);
void add_terminal(Grammar* g, const char* symbol);
void add_non_terminal(Grammar* g, const char* symbol);
void add_production(Grammar* g, const char* lhs, const char* rhs_seq[], int rhs_len);
void print_grammar(const Grammar* g);
void load_grammar_from_file(Grammar* g, const char* filename);

#endif