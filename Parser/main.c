#include "grammar.h"
#include "first_follow.h"
#include "automaton.h"
#include "slr1_table.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("=== Cargando Gramática ===\n");
    Grammar* grammar = create_grammar();
    printf("=== Cargo Gramática ===\n");
    load_grammar_from_file(grammar, "producciones.txt");
    print_grammar(grammar);

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
    // Por ejemplo: num + num * num $

    Symbol* num = get_terminal(grammar, "num");
    Symbol* plus = get_terminal(grammar, "+");
    Symbol* star = get_terminal(grammar, "*");
    Symbol* dollar = get_terminal(grammar, "$");

    Symbol* input_symbols[] = {num, plus, num, star, num, dollar};
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