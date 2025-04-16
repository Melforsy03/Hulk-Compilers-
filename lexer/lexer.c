
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"
#ifndef _WIN32
#else

char* strndup(const char* s, size_t n) {
    char* p = malloc(n + 1);
    if (p) {
        strncpy(p, s, n);
        p[n] = '\0';
    }
    return p;
}
#endif

typedef struct {
    const char* inicio;
    const char* actual;
    int linea;
} AnalizadorLexico;

AnalizadorLexico lexer;
Token* tokens = NULL;
int capacidad = 0;
int cantidad = 0;

// Añadir un token a la lista de tokens
void agregar_token(TokenType tipo, const char* inicio, int longitud) {
    if (cantidad >= capacidad) {
        capacidad = capacidad == 0 ? 16 : capacidad * 2;
        tokens = realloc(tokens, sizeof(Token) * capacidad);
    }

    Token token;
    token.type = tipo;
    token.lexeme = strndup(inicio, longitud);
    token.line = lexer.linea;
    tokens[cantidad++] = token;
}
// Ignoramos espacios en blanco y salto de lineas
void saltar_espacios() {
    while (1) {
        char c = *lexer.actual;
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                lexer.actual++;
                break;
            case '\n':
                lexer.linea++;
                lexer.actual++;
                break;
            default:
                return;
        }
    }
}
 //verificamos si el caracter siguiente es el esperado
int coincidir(char esperado) {
    if (*lexer.actual != esperado) return 0;
    lexer.actual++;
    return 1;
}
// Avanza y devuelve el carácter actual
char avanzar() {
    return *lexer.actual++;
}

// Mira el carácter actual sin avanzar
char mirar() {
    return *lexer.actual;
}

// Mira el carácter siguiente
char mirar_siguiente() {
    if (mirar() == '\0') return '\0';
    return lexer.actual[1];
}

// Verifica si se llegó al final del texto
int fin_de_codigo() {
    return *lexer.actual == '\0';
}

// Detecta palabras clave y las convierte en tokens específicos#include <string.h>

TokenType tipo_identificador(const char* inicio, int longitud) {
    switch (longitud) {
        case 2:
            if (memcmp(inicio, "in", 2) == 0) return TOKEN_IN;
            if (memcmp(inicio, "if", 2) == 0) return TOKEN_IF;
            break;

        case 3:
            if (memcmp(inicio, "let", 3) == 0) return TOKEN_LET;
            if (memcmp(inicio, "for", 3) == 0) return TOKEN_FOR;
            if (memcmp(inicio, "new", 3) == 0) return TOKEN_NEW;
            break;

        case 4:
            if (memcmp(inicio, "base", 4) == 0) return TOKEN_BASE;
            if (memcmp(inicio, "elif", 4) == 0) return TOKEN_ELIF;
            if (memcmp(inicio, "else", 4) == 0) return TOKEN_ELSE;
            if (memcmp(inicio, "type", 4) == 0) return TOKEN_TYPE;
            if (memcmp(inicio, "self", 4) == 0) return TOKEN_SELF;
            if (memcmp(inicio, "true", 4) == 0) return TOKEN_TRUE;
            if (memcmp(inicio, "then", 4) == 0) return TOKEN_THEN;
            break;

        case 5:
            if (memcmp(inicio, "Print", 5) == 0) return TOKEN_PRINT;
            if (memcmp(inicio, "false", 5) == 0) return TOKEN_FALSE;
            if (memcmp(inicio, "while", 5) == 0) return TOKEN_WHILE;
            break;

        case 6:
            if (memcmp(inicio, "return", 6) == 0) return TOKEN_RETURN;
            break;

        case 8:
            if (memcmp(inicio, "function", 8) == 0) return TOKEN_FUNCTION;
            if (memcmp(inicio, "inherits", 8) == 0) return TOKEN_INHERITS;
            break;
    }

    return TOKEN_IDENTIFIER;
}


// Analiza identificadores y palabras clave
void analizar_identificador() {
    const char* inicio = lexer.actual - 1;
    while (isalnum(mirar()) || mirar() == '_') avanzar();
    int longitud = lexer.actual - inicio;
    agregar_token(tipo_identificador(inicio, longitud), inicio, longitud);
}

// Analiza números enteros o decimales
void analizar_numero() {
    const char* inicio = lexer.actual - 1;
    while (isdigit(mirar())) avanzar();
    if (mirar() == '.' && isdigit(mirar_siguiente())) {
        avanzar(); 
        while (isdigit(mirar())) avanzar();
    }
    int longitud = lexer.actual - inicio;
    agregar_token(TOKEN_NUMBER, inicio, longitud);
}

// Analiza cadenas de texto, incluyendo secuencias de escape
void analizar_cadena() {
    const char* inicio = lexer.actual;
    while (mirar() != '"' && !fin_de_codigo()) {
        if (mirar() == '\n') lexer.linea++;
        if (mirar() == '\\' && mirar_siguiente() == '"') {
            lexer.actual += 2; // saltar \" escapado
        } else {
            avanzar();
        }
    }

    if (fin_de_codigo()) {
        agregar_token(TOKEN_ERROR, inicio, lexer.actual - inicio);
        return;
    }

    avanzar(); // consumir comilla de cierre
    int longitud = lexer.actual - inicio + 1;
    agregar_token(TOKEN_STRING, inicio - 1, longitud);
}

// Analiza un token individual
void escanear_token() {
    saltar_espacios();

    if (fin_de_codigo()) return;

    char c = avanzar();
    switch (c) {
        case '+': agregar_token(TOKEN_PLUS, lexer.actual - 1, 1); break;
        case '-':
            if (coincidir('>')) agregar_token(TOKEN_ARROW, lexer.actual - 2, 2);
            else agregar_token(TOKEN_MINUS, lexer.actual - 1, 1);
            break;
        case '*': agregar_token(TOKEN_STAR, lexer.actual - 1, 1); break;
        case '/': agregar_token(TOKEN_SLASH, lexer.actual - 1, 1); break;
        case '@': agregar_token(TOKEN_AT, lexer.actual - 1, 1); break;
        case '^': agregar_token(TOKEN_POWER, lexer.actual - 1, 1); break;
        case '=':
            if (coincidir('>')) {
                agregar_token(TOKEN_ARROW, lexer.actual - 2, 2);  // reconoce =>
            } 
            else if (coincidir('=')) {
                agregar_token(TOKEN_EQUAL_EQUAL, lexer.actual - 2, 2);  // reconoce ==
            } else {
                agregar_token(TOKEN_ASSIGN, lexer.actual - 1, 1);  // reconoce =
            }
            break;
        case '!':
            if (coincidir('=')) agregar_token(TOKEN_NOT_EQUAL, lexer.actual - 2, 2);
            else agregar_token(TOKEN_NOT, lexer.actual - 1, 1);
            break;
        case ':':
            if (coincidir('=')) agregar_token(TOKEN_COLON_EQUAL, lexer.actual - 2, 2);
            break;
        case '<':
            if (coincidir('=')) agregar_token(TOKEN_LESS_EQUAL, lexer.actual - 2, 2);
            else agregar_token(TOKEN_LESS, lexer.actual - 1, 1);
            break;
        case '>':
            if (coincidir('=')) agregar_token(TOKEN_GREATER_EQUAL, lexer.actual - 2, 2);
            else agregar_token(TOKEN_GREATER, lexer.actual - 1, 1);
            break;
        case '&': agregar_token(TOKEN_AND, lexer.actual - 1, 1); break;
        case '|': agregar_token(TOKEN_OR, lexer.actual - 1, 1); break;
        case '(': agregar_token(TOKEN_LPAREN, lexer.actual - 1, 1); break;
        case ')': agregar_token(TOKEN_RPAREN, lexer.actual - 1, 1); break;
        case '{': agregar_token(TOKEN_LBRACE, lexer.actual - 1, 1); break;
        case '}': agregar_token(TOKEN_RBRACE, lexer.actual - 1, 1); break;
        case ',': agregar_token(TOKEN_COMMA, lexer.actual - 1, 1); break;
        case ';': agregar_token(TOKEN_SEMICOLON, lexer.actual - 1, 1); break;
        case '.': agregar_token(TOKEN_DOT, lexer.actual - 1, 1); break;
        case '"': analizar_cadena(); break;
        default:
            if (isalpha(c)) analizar_identificador();
            else if (isdigit(c)) analizar_numero();
            else agregar_token(TOKEN_ERROR, lexer.actual - 1, 1);
            break;
    }
}

// Función principal para generar los tokens
Token* tokenize(const char* codigo) {
    lexer.inicio = codigo;
    lexer.actual = codigo;
    lexer.linea = 1;
    tokens = NULL;
    capacidad = 0;
    cantidad = 0;

    while (!fin_de_codigo()) {
        escanear_token();
    }

    agregar_token(TOKEN_EOF, lexer.actual, 0);
    return tokens;
}

// Liberar la memoria usada por los tokens
void free_tokens(Token* tokens) {
    for (int i = 0; i < cantidad; i++) {
        free((char*)tokens[i].lexeme);
    }
    free(tokens);
}


