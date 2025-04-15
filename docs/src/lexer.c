//Definimos los tokens 

#ifndef LEXER_H
#define LEXER_H

typedef enum {

    // Palabras clave
    TOKEN_LET, TOKEN_IN, TOKEN_FUNCTION, TOKEN_TYPE,
    TOKEN_IF, TOKEN_ELIF, TOKEN_ELSE,
    TOKEN_WHILE, TOKEN_FOR, TOKEN_TRUE, TOKEN_FALSE,
    TOKEN_NEW, TOKEN_INHERITS, TOKEN_SELF, TOKEN_BASE, TOKEN_RETURN,

    // Símbolos y operadores
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH, TOKEN_AT, TOKEN_POWER,
    TOKEN_ASSIGN, TOKEN_COLON_EQUAL, TOKEN_EQUAL_EQUAL, TOKEN_NOT_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL, TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_AND, TOKEN_OR, TOKEN_NOT,
    TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_LBRACE, TOKEN_RBRACE,
    TOKEN_COMMA, TOKEN_SEMICOLON, TOKEN_ARROW, TOKEN_DOT,

    // Literales
    TOKEN_IDENTIFIER, TOKEN_NUMBER, TOKEN_STRING,

    // Especiales
    TOKEN_EOF,
    TOKEN_ERROR
    
} TokenType;

typedef struct {
    TokenType type;
    const char* lexeme;
    int line;
} Token;

Token* tokenize(const char* source);
void free_tokens(Token* tokens);

#endif
