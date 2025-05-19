#include "grammar.h"
#include "first_follow.h"
#include "automaton.h"
#include "slr1_table.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>

int main() 
{
    printf("=== Cargando Gramática ===\n");
    Grammar* grammar = create_grammar();
    printf("=== Cargo Gramática ===\n");
    load_grammar_from_file(grammar, "producciones.txt");

    // Después de cargar producciones
    Symbol* augmented = create_symbol("S'", NON_TERMINAL);
    grammar->nonterminals[grammar->nonterminals_count++] = augmented;

    Symbol** right = malloc(sizeof(Symbol*));
    right[0] = grammar->start_symbol;

    Production* augmented_prod = create_production(augmented, right, 1);
    augmented_prod->number = -1;

    augmented_prod->right[0] = grammar->start_symbol;
    augmented_prod->right_len = 1;

    grammar->productions[grammar->production_count++] = augmented_prod;

    printf("\n=== Calculando FIRST y FOLLOW ===\n");
    ContainerSet** firsts = compute_firsts(grammar);
    print_sets(grammar, firsts, "FIRST");

    ContainerSet** follows = compute_follows(grammar, firsts);
    print_sets(grammar, follows, "FOLLOW");

    printf("\n=== Construyendo Autómata LR(0) ===\n");
    State* start = build_LR0_automaton(grammar);
    print_automaton(start);

    printf("\n=== Construyendo Tabla SLR(1) ===\n");
    SLR1Table* slr_table = build_slr1_table(start, grammar, follows);
    print_slr1_table(slr_table);

    // === Simulación manual de símbolos (tokens) ===
    // Por ejemplo: id + id * id $

    Symbol* function_kw = get_terminal(grammar, "function");
    Symbol* id = get_terminal(grammar, "id");
    Symbol* lparen = get_terminal(grammar, "(");
    Symbol* rparen = get_terminal(grammar, ")");
    Symbol* arrow = get_terminal(grammar, "=>");
    Symbol* semicolon = get_terminal(grammar, ";");
    Symbol* dollar = get_terminal(grammar,"$");

    Symbol* input_symbols[] = {
        function_kw, id, lparen, rparen, arrow, id, semicolon, dollar
    };
    int token_count = sizeof(input_symbols) / sizeof(Symbol*);

    printf("\n=== Analizando cadena ===\n");

    ActionEntrySLR* actions = NULL;
    int action_count = 0;

    int accepted = parse(slr_table, input_symbols, token_count, &actions, &action_count);

    if (accepted) {
        printf("\n=== Cadena ACEPTADA ===\n");
    } else {
        printf("\n=== Cadena RECHAZADA ===\n");
    }

    // Limpieza de memoria
    free(actions);
    free_sets(firsts, grammar->symbol_count);
    free_sets(follows, grammar->symbol_count);
    free_grammar(grammar);
    free_slr1_table(slr_table);

    return 0;
}