
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "lexer.h"
#include "func_aux_lexer.h"
const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_LET: return "LET";
        case TOKEN_IN: return "IN";
        case TOKEN_FUNCTION: return "FUNCTION";
        case TOKEN_TYPE: return "TYPE";
        case TOKEN_IF: return "IF";
        case TOKEN_THEN: return "THEN";
        case TOKEN_ELIF: return "ELIF";
        case TOKEN_ELSE: return "ELSE";
        case TOKEN_WHILE: return "WHILE";
        case TOKEN_FOR: return "FOR";
        case TOKEN_TRUE: return "TRUE";
        case TOKEN_FALSE: return "FALSE";
        case TOKEN_NEW: return "NEW";
        case TOKEN_INHERITS: return "INHERITS";
        case TOKEN_SELF: return "SELF";
        case TOKEN_BASE: return "BASE";
        case TOKEN_RETURN: return "RETURN";
        case TOKEN_PRINT: return "PRINT";
        case TOKEN_RANGE: return "RANGE";
        case TOKEN_PROTOCOL: return "PROTOCOL";
        case TOKEN_EXTENDS: return "EXTENDS";
        case TOKEN_DSTAR: return "DSTAR";
        case TOKEN_ARROW: return "ARROW";
        case TOKEN_COLON_EQUAL: return "COLON_EQUAL";
        case TOKEN_AT_AT: return "AT_AT";
        case TOKEN_EQUAL_EQUAL: return "EQUAL_EQUAL";
        case TOKEN_NOT_EQUAL: return "NOT_EQUAL";
        case TOKEN_LESS_EQUAL: return "LESS_EQUAL";
        case TOKEN_GREATER_EQUAL: return "GREATER_EQUAL";
        case TOKEN_AND: return "AND";
        case TOKEN_OR: return "OR";

        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_STAR: return "STAR";
        case TOKEN_SLASH: return "SLASH";
        case TOKEN_MODULO: return "MODULO";
        case TOKEN_POWER: return "POWER";

        case TOKEN_AT: return "AT";
        case TOKEN_ASSIGN: return "ASSIGN";
        case TOKEN_LESS: return "LESS";
        case TOKEN_GREATER: return "GREATER";
        case TOKEN_NOT: return "NOT";

        case TOKEN_DOT: return "DOT";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_LBRACKET: return "LBRACKET";
        case TOKEN_RBRACKET: return "RBRACKET";

        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_STRING: return "STRING";

        case TOKEN_WHITESPACE: return "WHITESPACE";
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";

        default: return "UNDEFINED";
    }
}



void lexer_free(Lexer* lexer) {
    if (!lexer) return;
    free((char*)lexer->source);  // casteamos porque strdup devuelve char*
    free(lexer);
}
Lexer* lexer_new(const char* source) {
    Lexer* lexer = (Lexer*) malloc(sizeof(Lexer));
    if (!lexer) return NULL;
    lexer->source = strdup(source);  // duplicamos para seguridad
    lexer->pos = 0;
    return lexer;
}
#include "lexer.h"
#include "parser.h"  // Para Symbol, Grammar, get_terminal

extern const char* token_type_to_string(TokenType type);
extern Token* next_tokens(const char* input, int* count);

