#include <stdio.h>
#include "grammar.h"
#include "containerset.h"
#include "first_follow.h"
#include "automaton.h"
#include "slr1_table.h"
#include "leer_gramatica.h"

int main() {
    printf("Cargando gramática...\n");

    // 1. Crear la gramática
    Grammar* grammar = create_grammar();
    load_grammar_from_file(grammar, "producciones.txt");

    printf("\nGramática cargada correctamente.\n");
    print_grammar(grammar);

    // 2. Calcular FIRST y FOLLOW
    printf("\nCalculando conjuntos FIRST...\n");
    ContainerSet** firsts = compute_firsts(grammar);
    print_sets(grammar, firsts, "Conjuntos FIRST");

    printf("\nCalculando conjuntos FOLLOW...\n");
    ContainerSet** follows = compute_follows(grammar, firsts);
    print_sets(grammar, follows, "Conjuntos FOLLOW");

    // 3. Construir el autómata LR(0)
    printf("\nConstruyendo autómata LR(0)...\n");
    State* start = build_LR0_automaton(grammar);
    print_automaton(start);

    // 4. Construir la tabla SLR(1)
    printf("\nConstruyendo tabla SLR(1)...\n");
    SLR1Table* slr_table = build_slr1_table(start, grammar, follows);
    print_slr1_table(slr_table);

    // 5. Liberar memoria 
    free_sets(firsts, grammar->symbol_count);
    free_sets(follows, grammar->symbol_count);
    free_grammar(grammar);
    free_slr1_table(slr_table);
    
    printf("\nPrograma terminado correctamente.\n");
    return 0;
}
