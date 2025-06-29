#include <stdio.h> 
#include "../grammar/grammar_init.h" 
#include "../grammar/load_grammar.h" 
#include "../parser/containerset.h"
#include "../parser/first_follow.h" 
#include "../parser/parser_table.h" 
#include "../parser/parser_ll1.h" 
#include "../grammar/grammar.h" 
#include "../lexer/lexer.h"
#include "../codigo_generado/generacion_codigo.h"
int main(void) { 
    Grammar* g = init_arithmetic_grammar();
    print_grammar(g);  

    ContainerSet** firsts = compute_firsts(g);
    print_sets(g, firsts, "FIRST");
    ContainerSet** follows = compute_follows(g, firsts);
    print_sets(g, follows, "FOLLOW");

    ParseTable* table = build_parse_table(g, firsts, follows);
    if (!table) {
        fprintf(stderr, "Error al construir tabla de parsing\n");
        return 1;
    }

    print_parse_table(table, g);
    const char* source = "3 + 4 * 2";
    const char* input_ptr = source;

    ParserLL1* parser = parser_ll1_new(&input_ptr, g);
    
    Node* ast = parse_program(parser);

    salida_llvm = fopen("hulk/programa.ll", "w");
    if (!salida_llvm) {
        fprintf(stderr, "No se pudo abrir archivo LLVM para escritura\n");
        return 1;
    }

    generar_programa((ProgramNode*)ast);

    fclose(salida_llvm); 
}
