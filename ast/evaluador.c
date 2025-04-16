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
        case NODO_IF: {
                Valor cond = eval(nodo->ifthen.condicion, env);
            
                if (cond.tipo != VALOR_BOOL) {
                    fprintf(stderr, "Error: la condición del 'if' no es booleana.\n");
                    exit(1);
                }
            
                if (cond.booleano) {
                    return eval(nodo->ifthen.entonces, env);
                } else {
                    return eval(nodo->ifthen.sino, env);
                }
            }
        case NODO_BINARIO: {
                Valor izq = eval(nodo->binario.izquierdo, env);
                Valor der = eval(nodo->binario.derecho, env);
                TokenType op = nodo->binario.operador.type;
            
                Valor resultado;
            
                switch (op) {
                    case TOKEN_PLUS:
                        resultado.tipo = VALOR_NUMERO;
                        resultado.numero = izq.numero + der.numero;
                        break;
                    case TOKEN_MINUS:
                        resultado.tipo = VALOR_NUMERO;
                        resultado.numero = izq.numero - der.numero;
                        break;
                    case TOKEN_STAR:
                        resultado.tipo = VALOR_NUMERO;
                        resultado.numero = izq.numero * der.numero;
                        break;
                    case TOKEN_SLASH:
                        resultado.tipo = VALOR_NUMERO;
                        resultado.numero = izq.numero / der.numero;
                        break;
                    case TOKEN_POWER:
                        resultado.tipo = VALOR_NUMERO;
                        resultado.numero = pow(izq.numero, der.numero);
                        break;
            
                    case TOKEN_EQUAL_EQUAL:
                        resultado.tipo = VALOR_BOOL;
                        resultado.booleano = (izq.numero == der.numero);
                        break;
                    case TOKEN_NOT_EQUAL:
                        resultado.tipo = VALOR_BOOL;
                        resultado.booleano = (izq.numero != der.numero);
                        break;
                    case TOKEN_GREATER:
                        resultado.tipo = VALOR_BOOL;
                        resultado.booleano = (izq.numero > der.numero);
                        break;
                    case TOKEN_LESS:
                        resultado.tipo = VALOR_BOOL;
                        resultado.booleano = (izq.numero < der.numero);
                        break;
                    case TOKEN_GREATER_EQUAL:
                        resultado.tipo = VALOR_BOOL;
                        resultado.booleano = (izq.numero >= der.numero);
                        break;
                    case TOKEN_LESS_EQUAL:
                        resultado.tipo = VALOR_BOOL;
                        resultado.booleano = (izq.numero <= der.numero);
                        break;
            
                    case TOKEN_AND:
                        resultado.tipo = VALOR_BOOL;
                        resultado.booleano = izq.booleano && der.booleano;
                        break;
                    case TOKEN_OR:
                        resultado.tipo = VALOR_BOOL;
                        resultado.booleano = izq.booleano || der.booleano;
                        break;
            
                    case TOKEN_AT: {  // concatenación: texto @ valor
                        char buffer[256];
            
                        if (izq.tipo == VALOR_CADENA && der.tipo == VALOR_CADENA) {
                            resultado.tipo = VALOR_CADENA;
                            resultado.cadena = malloc(strlen(izq.cadena) + strlen(der.cadena) + 1);
                            strcpy(resultado.cadena, izq.cadena);
                            strcat(resultado.cadena, der.cadena);
                        } else {
                            resultado.tipo = VALOR_CADENA;
                            resultado.cadena = malloc(256);
            
                            if (izq.tipo == VALOR_CADENA) {
                                strcpy(resultado.cadena, izq.cadena);
                            } else if (izq.tipo == VALOR_NUMERO) {
                                sprintf(resultado.cadena, "%.2f", izq.numero);
                            } else if (izq.tipo == VALOR_BOOL) {
                                sprintf(resultado.cadena, izq.booleano ? "true" : "false");
                            }
            
                            if (der.tipo == VALOR_CADENA) {
                                strcat(resultado.cadena, der.cadena);
                            } else if (der.tipo == VALOR_NUMERO) {
                                sprintf(buffer, "%.2f", der.numero);
                                strcat(resultado.cadena, buffer);
                            } else if (der.tipo == VALOR_BOOL) {
                                strcat(resultado.cadena, der.booleano ? "true" : "false");
                            }
                        }
            
                        break;
                    }
            
                    default:
                        fprintf(stderr, "Error: operador binario no soportado (%d)\n", op);
                        exit(1);
                }
            
                return resultado;
            }
            
        default:
            fprintf(stderr, " Nodo no manejado aun en eval.c (tipo %d)\n", nodo->tipo);
            exit(1);
    }
}

