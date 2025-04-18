#ifndef LEXER_MODULO_H
#define LEXER_MODULO_H

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
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_POWER,
    TOKEN_AT,
    TOKEN_ASSIGN,
    TOKEN_COLON_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_ARROW,
    TOKEN_DOT,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,
    TOKEN_ERROR,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_EOF

} TokenType;

typedef struct {
    TokenType type;
    const char* lexeme;
    int line;
} Token;
typedef struct {
    const char* inicio;
    const char* actual;
    int linea;
} AnalizadorLexico;

extern Token* tokens;
extern int cantidad;
extern int capacidad;
Token analizar(const char* lexema);
int acepta_number(const char* texto);
int acepta_string(const char* texto);
#endif
