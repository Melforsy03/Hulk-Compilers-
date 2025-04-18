
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"
#include"lexer_modulo.h"
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


AnalizadorLexico lexer;
Token* tokens = NULL;
 extern int cantidad ;
 extern int capacidad ;

// Añadir un token a la lista de tokens
void agregar_token(TokenType tipo, const char* inicio, int longitud) {
    if (cantidad >= capacidad) {
        capacidad = capacidad == 0 ? 16 : capacidad * 2;
        Token* nuevo = realloc(tokens, sizeof(Token) * capacidad);
        if (!nuevo) {
            fprintf(stderr, "Error crítico: realloc falló\n");
            exit(1);
        }
        tokens = nuevo;
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

// Analiza identificadores y palabras clave
void analizar_identificador() {
    const char* inicio = lexer.actual - 1;
    while (isalnum(mirar()) || mirar() == '_') avanzar();
    int longitud = lexer.actual - inicio;

    char* lexema = strndup(inicio, longitud);
    Token resultado = analizar(lexema);
  
    if (resultado.type != TOKEN_ERROR) {
        agregar_token(resultado.type, inicio, longitud);
    } else if (acepta_identifier(lexema)) {
        agregar_token(TOKEN_IDENTIFIER, inicio, longitud);
    } else {
        agregar_token(TOKEN_ERROR, inicio, longitud);
    }

    free(lexema);
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
    char* lexema = strndup(inicio, longitud);
    if (acepta_number(lexema)) {
        agregar_token(TOKEN_NUMBER, inicio, longitud);
    } else {
        agregar_token(TOKEN_ERROR, inicio, longitud);
    }
    free(lexema);
    
}
// Analiza cadenas de texto, incluyendo secuencias de escape
void analizar_cadena() {
    const char* inicio = lexer.actual;
    while (mirar() != '"' && !fin_de_codigo()) {
        if (mirar() == '\n') lexer.linea++;
        if (mirar() == '\\' && mirar_siguiente() == '"') {
            lexer.actual += 2; 
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
    char* lexema = strndup(inicio - 1, longitud);
    if (acepta_string(lexema)) {
        agregar_token(TOKEN_STRING, inicio - 1, longitud);
    } else {
        agregar_token(TOKEN_ERROR, inicio - 1, longitud);
    }
    free(lexema);
    
}
int acepta_number(const char* texto) {
    int estado = 0;
    for (int i = 0; texto[i]; i++) {
        char c = texto[i];
        switch (estado) {
            case 0:
                if (c >= '0' && c <= '9') estado = 1;
                else return 0;
                break;
            case 1:
                if (c >= '0' && c <= '9') estado = 1;
                else if (c == '.') estado = 2;
                else return 0;
                break;
            case 2:
                if (c >= '0' && c <= '9') estado = 3;
                else return 0;
                break;
            case 3:
                if (c >= '0' && c <= '9') estado = 3;
                else return 0;
                break;
            default:
                return 0;
        }
    }
    return (estado == 1 || estado == 3);
}
int acepta_string(const char* texto) {
    int estado = 0;
    for (int i = 0; texto[i]; i++) {
        char c = texto[i];
        switch (estado) {
            case 0:
                if (c == '"') estado = 1;
                else return 0;
                break;
            case 1:
                if (c == '\\') estado = 2;         
                else if (c == '"') estado = 3;      
                else estado = 1;                     
                break;
            case 2:
                estado = 1; 
                break;
            case 3:
                return 0; 
            default:
                return 0;
        }
    }
    return estado == 3;
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
            case '&':
            if (coincidir('&')) {
                agregar_token(TOKEN_AND, lexer.actual - 2, 2); 
            } else {
                agregar_token(TOKEN_ERROR, lexer.actual - 1, 1); 
                fprintf(stderr, "[Error léxico] Se esperaba '&&' pero se encontró '&'\n");
            }
            break;
        
            case '|':
            if (coincidir('|')) {
                agregar_token(TOKEN_OR, lexer.actual - 2, 2); 
            } else {
                agregar_token(TOKEN_ERROR, lexer.actual - 1, 1);
                fprintf(stderr, "[Error léxico] Se esperaba '||' pero se encontró '|'\n");
            }
            break;
        
        case '(': agregar_token(TOKEN_LPAREN, lexer.actual - 1, 1); break;
        case ')': agregar_token(TOKEN_RPAREN, lexer.actual - 1, 1); break;
        case '{': agregar_token(TOKEN_LBRACE, lexer.actual - 1, 1); break;
        case '}': agregar_token(TOKEN_RBRACE, lexer.actual - 1, 1); break;
        case ',': agregar_token(TOKEN_COMMA, lexer.actual - 1, 1); break;
        case ';': agregar_token(TOKEN_SEMICOLON, lexer.actual - 1, 1); break;
        case '.': agregar_token(TOKEN_DOT, lexer.actual - 1, 1); break;
        case '"': analizar_cadena(); break;
        default:
            if (isalpha(c)) {
                analizar_identificador();
               
            } else if (isdigit(c)) {
                analizar_numero();
            } else {
                agregar_token(TOKEN_ERROR, lexer.actual - 1, 1);
            }
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

void free_tokens(Token* tokens) {
    for (int i = 0; i < cantidad; i++) {
        free((char*)tokens[i].lexeme);
    }
    free(tokens);
}


