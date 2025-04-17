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
                    fprintf(stderr, "Error: el iterable debe ser un objeto (rango o lista).\n");
                    exit(1);
                }
            
                Valor resultado = {.tipo = VALOR_NULO};
            
                for (int i = 0; i < iterable.lista.cantidad; i++) {
                    Entorno* nuevo_entorno = malloc(sizeof(Entorno));
                    nuevo_entorno->variables = NULL;
                    nuevo_entorno->funciones = env->funciones; 
                    nuevo_entorno->anterior = env;
                    
                    Valor original = iterable.lista.valores[i];
                    Valor copia;

                    switch (original.tipo) {
                        case VALOR_NUMERO:
                            copia.tipo = VALOR_NUMERO;
                            copia.numero = original.numero;
                            copia.debe_liberarse = 0;
                            break;
                        case VALOR_BOOL:
                            copia.tipo = VALOR_BOOL;
                            copia.booleano = original.booleano;
                            copia.debe_liberarse = 0;
                            break;
                        case VALOR_CADENA:
                            copia.tipo = VALOR_CADENA;
                            copia.cadena = strdup(original.cadena);
                            copia.debe_liberarse = 1;
                            break;
                        case VALOR_OBJETO:
                            copia.tipo = VALOR_OBJETO;
                            copia.lista.cantidad = original.lista.cantidad;
                            copia.lista.valores = malloc(sizeof(Valor) * copia.lista.cantidad);
                            copia.debe_liberarse = 1;
                            for (int j = 0; j < copia.lista.cantidad; j++) {
                                Valor elem = original.lista.valores[j];
                                Valor copia_elem;
                                copia_elem.tipo = elem.tipo;
                                copia_elem.debe_liberarse = 0;
                                if (elem.tipo == VALOR_NUMERO) {
                                    copia_elem.numero = elem.numero;
                                } else if (elem.tipo == VALOR_BOOL) {
                                    copia_elem.booleano = elem.booleano;
                                } else if (elem.tipo == VALOR_CADENA) {
                                    copia_elem.cadena = strdup(elem.cadena);
                                    copia_elem.debe_liberarse = 1;
                                }
                                copia.lista.valores[j] = copia_elem;
                            }
                            break;
                        default:
                            copia.tipo = VALOR_NULO;
                            copia.debe_liberarse = 0;
                            break;
                    }

                    Variable* var = malloc(sizeof(Variable));
                    var->nombre = strdup(nodo->bucle_for.variable);
                    var->valor = copia;
                    var->siguiente = NULL;
            
                    nuevo_entorno->variables = var;
            
                    resultado = eval(nodo->bucle_for.cuerpo, nuevo_entorno);
            
                    // Liberar solo el entorno creado, sin liberar el global (env->anterior)
                    liberar_variable(nuevo_entorno->variables);
                    liberar_funciones(nuevo_entorno->funciones);
                    free(nuevo_entorno);

                }
            
                return resultado;
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
            case NODO_OBJETO_ITERABLE: {
                Valor iterable;
                iterable.tipo = VALOR_OBJETO;
                iterable.debe_liberarse = 0; 
                iterable.lista.valores = nodo->objeto.valores;
                iterable.lista.cantidad = nodo->objeto.cantidad;
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
    
                // Función nativa: range
                if (strcmp(f->nombre, "range") == 0 && f->cuerpo == NULL) {
                    if (nodo->llamada.cantidad != 2) {
                        fprintf(stderr, "Error: 'range' espera 2 argumentos.\n");
                        exit(1);
                    }
            
                    Valor inicio = eval(nodo->llamada.argumentos[0], env);
                    Valor fin = eval(nodo->llamada.argumentos[1], env);
            
                    if (inicio.tipo != VALOR_NUMERO || fin.tipo != VALOR_NUMERO) {
                        fprintf(stderr, "Error: argumentos de 'range' deben ser numéricos.\n");
                        exit(1);
                    }
            
                    int cantidad = (int)(fin.numero - inicio.numero);
                    if (cantidad < 0) cantidad = 0;
            
                    Valor* valores = malloc(sizeof(Valor) * cantidad);
                    for (int i = 0; i < cantidad; i++) {
                        valores[i].tipo = VALOR_NUMERO;
                        valores[i].numero = inicio.numero + i;
                    }
            
                    Valor resultado;
                    resultado.tipo = VALOR_OBJETO;
                    resultado.lista.valores = valores;
                    resultado.lista.cantidad = cantidad;
                    return resultado;
                }
           
                if (nodo->llamada.cantidad != f->cantidad_parametros) {
                    fprintf(stderr, "Error: función '%s' esperaba %d argumentos, recibió %d\n",
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
                    case TOKEN_AND:
                        resultado.tipo = VALOR_BOOL;
                        resultado.booleano = izq.booleano && der.booleano;
                        break;
                    case TOKEN_OR:
                        resultado.tipo = VALOR_BOOL;
                        resultado.booleano = izq.booleano || der.booleano;
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
        Variable* siguiente = var->siguiente;
        if (var->nombre) {
            free(var->nombre);
            var->nombre = NULL;
        }        
        if (var->valor.tipo == VALOR_OBJETO && var->valor.debe_liberarse) {
            if (var->valor.lista.valores != NULL) {
                for (int i = 0; i < var->valor.lista.cantidad; i++) {
                    if (var->valor.lista.valores[i].tipo == VALOR_CADENA &&
                        var->valor.lista.valores[i].debe_liberarse) {
                        free(var->valor.lista.valores[i].cadena);
                        var->valor.lista.valores[i].cadena = NULL;
                    }
                }
                if (var->valor.lista.valores) {
                    free(var->valor.lista.valores);
                    var->valor.lista.valores = NULL;
                }
                
            }
        }
        free(var);
        var = siguiente;
    }
}

void liberar_funciones(Funcion* f) {
    while (f) {
        Funcion* sig = f->siguiente;
        free(f->nombre);
        free(f);
        f = sig;
    }
}

void liberar_entorno(Entorno* env) {
    if (env == NULL) return;

    
    while (env->anterior != NULL) {
        Entorno* anterior = env->anterior;
        liberar_variable(env->variables);
        liberar_funciones(env->funciones);
        free(env);
        env = anterior;
    }
}
