// regex_parser.c
#include "regex_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static const char* entrada;
static int posicion;

static char actual() {
    return entrada[posicion];
}

static char avanzar() {
    return entrada[posicion++];
}

static int fin() {
    return entrada[posicion] == '\0';
}

Nodo* crear_nodo(NodoTIpo tipo) {
    Nodo* n = malloc(sizeof(Nodo));
    n->tipo = tipo;
    return n;
}

Nodo* parse_char() {
    char c = avanzar();
    Nodo* n = crear_nodo(REGEX_CHAR);
    n->caracter = c;
    return n;
}

Nodo* parse_class() {
    avanzar(); // saltar '['
    Nodo* acumulador = NULL;

    while (actual() != ']' && !fin()) {
        char inicio = avanzar();
        if (actual() == '-' && entrada[posicion + 1] != ']') {
            avanzar(); // saltar '-'
            char fin_rango = avanzar();

            for (char c = inicio; c <= fin_rango; c++) {
                Nodo* letra = crear_nodo(REGEX_CHAR);
                letra->caracter = c;

                if (!acumulador) {
                    acumulador = letra;
                } else {
                    Nodo* nuevo_alt = crear_nodo(REGEX_ALT);
                    nuevo_alt->binario.izq = acumulador;
                    nuevo_alt->binario.der = letra;
                    acumulador = nuevo_alt;
                }
            }
        } else {
            Nodo* letra = crear_nodo(REGEX_CHAR);
            letra->caracter = inicio;

            if (!acumulador) {
                acumulador = letra;
            } else {
                Nodo* nuevo_alt = crear_nodo(REGEX_ALT);
                nuevo_alt->binario.izq = acumulador;
                nuevo_alt->binario.der = letra;
                acumulador = nuevo_alt;
            }
        }
    }

    if (actual() == ']') avanzar(); // saltar ']'

    return acumulador;
}


Nodo* parse_base();

Nodo* parse_factor() {
    Nodo* base = parse_base();
    if (actual() == '*') {
        avanzar();
        Nodo* n = crear_nodo(REGEX_STAR);
        n->unico = base;
        return n;
    } else if (actual() == '+') {
        avanzar();
        Nodo* n = crear_nodo(REGEX_PLUS);
        n->unico = base;
        return n;
    } else if (actual() == '?') {
        avanzar();
        Nodo* n = crear_nodo(REGEX_OPTIONAL);
        n->unico = base;
        return n;
    }
    return base;
}

Nodo* parse_base() {
    if (actual() == '(') {
        avanzar();
        Nodo* e = parsear_regex(entrada + posicion);
        while (actual() != ')' && !fin()) avanzar();
        avanzar(); // consumir ')'
        return e;
    } else if (actual() == '[') {
        return parse_class();
    } else if (actual() == '.') {
        avanzar();
        Nodo* n = crear_nodo(REGEX_ANY);
        return n;
    } else {
        return parse_char();
    }
}

Nodo* parse_term() {
    Nodo* izq = parse_factor();
    while (!fin() && actual() != '|' && actual() != ')') {
        Nodo* der = parse_factor();
        Nodo* nuevo = crear_nodo(REGEX_CONCAT);
        nuevo->binario.izq = izq;
        nuevo->binario.der = der;
        izq = nuevo;
    }
    return izq;
}

Nodo* parsear_regex(const char* r) {
    entrada = r;
    posicion = 0;
    Nodo* izq = parse_term();
    while (!fin() && actual() == '|') {
        avanzar();
        Nodo* der = parse_term();
        Nodo* nuevo = crear_nodo(REGEX_ALT);
        nuevo->binario.izq = izq;
        nuevo->binario.der = der;
        izq = nuevo;
    }
    return izq;
}

void liberar_regex(Nodo* nodo) {
    if (!nodo) return;
    switch (nodo->tipo) {
        case REGEX_CHAR:
        case REGEX_ANY:
        case REGEX_CLASS:
            break;
        case REGEX_CONCAT:
        case REGEX_ALT:
            liberar_regex(nodo->binario.izq);
            liberar_regex(nodo->binario.der);
            break;
        case REGEX_STAR:
        case REGEX_PLUS:
        case REGEX_OPTIONAL:
            liberar_regex(nodo->unico);
            break;
    }
    free(nodo);
}

void imprimir_arbol(Nodo* nodo, int nivel) {
    if (!nodo) return;
    for (int i = 0; i < nivel; i++) printf("  ");
    switch (nodo->tipo) {
        case REGEX_CHAR: printf("CHAR '%c'\n", nodo->caracter); break;
        case REGEX_ANY: printf("ANY .\n"); break;
        case REGEX_CLASS: printf("CLASS [%c-%c]\n", nodo->clase.desde, nodo->clase.hasta); break;
        case REGEX_CONCAT: printf("CONCAT\n"); break;
        case REGEX_ALT: printf("ALT\n"); break;
        case REGEX_STAR: printf("STAR *\n"); break;
        case REGEX_PLUS: printf("PLUS +\n"); break;
        case REGEX_OPTIONAL: printf("OPTIONAL ?\n"); break;
    }
    if (nodo->tipo == REGEX_CONCAT || nodo->tipo == REGEX_ALT) {
        imprimir_arbol(nodo->binario.izq, nivel + 1);
        imprimir_arbol(nodo->binario.der, nivel + 1);
    } else if (nodo->tipo == REGEX_STAR || nodo->tipo == REGEX_PLUS || nodo->tipo == REGEX_OPTIONAL) {
        imprimir_arbol(nodo->unico, nivel + 1);
    }
}
