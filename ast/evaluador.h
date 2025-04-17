// evaluador.h

#ifndef EVALUADOR_H
#define EVALUADOR_H

#include "../parser/parser.h"  // Asegúrate de que 'parser.h' esté incluido para acceder a los tokens y nodos

// Definición de 'Funcion' que se almacenará en el entorno
typedef struct Funcion {
    char* nombre;
    NodoAST** parametros;      // Array de parámetros
    int cantidad_parametros;   // NUEVO CAMPO: número de parámetros
    NodoAST* cuerpo;
    struct Funcion* siguiente;
} Funcion;

// Definición de la estructura 'Valor' y demás tipos
typedef enum {
    VALOR_NUMERO,
    VALOR_BOOL,
    VALOR_CADENA,
    VALOR_NULO,
    VALOR_OBJETO
} TipoValor;

typedef struct Valor {
    TipoValor tipo;
    union {
        double numero;
        int booleano;
        char* cadena;
        void* objeto;  
        struct { 
            Valor* valores; 
            int cantidad;   
        } lista;
    };
} Valor;

// Definición de la estructura 'Variable'
typedef struct Variable {
    char* nombre;
    Valor valor;
    struct Variable* siguiente;
} Variable;

// Definición de la estructura 'Entorno'
typedef struct Entorno {
    Variable* variables;
    Funcion* funciones;  
    struct Entorno* anterior;
} Entorno;
Variable* obtener_variable(Entorno* env, const char* nombre);
Funcion* obtener_funcion(Entorno* env, const char* nombre);
Valor eval(NodoAST* nodo, Entorno* env);

#endif // EVALUADOR_H
