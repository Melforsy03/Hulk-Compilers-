#ifndef REGEX_PARSER_H
#define REGEX_PARSER_H

typedef enum {
    REGEX_CHAR,
    REGEX_ANY,
    REGEX_CLASS,
    REGEX_CONCAT,
    REGEX_ALT,
    REGEX_STAR,
    REGEX_PLUS,
    REGEX_OPTIONAL
} NodoTIpo;

typedef struct Nodo {
    NodoTIpo tipo;
    union {
        char caracter;            
        struct {                  
            char desde;
            char hasta;
        } clase;
        struct { struct Nodo* izq; struct Nodo* der; } binario;
        struct Nodo* unico;
    };
} Nodo;

Nodo* parsear_regex(const char* regex);
void liberar_regex(Nodo* nodo);
void imprimir_arbol(Nodo* nodo, int nivel);

#endif
