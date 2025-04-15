#include <stdio.h>
#include "lexer.h"
#define RESET_COLOR   "\x1b[0m"
#define RED_COLOR     "\x1b[31m"
#define GREEN_COLOR   "\x1b[32m"
#define YELLOW_COLOR  "\x1b[33m"
#define BLUE_COLOR    "\x1b[34m"
#define MAGENTA_COLOR "\x1b[35m"
#define CYAN_COLOR    "\x1b[36m"
#define GRAY_COLOR    "\x1b[90m"

const char* nombre_token(TokenType tipo) {
    switch (tipo) {
        case TOKEN_LET: return GREEN_COLOR "LET" RESET_COLOR;
        case TOKEN_IN: return GREEN_COLOR "IN" RESET_COLOR;
        case TOKEN_FUNCTION: return GREEN_COLOR "FUNCTION" RESET_COLOR;
        case TOKEN_TYPE: return GREEN_COLOR "TYPE" RESET_COLOR;
        case TOKEN_IF: return GREEN_COLOR "IF" RESET_COLOR;
        case TOKEN_ELIF: return GREEN_COLOR "ELIF" RESET_COLOR;
        case TOKEN_ELSE: return GREEN_COLOR "ELSE" RESET_COLOR;
        case TOKEN_WHILE: return GREEN_COLOR "WHILE" RESET_COLOR;
        case TOKEN_FOR: return GREEN_COLOR "FOR" RESET_COLOR;
        case TOKEN_TRUE: return CYAN_COLOR "TRUE" RESET_COLOR;
        case TOKEN_FALSE: return CYAN_COLOR "FALSE" RESET_COLOR;
        case TOKEN_NEW: return GREEN_COLOR "NEW" RESET_COLOR;
        case TOKEN_INHERITS: return GREEN_COLOR "INHERITS" RESET_COLOR;
        case TOKEN_SELF: return YELLOW_COLOR "SELF" RESET_COLOR;
        case TOKEN_BASE: return YELLOW_COLOR "BASE" RESET_COLOR;
        case TOKEN_RETURN: return GREEN_COLOR "RETURN" RESET_COLOR;

        case TOKEN_PLUS: return BLUE_COLOR "PLUS" RESET_COLOR;
        case TOKEN_MINUS: return BLUE_COLOR "MINUS" RESET_COLOR;
        case TOKEN_STAR: return BLUE_COLOR "STAR" RESET_COLOR;
        case TOKEN_SLASH: return BLUE_COLOR "SLASH" RESET_COLOR;
        case TOKEN_AT: return BLUE_COLOR "AT" RESET_COLOR;
        case TOKEN_POWER: return BLUE_COLOR "POWER" RESET_COLOR;
        case TOKEN_ASSIGN: return BLUE_COLOR "ASSIGN" RESET_COLOR;
        case TOKEN_COLON_EQUAL: return BLUE_COLOR "COLON_EQUAL" RESET_COLOR;
        case TOKEN_EQUAL_EQUAL: return BLUE_COLOR "EQUAL_EQUAL" RESET_COLOR;
        case TOKEN_NOT_EQUAL: return BLUE_COLOR "NOT_EQUAL" RESET_COLOR;
        case TOKEN_LESS: return BLUE_COLOR "LESS" RESET_COLOR;
        case TOKEN_LESS_EQUAL: return BLUE_COLOR "LESS_EQUAL" RESET_COLOR;
        case TOKEN_GREATER: return BLUE_COLOR "GREATER" RESET_COLOR;
        case TOKEN_GREATER_EQUAL: return BLUE_COLOR "GREATER_EQUAL" RESET_COLOR;
        case TOKEN_AND: return BLUE_COLOR "AND" RESET_COLOR;
        case TOKEN_OR: return BLUE_COLOR "OR" RESET_COLOR;
        case TOKEN_NOT: return BLUE_COLOR "NOT" RESET_COLOR;
        case TOKEN_LPAREN: return MAGENTA_COLOR "LPAREN" RESET_COLOR;
        case TOKEN_RPAREN: return MAGENTA_COLOR "RPAREN" RESET_COLOR;
        case TOKEN_LBRACE: return MAGENTA_COLOR "LBRACE" RESET_COLOR;
        case TOKEN_RBRACE: return MAGENTA_COLOR "RBRACE" RESET_COLOR;
        case TOKEN_COMMA: return MAGENTA_COLOR "COMMA" RESET_COLOR;
        case TOKEN_SEMICOLON: return MAGENTA_COLOR "SEMICOLON" RESET_COLOR;
        case TOKEN_ARROW: return MAGENTA_COLOR "ARROW" RESET_COLOR;
        case TOKEN_DOT: return MAGENTA_COLOR "DOT" RESET_COLOR;

        case TOKEN_IDENTIFIER: return YELLOW_COLOR "IDENTIFIER" RESET_COLOR;
        case TOKEN_NUMBER: return CYAN_COLOR "NUMBER" RESET_COLOR;
        case TOKEN_STRING: return CYAN_COLOR "STRING" RESET_COLOR;

        case TOKEN_EOF: return GRAY_COLOR "EOF" RESET_COLOR;
        case TOKEN_ERROR: return RED_COLOR "ERROR" RESET_COLOR;

        default: return RED_COLOR "UNKNOWN" RESET_COLOR;
    }
}


int main() {
    const char* codigo = "let x = 42 + 3.14; print(x);";
    Token* tokens = tokenize(codigo);

    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        printf("[%-15s] '%s'  (linea %d)\n", nombre_token(tokens[i].type), tokens[i].lexeme, tokens[i].line);
    }
    
    free_tokens(tokens);
    return 0;
}
