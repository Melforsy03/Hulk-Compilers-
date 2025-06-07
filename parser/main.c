#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "grammar.h"
#include "first_follow.h"
#include "automaton.h"
#include "parser.h"
#include "lr1_table.h"

int main() {
    printf("=== Cargando Gramática ===\\n");
    Grammar* grammar = create_grammar("archivo.txt");
    load_grammar_from_file(grammar, "gramatica.txt");
    printf("=== Gramática cargada ===\\n");

    // Buscar el símbolo original 'Program'
    Symbol* program_sym = NULL;
    for (int i = 0; i < grammar->symbol_count; ++i) {
        if (strcmp(grammar->symbols[i]->name, "Program") == 0 &&
            grammar->symbols[i]->type == NON_TERMINAL) {
            program_sym = grammar->symbols[i];
            break;
        }
    }

    if (program_sym == NULL) {
        fprintf(stderr, "Error: No se encontró el símbolo 'Program' en la gramática.\\n");
        exit(1);
    }

    // Crear símbolo S' y agregarlo al conjunto de símbolos
    Symbol* start_prime = create_symbol("S'", NON_TERMINAL);
    grammar->symbols[grammar->symbol_count++] = start_prime;

    // Crear producción S' → Program
    Symbol** right = malloc(sizeof(Symbol*));
    right[0] = program_sym;

    Production* augmented_prod = create_production(start_prime, right, 1);
    augmented_prod->number = 0;
    grammar->productions[grammar->production_count++] = augmented_prod;

    // Establecer símbolo inicial para el autómata
    grammar->start_symbol = start_prime;

    if (!grammar->eof) {
        grammar->eof = create_symbol("$", EOF_SYM);
        add_symbol(grammar, "$", EOF_SYM);  // Añadirlo al conjunto de símbolos
    }

    // Calcular FIRST y FOLLOW
    printf("\\n=== Calculando FIRST y FOLLOW === \n");
    ContainerSet** firsts = compute_firsts(grammar);
    print_sets(grammar, firsts, "FIRST");


    ContainerSet** follows = compute_follows(grammar, firsts);
    print_sets(grammar, follows,"FOLLOW");

    // Construir autómata LR(0)
    printf("\\n=== Construyendo Autómata LR(1) === \n");
    State* start_state = build_LR1_automaton(grammar, firsts);
    print_automaton(start_state);

    // Construir tabla LR(1)
    printf("\\n=== Construyendo Tabla LR(1) ===\\n");
    LR1Table* lr_table = build_lr1_table(start_state, grammar);
    print_lr1_table(lr_table);

    // === Simulación manual de símbolos (tokens) ===
    Symbol* num1 = get_terminal(grammar, "string");
    Symbol* num2 = get_terminal(grammar, ">");
    Symbol* num4 = get_terminal(grammar, "-");
    Symbol* num5 = get_terminal(grammar, "num");
    Symbol* semicolon = get_terminal(grammar, ";");
    Symbol* dollar = get_terminal(grammar, "$");

    Symbol* input_symbols[] = {
       num1, num2, num4, num5, semicolon, dollar
    };
    int token_count = sizeof(input_symbols) / sizeof(Symbol*);

    printf("\n=== Analizando cadena ===\n");

    ActionEntryLR1* actions = NULL;
    int action_count = 0;

    int accepted = parser(lr_table, input_symbols, token_count, &actions, &action_count);

    if (accepted) {
        printf("\n=== Cadena ACEPTADA ===\n");
    } 
    else {
        printf("\n=== Cadena RECHAZADA ===\n");
    }

    // Limpieza de memoria
    free(actions);
    free_sets(firsts, grammar->symbol_count);
    free_sets(follows, grammar->symbol_count);
    free_grammar(grammar);
    free_lr1_table(lr_table);
    
    // Finalizar
    return 0;
}