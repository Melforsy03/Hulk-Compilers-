#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./parser/parser.h"
#include "./lexer/lexer.h"
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
Valor obtener_variable(Entorno* env, const char* nombre) {
    while (env) {
        Variable* v = env->variables;
        while (v) {
            if (strcmp(v->nombre, nombre) == 0) {
                return v->valor;
            }
            v = v->siguiente;
        }
        env = env->anterior;
    }
    fprintf(stderr, "Error: variable no definida: %s\n", nombre);
    exit(1);
}
Valor eval(NodoAST* nodo, Entorno* env) {
    switch (nodo->tipo) {
        case NODO_LITERAL: {
            Valor v;
            v.tipo = VALOR_NUMERO;
            v.numero = nodo->literal.valor;
            return v;
        }
        case NODO_LITERAL_BOOL: {
            Valor v;
            v.tipo = VALOR_BOOL;
            v.booleano = nodo->literal_bool.valor;
            return v;
        }
        case NODO_LITERAL_STRING: {
            Valor v;
            v.tipo = VALOR_CADENA;
            v.cadena = strdup(nodo->literal_string.valor);
            return v;
        }
        case NODO_VARIABLE: {
            return obtener_variable(env, nodo->variable.nombre);
        }        
        case NODO_LET: {
            // evalua el valor inicial
            Valor valor = eval(nodo->let.valor, env);
        
            //  Crea un nuevo entorno con ese valor ligado a la variable
            Entorno* nuevo = malloc(sizeof(Entorno));
            nuevo->variables = NULL;
            nuevo->anterior = env;
        
            Variable* var = malloc(sizeof(Variable));
            var->nombre = strdup(nodo->let.nombre);
            var->valor = valor;
            var->siguiente = NULL;
        
            nuevo->variables = var;
        
            //  Evalúa la expresión del cuerpo en el nuevo entorno
            Valor resultado = eval(nodo->let.cuerpo, nuevo);
        
            return resultado;
        }
        case NODO_ASIGNACION: {
            Variable* var = env->variables;
            while (var) {
                if (strcmp(var->nombre, nodo->asignacion.nombre) == 0) {
                    Valor nuevo = eval(nodo->asignacion.valor, env);
                    var->valor = nuevo;
                    return nuevo;
                }
                var = var->siguiente;
            }
        
            // Si no se encuentra la variable en el entorno actual
            fprintf(stderr, "Error: variable '%s' no declarada para asignación (:=)\n", nodo->asignacion.nombre);
            exit(1);
        }        
        case NODO_PRINT: {
                Valor valor = eval(nodo->print.expresion, env);
            
                printf("[Print] ");
            
                switch (valor.tipo) {
                    case VALOR_NUMERO:
                        printf("%.2f", valor.numero);
                        break;
                    case VALOR_BOOL:
                        printf("%s", valor.booleano ? "true" : "false");
                        break;
                    case VALOR_CADENA:
                        printf("%s", valor.cadena);
                        break;
                    default:
                        printf("(nulo)");
                        break;
                }
            
                printf("\n");
            
                Valor vacio;
                vacio.tipo = VALOR_NULO;
                return vacio;
            }
        case NODO_BLOQUE: {
                Valor resultado;
                resultado.tipo = VALOR_NULO;
            
                for (int i = 0; i < nodo->bloque.cantidad; i++) {
                    resultado = eval(nodo->bloque.expresiones[i], env);
                }
            
                return resultado;
            }
                
            // Si no se encuentra la variable en el entorno actual
            fprintf(stderr, "Error: variable '%s' no declarada para asignación (:=)\n", nodo->asignacion.nombre);
            exit(1);
        
        default:
            fprintf(stderr, " Nodo no manejado aun en eval.c (tipo %d)\n", nodo->tipo);
            exit(1);
    }
}

