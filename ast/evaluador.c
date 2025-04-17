#include "evaluador.h"  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Variable* obtener_variable(Entorno* env, const char* nombre) {
    while (env) {
        Variable* var = env->variables;
        while (var) {
            if (strcmp(var->nombre, nombre) == 0) {
                return var;  // Devuelve la variable encontrada
            }
            var = var->siguiente;
        }
        env = env->anterior;  // Si no la encontró, busca en el entorno anterior
    }
    fprintf(stderr, "Error: variable '%s' no definida.\n", nombre);
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
            // Buscar la variable en el entorno
            Variable* var = obtener_variable(env, nodo->variable.nombre);
            return var->valor;  // Retorna el valor de la variable
        }
        
        case NODO_LET: {
            Valor valor = eval(nodo->let.valor, env);
        
            // Crear un nuevo entorno para la variable 'let'
            Entorno* nuevo_entorno = malloc(sizeof(Entorno));
            nuevo_entorno->variables = malloc(sizeof(Variable));
            nuevo_entorno->variables->nombre = strdup(nodo->let.nombre);
            nuevo_entorno->variables->valor = valor;
            nuevo_entorno->variables->siguiente = env->variables;
            nuevo_entorno->anterior = env;
        
            // Evaluar el cuerpo del bloque en el nuevo entorno
            return eval(nodo->let.cuerpo, nuevo_entorno);
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
        case NODO_FOR: {
                Valor iterable = eval(nodo->bucle_for.iterable, env);
            
                if (iterable.tipo != VALOR_OBJETO) {
                    fprintf(stderr, "Error: el iterable debe ser un objeto o rango válido.\n");
                    exit(1);
                }
            
                // Crear un nuevo entorno para el bucle
                Entorno* nuevo_entorno = malloc(sizeof(Entorno));
                nuevo_entorno->variables = NULL;
                nuevo_entorno->anterior = env; // Apuntando al entorno anterior
            
                for (int i = 0; i < iterable.lista.cantidad; i++) {
                    Valor valor = iterable.lista.valores[i];
            
                    Variable* var_x = malloc(sizeof(Variable));
                    var_x->nombre = strdup(nodo->bucle_for.variable);
                    var_x->valor = valor;
                    var_x->siguiente = nuevo_entorno->variables;
                    nuevo_entorno->variables = var_x;
            
                    eval(nodo->bucle_for.cuerpo, nuevo_entorno);
            
                    // Limpiar el entorno después de cada iteración
                    nuevo_entorno->variables = nuevo_entorno->variables->siguiente;
                }
            
                Valor vacio;
                vacio.tipo = VALOR_NULO;
                return vacio;
            }
            
            
        case NODO_WHILE: {
                // Evaluar la condición
                Valor condicion = eval(nodo->bucle_while.condicion, env);
            
                if (condicion.tipo != VALOR_BOOL) {
                    fprintf(stderr, "Error: la condición de 'while' debe ser un valor booleano.\n");
                    exit(1);
                }
            
                // Mientras la condición sea verdadera, ejecutar el cuerpo
                while (condicion.booleano) {
                    eval(nodo->bucle_while.cuerpo, env);  // Evaluamos el cuerpo del bucle
            
                    // Volver a evaluar la condición después de cada iteración
                    condicion = eval(nodo->bucle_while.condicion, env);
                    if (condicion.tipo != VALOR_BOOL) {
                        fprintf(stderr, "Error: la condición de 'while' debe ser un valor booleano.\n");
                        exit(1);
                    }
                }
            
                // Retornar un valor vacío después del bucle
                Valor vacio;
                vacio.tipo = VALOR_NULO;
                return vacio;
            }
        case NODO_OBJETO: {
                Valor iterable;
                iterable.tipo = VALOR_OBJETO;
                iterable.lista.valores = nodo->objeto.valores;  // El arreglo de valores ya está en el nodo
                iterable.lista.cantidad = nodo->objeto.cantidad;  // La cantidad de valores
            
                return iterable;
            }
                
        case NODO_BINARIO: {
                Valor izq = eval(nodo->binario.izquierdo, env);
                Valor der = eval(nodo->binario.derecho, env);
                TokenType op = nodo->binario.operador.type;
            
                Valor resultado;
                if (izq.tipo == VALOR_NULO || der.tipo == VALOR_NULO) {
                    fprintf(stderr, "Error: operación binaria con valores nulos.\n");
                    exit(1);
                }
            
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

