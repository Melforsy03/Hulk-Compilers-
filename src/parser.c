#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

// Token actual del análisis
static Token* actual;
static NodoAST* parsear_print();
static NodoAST* parsear_let();
static NodoAST* parsear_expresion();
static NodoAST* parsear_primario();

// Avanzar al siguiente token
static void avanzar() {
    actual++;
}

// Coincide con un tipo específico de token
static int coincidir(TokenType tipo) {
    if (actual->type == tipo) {
        avanzar();
        return 1;
    }
    return 0;
}


// Exigir un token específico o lanzar error
static void exigir(TokenType tipo, const char* esperado) {
    if (!coincidir(tipo)) {
        fprintf(stderr, "[Error de sintaxis] Se esperaba %s en línea %d, se encontró '%s'\n",
                esperado, actual->line, actual->lexeme);
        exit(1);
    }
}

// Crear nodo literal
static NodoAST* crear_literal(double valor, int linea) {
    NodoAST* nodo = malloc(sizeof(NodoAST));
    nodo->tipo = NODO_LITERAL;
    nodo->linea = linea;
    nodo->literal.valor = valor;
    return nodo;
}

// Crear nodo variable
static NodoAST* crear_variable(const char* nombre, int linea) {
    NodoAST* nodo = malloc(sizeof(NodoAST));
    nodo->tipo = NODO_VARIABLE;
    nodo->linea = linea;
    nodo->variable.nombre = strdup(nombre);
    return nodo;
}

static NodoAST* parsear_print() {
    exigir(TOKEN_PRINT, "'Print'");
    exigir(TOKEN_LPAREN, "'('");
    NodoAST* expr = parsear_expresion();
    exigir(TOKEN_RPAREN, "')'");
    NodoAST* nodo = malloc(sizeof(NodoAST));
    nodo->tipo = NODO_PRINT;
    nodo->linea = actual[-1].line;
    nodo->print.expresion = expr;
    return nodo;
}

static NodoAST* parsear_termino() {
    NodoAST* izquierdo = parsear_primario();

    while (actual->type == TOKEN_STAR || actual->type == TOKEN_SLASH) {
        Token op = *actual;
        avanzar();
        NodoAST* derecho = parsear_primario();

        NodoAST* nodo = malloc(sizeof(NodoAST));
        nodo->tipo = NODO_BINARIO;
        nodo->linea = op.line;
        nodo->binario.izquierdo = izquierdo;
        nodo->binario.operador = op;
        nodo->binario.derecho = derecho;
        izquierdo = nodo;
    }

    return izquierdo;
}

static NodoAST* parsear_expresion() {
    NodoAST* izquierdo = parsear_termino();

    while (actual->type == TOKEN_PLUS || actual->type == TOKEN_MINUS) {
        Token op = *actual;
        avanzar();
        NodoAST* derecho = parsear_termino();

        NodoAST* nodo = malloc(sizeof(NodoAST));
        nodo->tipo = NODO_BINARIO;
        nodo->linea = op.line;
        nodo->binario.izquierdo = izquierdo;
        nodo->binario.operador = op;
        nodo->binario.derecho = derecho;
        izquierdo = nodo;
    }

    return izquierdo;
}

static NodoAST* parsear_primario() {
    if (coincidir(TOKEN_NUMBER)) {
        return crear_literal(atof(actual[-1].lexeme), actual[-1].line);
    }

    if (coincidir(TOKEN_IDENTIFIER)) {
        return crear_variable(actual[-1].lexeme, actual[-1].line);
    }

    if (coincidir(TOKEN_PRINT)) {
        actual--; // retrocedemos para que `parsear_print()` lo procese bien
        return parsear_print();
    }

    if (coincidir(TOKEN_LPAREN)) {
        NodoAST* expr = parsear_expresion();
        exigir(TOKEN_RPAREN, "')'");
        return expr;
    }

    fprintf(stderr, "[Error de sintaxis] Expresión inválida en línea %d: '%s'\n",
            actual->line, actual->lexeme);
    exit(1);
}


static NodoAST* parsear_let() {
    exigir(TOKEN_LET, "'let'");
    exigir(TOKEN_IDENTIFIER, "nombre de variable");
    char* nombre = strdup(actual[-1].lexeme);

    exigir(TOKEN_ASSIGN, "'='");

    NodoAST* valor = parsear_expresion();

    exigir(TOKEN_IN, "'in'");

    NodoAST* cuerpo = parsear_expresion();

    NodoAST* nodo = malloc(sizeof(NodoAST));
    nodo->tipo = NODO_LET;
    nodo->linea = actual[-1].line;
    nodo->let.nombre = nombre;
    nodo->let.valor = valor;
    nodo->let.cuerpo = cuerpo;
    return nodo;
}

// Función principal
NodoAST* parsear(Token* tokens) {
    actual = tokens;

    if (coincidir(TOKEN_LET)) {
        actual--;
        return parsear_let();
    }

    if (coincidir(TOKEN_PRINT)) {
        actual--;
        return parsear_print();
    }

    return parsear_expresion();
}

void imprimir_ast(NodoAST* nodo, int nivel) {
    if (!nodo) return;

    for (int i = 0; i < nivel; i++) printf("  ");

    switch (nodo->tipo) {
        case NODO_LITERAL:
            printf("LITERAL: %.2f\n", nodo->literal.valor);
            break;

        case NODO_VARIABLE:
            printf("VARIABLE: %s\n", nodo->variable.nombre);
            break;

        case NODO_BINARIO:
            printf("BINARIO: %s\n", nodo->binario.operador.lexeme);
            imprimir_ast(nodo->binario.izquierdo, nivel + 1);
            imprimir_ast(nodo->binario.derecho, nivel + 1);
            break;

        case NODO_PRINT:
            printf("PRINT:\n");
            imprimir_ast(nodo->print.expresion, nivel + 1);
            break;

        case NODO_LET:
            printf("LET: %s =\n", nodo->let.nombre);
            imprimir_ast(nodo->let.valor, nivel + 1);
            for (int i = 0; i < nivel; i++) printf("  ");
            printf("EN:\n");
            imprimir_ast(nodo->let.cuerpo, nivel + 1);
            break;

        default:
            printf("TIPO DE NODO DESCONOCIDO\n");
            break;
    }
}
void liberar_ast(NodoAST* nodo) {
    if (!nodo) return;

    switch (nodo->tipo) {
        case NODO_LITERAL:
            break;
        case NODO_VARIABLE:
            free(nodo->variable.nombre);
            break;
        case NODO_BINARIO:
            liberar_ast(nodo->binario.izquierdo);
            liberar_ast(nodo->binario.derecho);
            break;
        case NODO_LET:
            free(nodo->let.nombre);
            liberar_ast(nodo->let.valor);
            liberar_ast(nodo->let.cuerpo);
            break;
        case NODO_PRINT:
            liberar_ast(nodo->print.expresion);
            break;
        default:
            break;
    }

    free(nodo);
}
