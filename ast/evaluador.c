#include "evaluador.h"  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Variable* obtener_variable(Entorno* env, const char* nombre) {
    while (env) {
        Variable* var = env->variables;
        while (var) {
            if (strcmp(var->nombre, nombre) == 0) {
                // Devuelve la variable encontrada
                return var;  
            }
            var = var->siguiente;
        }
        // Si no la encontró, busca en el entorno anterior
        env = env->anterior;  
    }
    fprintf(stderr, "Error: variable '%s' no definida.\n", nombre);
    exit(1);
}

Valor eval(NodoAST* nodo, Entorno* env) {
    switch (nodo->tipo) {
        
        case NODO_BLOQUE: {
                Valor resultado;
                resultado.tipo = VALOR_NULO;
            
                for (int i = 0; i < nodo->bloque.cantidad; i++) {
                    resultado = eval(nodo->bloque.expresiones[i], env);
                }
            
                return resultado;
            }
                
            // Si no se encuentra la variable en el entorno actual
            fprintf(stderr, "Error: variable '%s' no declarada para asignacion (:=)\n", nodo->asignacion.nombre);
            exit(1);
        case NODO_IF: {
                Valor cond = eval(nodo->ifthen.condicion, env);
            
                if (cond.tipo != VALOR_BOOL) {
                    fprintf(stderr, "Error: la condicion del 'if' no es booleana.\n");
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
                    fprintf(stderr, "Error: el iterable debe ser un objeto o rango valido.\n");
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
                    fprintf(stderr, "Error: la condicion de 'while' debe ser un valor booleano.\n");
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
                
            case NODO_LITERAL: {
                return (Valor){.tipo = VALOR_NUMERO, .numero = nodo->literal.valor};
            }
            case NODO_LITERAL_BOOL: {
                return (Valor){.tipo = VALOR_BOOL, .booleano = nodo->literal_bool.valor};
            }
            case NODO_LITERAL_STRING: {
                Valor v = {.tipo = VALOR_CADENA};
                v.cadena = strdup(nodo->literal_string.valor);
                return v;
            }
            case NODO_VARIABLE: {
                Variable* var = obtener_variable(env, nodo->variable.nombre);
                return var->valor;
            }
            case NODO_LET: {
                Valor val = eval(nodo->let.valor, env);
                Entorno* nuevo = malloc(sizeof(Entorno));
                nuevo->variables = malloc(sizeof(Variable));
                nuevo->variables->nombre = strdup(nodo->let.nombre);
                nuevo->variables->valor = val;
                nuevo->variables->siguiente = NULL;
                nuevo->funciones = NULL;
                nuevo->anterior = env;
                return eval(nodo->let.cuerpo, nuevo);
            }
            case NODO_ASIGNACION: {
                Variable* var = obtener_variable(env, nodo->asignacion.nombre);
                var->valor = eval(nodo->asignacion.valor, env);
                return var->valor;
            }
            case NODO_PRINT: {
                Valor v = eval(nodo->print.expresion, env);
                printf("[Print] ");
                switch (v.tipo) {
                    case VALOR_NUMERO: printf("%.2f", v.numero); break;
                    case VALOR_BOOL: printf("%s", v.booleano ? "true" : "false"); break;
                    case VALOR_CADENA: printf("%s", v.cadena); break;
                    default: printf("(nulo)"); break;
                }
                printf("\n");
                return (Valor){.tipo = VALOR_NULO};
            }
            case NODO_FUNCION: {
                Funcion* f = malloc(sizeof(Funcion));
                f->nombre = strdup(nodo->funcion.nombre);
                f->parametros = nodo->funcion.parametros;
                f->cantidad_parametros = nodo->funcion.cantidad_parametros;
                f->cuerpo = nodo->funcion.cuerpo;
                f->siguiente = env->funciones;
                env->funciones = f;
                return (Valor){.tipo = VALOR_NULO};
            }
            case NODO_LLAMADA: {
                Funcion* f = obtener_funcion(env, nodo->llamada.nombre);
                if (nodo->llamada.cantidad != f->cantidad_parametros) {
                    fprintf(stderr, "Error: funcion '%s' esperaba %d argumentos, recibio %d\n",
                            f->nombre, f->cantidad_parametros, nodo->llamada.cantidad);
                    exit(1);
                }
                Entorno* nuevo = malloc(sizeof(Entorno));
                nuevo->variables = NULL;
                nuevo->funciones = NULL;
                nuevo->anterior = env;
                for (int i = 0; i < f->cantidad_parametros; i++) {
                    Variable* var = malloc(sizeof(Variable));
                    var->nombre = strdup(f->parametros[i]->variable.nombre);
                    var->valor = eval(nodo->llamada.argumentos[i], env);
                    var->siguiente = nuevo->variables;
                    nuevo->variables = var;
                }
    
                return eval(f->cuerpo, nuevo);
            }
    
            case NODO_BINARIO: {
                Valor izq = eval(nodo->binario.izquierdo, env);
                Valor der = eval(nodo->binario.derecho, env);
                TokenType op = nodo->binario.operador.type;
    
                Valor resultado;
                if (izq.tipo == VALOR_NULO || der.tipo == VALOR_NULO) {
                    fprintf(stderr, "Error: operacion binaria con valores nulos.\n");
                    exit(1);
                }
                if ((op == TOKEN_PLUS || op == TOKEN_MINUS || op == TOKEN_STAR || op == TOKEN_SLASH || op == TOKEN_POWER)
                    && (izq.tipo != VALOR_NUMERO || der.tipo != VALOR_NUMERO)) {
                    fprintf(stderr, "Error: operador aritmetico requiere numeros.\n");
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
                        resultado.booleano = izq.numero == der.numero;
                        break;
                    case TOKEN_NOT_EQUAL:
                        resultado.tipo = VALOR_BOOL;
                        resultado.booleano = izq.numero != der.numero;
                        break;
                    case TOKEN_GREATER:
                        resultado.tipo = VALOR_BOOL;
                        resultado.booleano = izq.numero > der.numero;
                        break;
                    case TOKEN_LESS:
                        resultado.tipo = VALOR_BOOL;
                        resultado.booleano = izq.numero < der.numero;
                        break;
                    case TOKEN_GREATER_EQUAL:
                        resultado.tipo = VALOR_BOOL;
                        resultado.booleano = izq.numero >= der.numero;
                        break;
                    case TOKEN_LESS_EQUAL:
                        resultado.tipo = VALOR_BOOL;
                        resultado.booleano = izq.numero <= der.numero;
                        break;
                    default:
                        fprintf(stderr, "Error: operador binario no soportado.\n");
                        exit(1);
                }
                return resultado;
            }
    
            
        default:
            fprintf(stderr, " Nodo no manejado aun en eval.c (tipo %d)\n", nodo->tipo);
            exit(1);
    }
}
Funcion* obtener_funcion(Entorno* env, const char* nombre) {
    while (env) {
        Funcion* funcion = env->funciones;
        while (funcion) {
            if (strcmp(funcion->nombre, nombre) == 0) {
                return funcion;  // Devuelve la función si la encuentra
            }
            funcion = funcion->siguiente;
        }
        env = env->anterior;  
    }
    fprintf(stderr, "Error: funcion '%s' no definida.\n", nombre);
    exit(1);
}
void liberar_variable(Variable* var) {
    while (var) {
        Variable* sig = var->siguiente;
        free(var->nombre);
        if (var->valor.tipo == VALOR_CADENA) {
            free(var->valor.cadena);
        } else if (var->valor.tipo == VALOR_OBJETO) {
            for (int i = 0; i < var->valor.lista.cantidad; i++) {
                if (var->valor.lista.valores[i].tipo == VALOR_CADENA) {
                    free(var->valor.lista.valores[i].cadena);
                }
            }
            free(var->valor.lista.valores);
        }
        free(var);
        var = sig;
    }
}

void liberar_entorno(Entorno* env) {
    while (env) {
        Entorno* anterior = env->anterior;
        liberar_variable(env->variables);
        free(env);
        env = anterior;
    }
}
