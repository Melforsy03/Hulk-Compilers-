#include <stdio.h>
#include "lexer.h"

const char* nombre_token(TokenType tipo) {
    switch (tipo) {
        case TOKEN_LET: return "LET";
        case TOKEN_IN: return "IN";
        case TOKEN_FUNCTION: return "FUNCTION";
        case TOKEN_TYPE: return "TYPE";
        case TOKEN_IF: return "IF";
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

        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_STAR: return "STAR";
        case TOKEN_SLASH: return "SLASH";
        case TOKEN_AT: return "AT";
        case TOKEN_POWER: return "POWER";
        case TOKEN_ASSIGN: return "ASSIGN";
        case TOKEN_COLON_EQUAL: return "COLON_EQUAL";
        case TOKEN_EQUAL_EQUAL: return "EQUAL_EQUAL";
        case TOKEN_NOT_EQUAL: return "NOT_EQUAL";
        case TOKEN_LESS: return "LESS";
        case TOKEN_LESS_EQUAL: return "LESS_EQUAL";
        case TOKEN_GREATER: return "GREATER";
        case TOKEN_GREATER_EQUAL: return "GREATER_EQUAL";
        case TOKEN_AND: return "AND";
        case TOKEN_OR: return "OR";
        case TOKEN_NOT: return "NOT";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_ARROW: return "ARROW";
        case TOKEN_DOT: return "DOT";

        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_STRING: return "STRING";

        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";

        default: return "DESCONOCIDO";
    }
}

int main() {
    const char* codigo = "let x = 42 + 3.14; print(x);";
    Token* tokens = tokenize(codigo);

    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        printf("Token: tipo=%s, lexema='%s', línea=%d\n", nombre_token(tokens[i].type), tokens[i].lexeme, tokens[i].line);
    }

    free_tokens(tokens);
    return 0;
}
