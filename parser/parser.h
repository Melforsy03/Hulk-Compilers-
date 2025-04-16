#ifndef PARSER_H
#define PARSER_H
#include "../lexer/lexer.h"

// Tipos de nodo que puede tener el AST
typedef enum {
    NODO_LITERAL,
    NODO_VARIABLE,
    NODO_BINARIO,
    NODO_ASIGNACION,
    NODO_LET,
    NODO_PRINT,
    NODO_LLAMADA,
    NODO_BLOQUE, 
    NODO_FUNCION, 
    NODO_LITERAL_STRING,
    NODO_IF
} TipoNodo;

// Estructura del nodo del AST
typedef struct NodoAST {
    TipoNodo tipo;
    int linea;

    union {
        struct { char* nombre; } variable;
        struct { double valor; } literal;
        struct {
            struct NodoAST* izquierdo;
            Token operador;
            struct NodoAST* derecho;
        } binario;
        struct {
            char* nombre;
            struct NodoAST* valor;
        } asignacion;
        struct {
            char* nombre;
            struct NodoAST* valor;
            struct NodoAST* cuerpo;
        } let;
        struct {
            struct NodoAST* expresion;
        } print;
        struct {
            struct NodoAST** expresiones;
            int cantidad;
        } bloque;
        struct {
            char*nombre;
            struct NodoAst** argumento
        }llamada;
        struct 
        {
            char*nombre;
            char*parametro;
            struct NodoAST** cuerpo;
        }funcion;
        struct { char* valor; } literal_string;
        struct {
            struct NodoAST* condicion;
            struct NodoAST* entonces;
            struct NodoAST* sino;
        } ifthen;
        

    };
} NodoAST;

// Función principal del parser
NodoAST* parsear(Token* tokens);

// Liberar memoria
void liberar_ast(NodoAST* nodo);

// Imprimir AST
void imprimir_ast(NodoAST* nodo, int nivel);



#endif
