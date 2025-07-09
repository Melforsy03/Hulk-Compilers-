


#ifndef LL1_PARSER_H
#define LL1_PARSER_H

#include "ll1_table.h"
#include "grammar.h"
#include "lexer.h"
#include "cst.h"

// Estructura principal del parser LL(1)
typedef struct {
    const char* input;          // Texto de entrada
    Token current_token;        // Token actual
    LL1Table* ll1_table;        // Tabla LL(1)
    const Grammar* grammar;     // Gramática
} LL1Parser;

// Inicializa el parser con la entrada, tabla y gramática
void init_parser(LL1Parser* parser, const char* input, LL1Table* table, const Grammar* grammar);

// Parsea desde el símbolo inicial y devuelve el CST
CSTNode* parse(LL1Parser* parser, const char* start_symbol);

// Busca la producción correspondiente en la tabla LL(1)
Production* find_ll1_entry(const LL1Table* table, const char* non_terminal, const char* terminal);

#endif // LL1_PARSER_H
