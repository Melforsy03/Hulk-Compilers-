#ifndef EVALUADOR_H
#define EVALUADOR_H
#include "./parser/parser.h"

// Definir los tipos posibles de valores
typedef enum {
    VALOR_NUMERO,
    VALOR_BOOL,
    VALOR_CADENA,
    VALOR_NULO,
    VALOR_OBJETO
} TipoValor;

// Declaración adelantada de Valor
typedef struct Valor Valor;

// Estructura de una lista de valores
typedef struct {
    Valor* valores;  // Un arreglo de valores
    int cantidad;    // La cantidad de elementos en el arreglo
} ListaValores;

// Ahora definimos la estructura de Valor
typedef struct Valor {
    TipoValor tipo;
    union {
        double numero;
        int booleano;
        char* cadena;
        void* objeto;  // Para instancias de objetos
        ListaValores lista;  // Lista de valores (como el iterable de un rango)
    };
} Valor;

// Definición de las estructuras para el entorno y variables
typedef struct Variable {
    char* nombre;
    Valor valor;  // El valor que tiene esta variable
    struct Variable* siguiente;
} Variable;

typedef struct Entorno {
    Variable* variables;
    struct Entorno* anterior;
} Entorno;

// Declaración de las funciones
Valor eval(NodoAST* nodo, Entorno* env);
Variable* obtener_variable(Entorno* env, const char* nombre);

#endif // EVALUADOR_H
