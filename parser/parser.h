#ifndef PARSER_H
#define PARSER_H
#include "../lexer/lexer.h"
typedef struct NodoAST NodoAST;
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
    NODO_IF, 
    NODO_NOT,
    NODO_WHILE,
    NODO_FOR ,
    NODO_TIPO ,
    NODO_METODO,
    NODO_ATRIBUTO,
    NODO_INSTANCIA,
    NODO_ACCESO,
    NODO_LITERAL_BOOL, 
    NODO_SET
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
            char* nombre;
            struct NodoAST** argumentos;
            int cantidad;
        } llamada;
        
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
        struct {
            struct NodoAST* condicion;
            struct NodoAST* cuerpo;
        } bucle_while;
        struct {
            char* variable;
            struct NodoAST* iterable;
            struct NodoAST* cuerpo;
        } bucle_for;
        struct {
            char* nombre;
            char* padre; 
            NodoAST** miembros;
            int cantidad;
        } tipo_decl;
        
        struct { char* nombre; NodoAST* valor; } atributo;
        struct {
            int valor; 
        } literal_bool;
        struct {
            NodoAST* objeto;
            char* miembro;
        } acceso;
        struct {
            NodoAST* destino;     
            NodoAST* valor;       
        } set;
        
        
    };
} NodoAST;

typedef struct {
    char* nombre;
    NodoAST** miembros;
    int cantidad;
} NodoTipo;
// Función principal del parser
NodoAST* parsear(Token* tokens);

// Liberar memoria
void liberar_ast(NodoAST* nodo);

// Imprimir AST
void imprimir_ast(NodoAST* nodo, int nivel);

static NodoAST* parsear_print();
static NodoAST* parsear_let();
static NodoAST* parsear_expresion();
static NodoAST* parsear_primario();
static NodoAST* parsear_bloque() ;
static NodoAST* parsear_asignacion();
static NodoAST* parsear_llamada();
static NodoAST* parsear_funcion();
static NodoAST* parsear_concatenacion();
static NodoAST* parsear_potencia();
static NodoAST* parsear_if();
static NodoAST* parsear_comparacion();
static NodoAST* parsear_elif();
static NodoAST* parsear_logico_and();
static NodoAST* parsear_logico_or();
static NodoAST* parsear_while();
static NodoAST* parsear_for();
static NodoAST* parsear_tipo();
static NodoAST* parsear_asignacion_set();
#endif
