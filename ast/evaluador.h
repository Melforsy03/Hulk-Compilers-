#include <./parser/parser.h>
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
    };
} Valor;
typedef struct Variable {
    char* nombre;
    Valor valor;
    struct Variable* siguiente;
} Variable;

typedef struct Entorno {
    Variable* variables;
    struct Entorno* anterior;
} Entorno;

Valor eval(NodoAST* nodo, Entorno* env);
Valor obtener_variable(Entorno* env, const char* nombre);