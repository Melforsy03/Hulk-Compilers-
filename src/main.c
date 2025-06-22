#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../grammar/grammar.h"
#include "../grammar/load_grammar.h"
#include "../parser/first_follow.h"
#include "../parser/automaton.h"
#include "../parser/parser.h"
#include "../parser/lr1_table.h"
#include "../ast_nodes/ast_nodes.h"
#include "../lexer/lexer.h"
#include "../lexer/func_aux_lexer.h"
#include "../codigo_generado/generacion_codigo.h"
#include "../semantic_check/semantic_checking.h"
#include "../semantic_check/semantic.h"
#include "../semantic_check/semantic_errors.h"

Symbol** lexer_parse_file_to_symbols(const char* filename, Grammar* grammar, int* out_count);
int main() {
    Grammar* grammar = create_grammar("archivo.txt");
    load_grammar_from_file(grammar, "gramatica.txt");

    Symbol* program_sym = NULL;
    // Buscar símbolo inicial
    for (int i = 0; i < grammar->symbol_count; ++i) {
        if (strcmp(grammar->symbols[i]->name, "Program") == 0 &&
            grammar->symbols[i]->type == NON_TERMINAL) {
            program_sym = grammar->symbols[i];
            break;
        }
    }

    Symbol* start_prime = create_symbol("S'", NON_TERMINAL);
    grammar->symbols[grammar->symbol_count++] = start_prime;

    Symbol** right = malloc(sizeof(Symbol*));
    right[0] = program_sym;
    Production* augmented = create_production(start_prime, right, 1);
    augmented->number = 0;
    grammar->productions[grammar->production_count++] = augmented;
    grammar->start_symbol = start_prime;

    if (!grammar->eof) {
        grammar->eof = create_symbol("$", EOF_SYM);
        add_symbol(grammar, "$", EOF_SYM);
    }

    // FIRST/FOLLOW, autómata y tabla
    ContainerSet** firsts = compute_firsts(grammar);
    ContainerSet** follows = compute_follows(grammar, firsts);
    State* start_state = build_LR1_automaton(grammar, firsts);
    LR1Table* table = build_lr1_table(start_state, grammar);

    // ⬇️ Lexer transforma archivo en lista de símbolos
    int input_len = 0;
    Symbol** input_symbols = lexer_parse_file_to_symbols("entrada.txt", grammar, &input_len);

    // Parser
    ActionEntryLR1* actions = NULL;
    int action_count = 0;
    Node* accepted = parser(table, input_symbols, input_len, &actions, &action_count);
    
    if (!accepted) {
            fprintf(stderr, "[ERROR] Fallo el parseo (accepted == NULL)\n");
            return 1;
        }
        if (accepted->tipo != NODE_PROGRAM) {
            fprintf(stderr, "[ERROR] Nodo raíz no es de tipo ProgramNode (tipo = %d)\n", accepted->tipo);
            return 1;
        }

    HulkErrorList* semanticErrors = semantic_analysis((ProgramNode*)accepted);
    if(semanticErrors->count > 0){
        fprintf(stderr, "[ERROR] Se encontraron errores en el codigo en el chequeo de tipos \n");
    }
    
    salida_llvm = fopen("hulk/programa.ll", "w");
    if (!salida_llvm) {
        fprintf(stderr, "[ERROR] No se pudo abrir el archivo de salida 'programa.ll'\n");
        return 1;
    }
    generar_programa((ProgramNode*)accepted);

    fclose(salida_llvm);
    // Limpieza
    free(actions);
    free(input_symbols);
    free_sets(firsts, grammar->symbol_count);
    free_sets(follows, grammar->symbol_count);
    free_grammar(grammar);
    free_lr1_table(table);
    return 0;
}

Symbol** lexer_parse_file_to_symbols(const char* filename, Grammar* grammar, int* out_count) {
    // Leer archivo fuente
    FILE* f = fopen(filename, "rb");
    if (!f) {
        perror("Error al abrir archivo de entrada");
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);

    char* buffer = malloc(len + 1);
    fread(buffer, 1, len, f);
    buffer[len] = '\0';
    fclose(f);

    int token_count = 0;
    Token* tokens = next_tokens(buffer, &token_count);

    Symbol** symbols = malloc(sizeof(Symbol*) * (token_count + 2));
    int count = 0;

    for (int i = 0; i < token_count; i++) {
        if (tokens[i].type == TOKEN_WHITESPACE) {
            free((char*)tokens[i].lexema);
            continue;
        }

        if (tokens[i].type == TOKEN_ERROR) {
            fprintf(stderr, "Error léxico: '%.*s'\n", tokens[i].length, tokens[i].lexema);
            exit(1);
        }

        const char* name = token_type_to_string(tokens[i].type);
        Symbol* sym = get_terminal(grammar, name, 0, 0);

        if (!sym) {
            fprintf(stderr, "Token no reconocido en gramática: %s\n", name);
            exit(1);
        }

        symbols[count++] = sym;
        free((char*)tokens[i].lexema);
    }

    // Añadir símbolo de fin $
    Symbol* dollar = get_terminal(grammar, "EOF", 0, 0);
    if (dollar) {
        symbols[count++] = dollar;
    }

    free(tokens);
    free(buffer);

    *out_count = count;
    return symbols;
}