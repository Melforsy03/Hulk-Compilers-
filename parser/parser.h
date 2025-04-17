#ifndef PARSER_H
#define PARSER_H
#include "../lexer/lexer.h"

typedef struct NodoAST NodoAST;
typedef struct Valor Valor;
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
    NODO_SET, 
    NODO_NEW,
    NODO_OBJETO,
    
} TipoNodo;

typedef struct NodoAST {
    TipoNodo tipo;
    int linea;
    union {
        struct {
            char* nombre; 
            NodoAST* siguiente; // Para la lista de parámetros
        } variable;
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
            NodoAST** miembros;
            int cantidad;
            char** parametros;
            int cantidad_parametros;
            char* padre;
        } tipo_decl;
        struct {
            char* tipo_nombre;
            NodoAST** argumentos;
            int cantidad;
        } nuevo;
        
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
        struct { 
            Valor* valores; 
            int cantidad;     
        } objeto;
        struct { char* nombre; NodoAST* objeto; NodoAST** argumentos; int cantidad; } llamada;
        struct { 
            char* nombre;  
            NodoAST** parametros;     
            int cantidad_parametros; 
            NodoAST* cuerpo;
        } funcion;
        
    };
} NodoAST;

typedef struct {
    char* nombre;
    NodoAST** miembros;
    int cantidad;
} NodoTipo;

NodoAST* parsear(Token* tokens);


void liberar_ast(NodoAST* nodo);


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
static NodoAST* parsear_igualdad();
static NodoAST* crear_binario(NodoAST* izq, Token op, NodoAST* der) ;
static NodoAST* parsear_unario();
static NodoAST* crear_rango(int inicio, int fin);
static NodoAST* agregar_a_lista(NodoAST* lista, NodoAST* nuevo) ;

#endif
