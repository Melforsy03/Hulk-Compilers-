#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOKEN_LET,
    TOKEN_IN,
    TOKEN_FUNCTION,
    TOKEN_TYPE,
    TOKEN_IF,
    TOKEN_THEN,
    TOKEN_ELIF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NEW,
    TOKEN_INHERITS,
    TOKEN_SELF,
    TOKEN_BASE,
    TOKEN_RETURN,
    TOKEN_PRINT,
    TOKEN_RANGE,
    TOKEN_PROTOCOL,
    TOKEN_EXTENDS,
    TOKEN_TAN,
    TOKEN_COS,
    TOKEN_SIN,
    TOKEN_LOG,
    TOKEN_PI,
    TOKEN_COT,
    TOKEN_DSTAR,
    TOKEN_ARROW,
    TOKEN_COLON_EQUAL,
    TOKEN_AT_AT,
    TOKEN_EQUAL_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER_EQUAL,
    TOKEN_AND,
    TOKEN_OR,

    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_MODULO,
    TOKEN_POWER,

    TOKEN_AT,
    TOKEN_ASSIGN,
    TOKEN_LESS,
    TOKEN_GREATER,
    TOKEN_NOT,

    TOKEN_DOT,
    TOKEN_PUNTOS,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,

    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,

    TOKEN_WHITESPACE,
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;


typedef struct Transicion {
    int destino;
    char simbolo;
} Transicion;

typedef struct EstadoDFA {
    int id;
    int num_transiciones;
    Transicion transiciones[128];
    int es_final;
    TokenType tipo;
} EstadoDFA;

typedef struct {
    TokenType type;
    const char* lexema;
    int length;
    int line;    // <--- añadir
    int column; 
} Token;

Token next_token(const char** input);
void print_token(Token t);

#endif