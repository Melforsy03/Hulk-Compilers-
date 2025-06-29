#ifndef PARSER_LL1_H
#define PARSER_LL1_H
#include <string.h>
#include "../lexer/lexer.h" 
#include "../grammar/grammar.h" 
#include "../ast_nodes/ast_nodes.h" 

typedef struct {
    Token current_token;  
    const char** input;  
    Grammar* grammar;     
} ParserLL1;

// Crea nuevo parser usando tu lexer existente
ParserLL1* parser_ll1_new(const char** input, Grammar* grammar);

// Libera memoria del parser
void parser_ll1_free(ParserLL1* parser);

// Función principal de parsing
Node* parse_expression(ParserLL1* p);
Node* parse_program(ParserLL1* parser);

#endif