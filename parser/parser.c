#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#define CYAN "\x1b[36m"
#define YELLOW "\x1b[33m"
#define MAGENTA "\x1b[35m"
#define GREEN "\x1b[32m"
#define RED "\x1b[31m"
#define BLUE "\x1b[34m"
#define GRAY "\x1b[90m"
#define VIOLET "\x1b[95m"
#define LIGHT_RED "\x1b[91m"
#define LIGHT_CYAN "\x1b[96m"
#define LIGHT_YELLOW "\x1b[93m"
#define RESET "\x1b[0m"

// Token actual del análisis
static Token* actual;

// Avanzar al siguiente token
static void avanzar() {
    actual++;
}

// Coincide con un tipo específico de token
static int coincidir(TokenType tipo) {
    if (actual->type == tipo) {
        avanzar();
        return 1;
    }
    return 0;
}

// Exigir un token específico o lanzar error
static void exigir(TokenType tipo, const char* esperado) {
    if (!coincidir(tipo)) {
        fprintf(stderr, "[Error de sintaxis] Se esperaba %s en linea %d, se encontro '%s'\n",
                esperado, actual->line, actual->lexeme);
        exit(1);
    }
}

// Crear nodo literal
static NodoAST* crear_literal(double valor, int linea) {
    NodoAST* nodo = malloc(sizeof(NodoAST));
    nodo->tipo = NODO_LITERAL;
    nodo->linea = linea;
    nodo->literal.valor = valor;
    return nodo;
}

// Crear nodo variable
static NodoAST* crear_variable(const char* nombre, int linea) {
    NodoAST* nodo = malloc(sizeof(NodoAST));
    nodo->tipo = NODO_VARIABLE;
    nodo->linea = linea;
    nodo->variable.nombre = strdup(nombre);
    return nodo;
}
static NodoAST* parsear_print() {
    exigir(TOKEN_PRINT, "'Print'");
    exigir(TOKEN_LPAREN, "'('");
    NodoAST* expr = parsear_expresion();
    exigir(TOKEN_RPAREN, "')'");
    NodoAST* nodo = malloc(sizeof(NodoAST));
    nodo->tipo = NODO_PRINT;
    nodo->linea = actual[-1].line;
    nodo->print.expresion = expr;
    return nodo;
}
static NodoAST* parsear_llamada() {
    exigir(TOKEN_IDENTIFIER, "nombre de función");
    char* nombre = strdup(actual[-1].lexeme);

    exigir(TOKEN_LPAREN, "'('");

    // Lista dinámica de argumentos
    NodoAST** argumentos = NULL;
    int capacidad = 0;
    int cantidad = 0;

    if (!coincidir(TOKEN_RPAREN)) {
        do {
            if (cantidad >= capacidad) {
                capacidad = capacidad == 0 ? 4 : capacidad * 2;
                argumentos = realloc(argumentos, sizeof(NodoAST*) * capacidad);
            }
            argumentos[cantidad++] = parsear_expresion();
        } while (coincidir(TOKEN_COMMA));

        exigir(TOKEN_RPAREN, "')'");
    }

    NodoAST* nodo = malloc(sizeof(NodoAST));
    nodo->tipo = NODO_LLAMADA;
    nodo->linea = actual[-1].line;
    nodo->llamada.nombre = nombre;
    nodo->llamada.argumentos = argumentos;
    nodo->llamada.cantidad = cantidad;
    return nodo;
}

static NodoAST* parsear_termino() {
    NodoAST* izquierdo = parsear_potencia();

    while (actual->type == TOKEN_STAR || actual->type == TOKEN_SLASH) {
        Token op = *actual;
        avanzar();
        NodoAST* derecho = parsear_potencia();

        NodoAST* nodo = malloc(sizeof(NodoAST));
        nodo->tipo = NODO_BINARIO;
        nodo->linea = op.line;
        nodo->binario.izquierdo = izquierdo;
        nodo->binario.operador = op;
        nodo->binario.derecho = derecho;
        izquierdo = nodo;
    }

    return izquierdo;
}

static NodoAST* parsear_expresion() {
    return parsear_asignacion_set();
}

static NodoAST* parsear_suma() {
    NodoAST* izquierdo = parsear_termino();

    while (actual->type == TOKEN_PLUS || actual->type == TOKEN_MINUS) {
        Token op = *actual;
        avanzar();
        NodoAST* derecho = parsear_termino();

        NodoAST* nodo = malloc(sizeof(NodoAST));
        nodo->tipo = NODO_BINARIO;
        nodo->linea = op.line;
        nodo->binario.izquierdo = izquierdo;
        nodo->binario.operador = op;
        nodo->binario.derecho = derecho;
        izquierdo = nodo;
    }

    return izquierdo;
}
static NodoAST* parsear_concatenacion() {
    NodoAST* izquierdo = parsear_suma(); // menor precedencia debajo

    while (actual->type == TOKEN_AT) {
        Token op = *actual;
        avanzar();
        NodoAST* derecho = parsear_suma();

        NodoAST* nodo = malloc(sizeof(NodoAST));
        nodo->tipo = NODO_BINARIO;
        nodo->linea = op.line;
        nodo->binario.izquierdo = izquierdo;
        nodo->binario.operador = op;
        nodo->binario.derecho = derecho;
        izquierdo = nodo;
    }

    return izquierdo;
}
static NodoAST* parsear_comparacion() {
    NodoAST* izquierdo = parsear_concatenacion();  // menor precedencia

    while (
        actual->type == TOKEN_EQUAL_EQUAL ||
        actual->type == TOKEN_NOT_EQUAL ||
        actual->type == TOKEN_LESS ||
        actual->type == TOKEN_LESS_EQUAL ||
        actual->type == TOKEN_GREATER ||
        actual->type == TOKEN_GREATER_EQUAL
    ) {
        Token op = *actual;
        avanzar();
        NodoAST* derecho = parsear_concatenacion();

        NodoAST* nodo = malloc(sizeof(NodoAST));
        nodo->tipo = NODO_BINARIO;
        nodo->linea = op.line;
        nodo->binario.izquierdo = izquierdo;
        nodo->binario.operador = op;
        nodo->binario.derecho = derecho;
        izquierdo = nodo;
    }

    return izquierdo;
}
static NodoAST* parsear_while() {
    exigir(TOKEN_WHILE, "'while'");
    NodoAST* condicion = parsear_expresion();
    NodoAST* cuerpo = parsear_expresion(); 

    NodoAST* nodo = malloc(sizeof(NodoAST));
    nodo->tipo = NODO_WHILE;
    nodo->linea = condicion->linea;
    nodo->bucle_while.condicion = condicion;
    nodo->bucle_while.cuerpo = cuerpo;
    return nodo;
}
static NodoAST* parsear_for() {
    exigir(TOKEN_FOR, "'for'");
    exigir(TOKEN_LPAREN, "'('");

    exigir(TOKEN_IDENTIFIER, "nombre de variable");
    char* nombre = strdup(actual[-1].lexeme);

    exigir(TOKEN_IN, "'in'");

    NodoAST* iterable = parsear_expresion();

    exigir(TOKEN_RPAREN, "')'");

    NodoAST* cuerpo = parsear_expresion(); // puede ser bloque o expresión simple

    NodoAST* nodo = malloc(sizeof(NodoAST));
    nodo->tipo = NODO_FOR;
    nodo->linea = cuerpo->linea;
    nodo->bucle_for.variable = nombre;
    nodo->bucle_for.iterable = iterable;
    nodo->bucle_for.cuerpo = cuerpo;
    return nodo;
}
static NodoAST* parsear_primario() {
    NodoAST* expr = NULL;

    if (coincidir(TOKEN_NUMBER)) {
        expr = crear_literal(atof(actual[-1].lexeme), actual[-1].line);
    }
    else if (actual->type == TOKEN_IDENTIFIER) {
        Token identificador = *actual;
        Token siguiente = actual[1];

        if (siguiente.type == TOKEN_COLON_EQUAL) {
            return parsear_asignacion();
        } else if (siguiente.type == TOKEN_ASSIGN) {
            fprintf(stderr, "[Error de sintaxis] Se esperaba ':=' para asignación en línea %d, no '='\n", actual->line);
            exit(1);
        } else {
            avanzar();
            expr = crear_variable(identificador.lexeme, identificador.line);
        }
    }
    else if (coincidir(TOKEN_FUNCTION)) {
        actual--;
        return parsear_funcion();
    }
    else if (coincidir(TOKEN_PRINT)) {
        actual--; 
        return parsear_print();
    }
    else if (coincidir(TOKEN_LPAREN)) {
        expr = parsear_expresion();
        exigir(TOKEN_RPAREN, "')'");
    }
    else if (coincidir(TOKEN_NOT)) {
        NodoAST* operand = parsear_primario();
        NodoAST* nodo = malloc(sizeof(NodoAST));
        nodo->tipo = NODO_NOT;  
        nodo->linea = actual[-1].line;
        nodo->binario.operador.type = TOKEN_NOT;
        nodo->binario.izquierdo = operand;
        nodo->binario.derecho = NULL;
        expr = nodo;
    }    
    else if (coincidir (TOKEN_LET)) {
        actual--;
        return parsear_let();
    }
    else if (coincidir(TOKEN_LBRACE)) {
        actual--;
        return parsear_bloque();
    }
    else if (coincidir(TOKEN_STRING)) {
        NodoAST* nodo = malloc(sizeof(NodoAST));
        nodo->tipo = NODO_LITERAL_STRING;
        nodo->linea = actual[-1].line;
        nodo->literal_string.valor = strdup(actual[-1].lexeme);
        expr = nodo;
    }
    else if (coincidir(TOKEN_IF)) {
        actual--;
        return parsear_if();
    }
    else if (coincidir(TOKEN_WHILE)) {
        actual--;
        return parsear_while();
    }
    else if (coincidir(TOKEN_FOR)) {
        actual--;
        return parsear_for();
    }
    else if (coincidir(TOKEN_TYPE)) {
        actual--;
        return parsear_tipo();
    }
    else if (coincidir(TOKEN_TRUE)) {
        NodoAST* nodo = malloc(sizeof(NodoAST));
        nodo->tipo = NODO_LITERAL_BOOL;
        nodo->linea = actual[-1].line;
        nodo->literal_bool.valor = 1;
        expr = nodo;
    }
    else if (coincidir(TOKEN_FALSE)) {
        NodoAST* nodo = malloc(sizeof(NodoAST));
        nodo->tipo = NODO_LITERAL_BOOL;
        nodo->linea = actual[-1].line;
        nodo->literal_bool.valor = 0;
        expr = nodo;
    }
    else if (coincidir(TOKEN_BASE)) {
        exigir(TOKEN_LPAREN, "'('");
        exigir(TOKEN_RPAREN, "')'");
        NodoAST* nodo = malloc(sizeof(NodoAST));
        nodo->tipo = NODO_VARIABLE;
        nodo->linea = actual[-1].line;
        nodo->variable.nombre = strdup("base()");
        expr = nodo;
    }
    else if (coincidir(TOKEN_SELF)) {
        NodoAST* nodo = malloc(sizeof(NodoAST));
        nodo->tipo = NODO_VARIABLE;
        nodo->linea = actual[-1].line;
        nodo->variable.nombre = strdup("self");
        expr = nodo;
    }
    if (coincidir(TOKEN_NEW)) {
        exigir(TOKEN_IDENTIFIER, "nombre del tipo a instanciar");
        char* nombre_tipo = strdup(actual[-1].lexeme);
    
        exigir(TOKEN_LPAREN, "'('");
    
        NodoAST** argumentos = NULL;
        int cantidad = 0, capacidad = 0;
    
        if (!coincidir(TOKEN_RPAREN)) {
            do {
                if (cantidad >= capacidad) {
                    capacidad = capacidad == 0 ? 4 : capacidad * 2;
                    argumentos = realloc(argumentos, sizeof(NodoAST*) * capacidad);
                }
                argumentos[cantidad++] = parsear_expresion();
            } while (coincidir(TOKEN_COMMA));
            exigir(TOKEN_RPAREN, "')'");
        }
    
        NodoAST* nodo = malloc(sizeof(NodoAST));
        nodo->tipo = NODO_NEW;
        nodo->linea = actual[-1].line;
        nodo->nuevo.tipo_nombre = nombre_tipo;
        nodo->nuevo.argumentos = argumentos;
        nodo->nuevo.cantidad = cantidad;
    
        return nodo;
    }
    

    if (expr == NULL) {
        fprintf(stderr, "[Error de sintaxis] Expresión inválida en línea %d: '%s'\n",
                actual->line, actual->lexeme);
        exit(1);
    }

    // 🔁 Encadenamiento con '.' para pt.getX
    while (coincidir(TOKEN_DOT)) {
        exigir(TOKEN_IDENTIFIER, "nombre del miembro después de '.'");
        char* nombre = strdup(actual[-1].lexeme);

        NodoAST* acceso = malloc(sizeof(NodoAST));
        acceso->tipo = NODO_ACCESO;
        acceso->linea = actual[-1].line;
        acceso->acceso.objeto = expr;
        acceso->acceso.miembro = nombre;

        expr = acceso;
    }

    // 🔁 Llamadas como expr(...) → pt.getX(), base().x()
    while (coincidir(TOKEN_LPAREN)) {
        NodoAST** argumentos = NULL;
        int cantidad = 0, capacidad = 0;

        if (!coincidir(TOKEN_RPAREN)) {
            do {
                if (cantidad >= capacidad) {
                    capacidad = capacidad == 0 ? 4 : capacidad * 2;
                    argumentos = realloc(argumentos, sizeof(NodoAST*) * capacidad);
                }
                argumentos[cantidad++] = parsear_expresion();
            } while (coincidir(TOKEN_COMMA));
            exigir(TOKEN_RPAREN, "')'");
        }

        NodoAST* llamada = malloc(sizeof(NodoAST));
        llamada->tipo = NODO_LLAMADA;
        llamada->linea = actual[-1].line;
        llamada->llamada.nombre = NULL; 
        llamada->llamada.objeto = expr;  
        llamada->llamada.argumentos = argumentos;
        llamada->llamada.cantidad = cantidad;

        expr = llamada;
    }

    return expr;
}

static NodoAST* parsear_let() {
    exigir(TOKEN_LET, "'let'");
    exigir(TOKEN_IDENTIFIER, "nombre de variable");
    char* nombre = strdup(actual[-1].lexeme);

    exigir(TOKEN_ASSIGN, "'='");

    NodoAST* valor = parsear_expresion();

    exigir(TOKEN_IN, "'in'");

    NodoAST* cuerpo = parsear_expresion();

    NodoAST* nodo = malloc(sizeof(NodoAST));
    nodo->tipo = NODO_LET;
    nodo->linea = actual[-1].line;
    nodo->let.nombre = nombre;
    nodo->let.valor = valor;
    nodo->let.cuerpo = cuerpo;
    return nodo;
}

static NodoAST* parsear_bloque() {
    exigir(TOKEN_LBRACE, "'{'");

    NodoAST** expresiones = malloc(sizeof(NodoAST*) * 1024); // máximo 1024 expr
    int cantidad = 0;

    while (!coincidir(TOKEN_RBRACE)) {
        if (actual->type == TOKEN_EOF) {
            fprintf(stderr, "[Error de sintaxis] Bloque sin cerrar\n");
            exit(1);
        }

        if (coincidir(TOKEN_SEMICOLON)) continue;

        NodoAST* expr;

        if (coincidir(TOKEN_LET)) {
            actual--;
            expr = parsear_let();
        } else if (coincidir(TOKEN_PRINT)) {
            actual--;
            expr = parsear_print();
        } 
        else if (coincidir(TOKEN_FUNCTION)) {
            actual--;
            expr = parsear_funcion(); 
        }
        else if (actual->type == TOKEN_IDENTIFIER) {
            Token identificador = *actual;
            Token siguiente = actual[1];
        
            if (siguiente.type == TOKEN_COLON_EQUAL) {
                expr = parsear_asignacion();
            } else if (siguiente.type == TOKEN_LPAREN) {
                expr = parsear_llamada();
            } else if (siguiente.type == TOKEN_ASSIGN) {
                fprintf(stderr, "[Error de sintaxis] Se esperaba ':=' para asignación en línea %d, no '='\n", actual->line);
                exit(1);
            } else {
                avanzar();
                expr = crear_variable(identificador.lexeme, identificador.line);
            }
        }
        
        else {
            expr = parsear_expresion();
        }
       
        expresiones[cantidad++] = expr;

        // Solo exigir ';' si la expresión no es una función (ya la consume internamente)
        if (expr->tipo != NODO_FUNCION && !coincidir(TOKEN_SEMICOLON)) {
            if (actual->type != TOKEN_RBRACE) {
                fprintf(stderr, "[Error de sintaxis] Se esperaba ';' o '}' después de una expresión\n");
                exit(1);
            }
        }


        // Consumir el punto y coma si está presente
        coincidir(TOKEN_SEMICOLON);

  

    }

    NodoAST* nodo = malloc(sizeof(NodoAST));
    nodo->tipo = NODO_BLOQUE;
    nodo->linea = actual[-1].line;
    nodo->bloque.expresiones = expresiones;
    nodo->bloque.cantidad = cantidad;
    return nodo;
}

static NodoAST* parsear_asignacion() {
    exigir(TOKEN_IDENTIFIER, "nombre de variable");
    char* nombre = strdup(actual[-1].lexeme);

    if (strcmp(nombre, "self") == 0) {
        fprintf(stderr, "[Error de sintaxis] No se puede asignar directamente a 'self' en línea %d\n", actual[-1].line);
        exit(1);
    }
    exigir(TOKEN_COLON_EQUAL, "':='");

    NodoAST* valor = parsear_expresion();

    NodoAST* nodo = malloc(sizeof(NodoAST));
    nodo->tipo = NODO_ASIGNACION;
    nodo->linea = actual[-1].line;
    nodo->asignacion.nombre = nombre;
    nodo->asignacion.valor = valor;
    return nodo;
}

static NodoAST* parsear_funcion() {
    exigir(TOKEN_FUNCTION, "'function'");
    exigir(TOKEN_IDENTIFIER, "nombre de la función");

    char* nombre = strdup(actual[-1].lexeme);

    exigir(TOKEN_LPAREN, "'('");
    exigir(TOKEN_IDENTIFIER, "nombre del parámetro");

    char* parametro = strdup(actual[-1].lexeme);

    exigir(TOKEN_RPAREN, "')'");
    exigir(TOKEN_ARROW, "'=>'");

    NodoAST* cuerpo = parsear_expresion();

    exigir(TOKEN_SEMICOLON, "';'");

    NodoAST* nodo = malloc(sizeof(NodoAST));
    nodo->tipo = NODO_FUNCION;
    nodo->linea = actual[-1].line;
    nodo->funcion.nombre = nombre;
    nodo->funcion.parametro = parametro;
    nodo->funcion.cuerpo = cuerpo;

    return nodo;
}
static NodoAST* parsear_potencia() {
    NodoAST* izquierdo = parsear_primario();

    while (actual->type == TOKEN_POWER) {
        Token op = *actual;
        avanzar();
        NodoAST* derecho = parsear_primario(); 

        NodoAST* nodo = malloc(sizeof(NodoAST));
        nodo->tipo = NODO_BINARIO;
        nodo->linea = op.line;
        nodo->binario.izquierdo = izquierdo;
        nodo->binario.operador = op;
        nodo->binario.derecho = derecho;
        izquierdo = nodo;
    }

    return izquierdo;
}
static NodoAST* parsear_if() {
    exigir(TOKEN_IF, "'if'");
    NodoAST* condicion = parsear_expresion();
    exigir(TOKEN_THEN, "'then'");
    NodoAST* entonces = parsear_expresion();

    NodoAST* sino = NULL;

    if (coincidir(TOKEN_ELIF)) {
        actual--;
        sino = parsear_elif();  
    } else if (coincidir(TOKEN_ELSE)) {
        sino = parsear_expresion();
    } else {
        fprintf(stderr, "[Error de sintaxis] Se esperaba 'elif' o 'else' después de 'then'\n");
        exit(1);
    }

    NodoAST* nodo = malloc(sizeof(NodoAST));
    nodo->tipo = NODO_IF;
    nodo->linea = condicion->linea;
    nodo->ifthen.condicion = condicion;
    nodo->ifthen.entonces = entonces;
    nodo->ifthen.sino = sino;
    return nodo;
}

static NodoAST* parsear_elif() {
    exigir(TOKEN_ELIF, "'elif'");
    NodoAST* condicion = parsear_expresion();
    exigir(TOKEN_THEN, "'then'");
    NodoAST* entonces = parsear_expresion();

    NodoAST* sino = NULL;

    if (coincidir(TOKEN_ELIF)) {
        actual--;
        sino = parsear_elif();
    } else if (coincidir(TOKEN_ELSE)) {
        sino = parsear_expresion();
    } else {
        fprintf(stderr, "[Error de sintaxis] Se esperaba 'elif' o 'else' después de 'then'\n");
        exit(1);
    }

    NodoAST* nodo = malloc(sizeof(NodoAST));
    nodo->tipo = NODO_IF;
    nodo->linea = condicion->linea;
    nodo->ifthen.condicion = condicion;
    nodo->ifthen.entonces = entonces;
    nodo->ifthen.sino = sino;
    return nodo;
}
static NodoAST* parsear_logico_or() {
    NodoAST* izquierdo = parsear_logico_and();

    while (actual->type == TOKEN_OR) {
        Token op = *actual;
        avanzar();
        NodoAST* derecho = parsear_logico_and();

        NodoAST* nodo = malloc(sizeof(NodoAST));
        nodo->tipo = NODO_BINARIO;
        nodo->linea = op.line;
        nodo->binario.izquierdo = izquierdo;
        nodo->binario.operador = op;
        nodo->binario.derecho = derecho;
        izquierdo = nodo;
    }

    return izquierdo;
}
static NodoAST* parsear_logico_and() {
    NodoAST* izquierdo = parsear_comparacion();

    while (actual->type == TOKEN_AND) {
        Token op = *actual;
        avanzar();
        NodoAST* derecho = parsear_comparacion();

        NodoAST* nodo = malloc(sizeof(NodoAST));
        nodo->tipo = NODO_BINARIO;
        nodo->linea = op.line;
        nodo->binario.izquierdo = izquierdo;
        nodo->binario.operador = op;
        nodo->binario.derecho = derecho;
        izquierdo = nodo;
    }

    return izquierdo;
}
static NodoAST* parsear_tipo() {
    exigir(TOKEN_TYPE, "'type'");
    exigir(TOKEN_IDENTIFIER, "nombre del tipo");

    char* nombre_tipo = strdup(actual[-1].lexeme);

    char* padre_tipo = NULL;
    if (coincidir(TOKEN_INHERITS)) {
        exigir(TOKEN_IDENTIFIER, "nombre del tipo padre");
        padre_tipo = strdup(actual[-1].lexeme);
    }
    char** parametros = NULL;
    int cantidad_parametros = 0, capacidad_parametros = 0;

    if (coincidir(TOKEN_LPAREN)) {
        do {
            exigir(TOKEN_IDENTIFIER, "nombre del parámetro");
            if (cantidad_parametros >= capacidad_parametros) {
                capacidad_parametros = capacidad_parametros == 0 ? 4 : capacidad_parametros * 2;
                parametros = realloc(parametros, sizeof(char*) * capacidad_parametros);
            }
            parametros[cantidad_parametros++] = strdup(actual[-1].lexeme);
        } while (coincidir(TOKEN_COMMA));
        exigir(TOKEN_RPAREN, "')'");
    }

   
    exigir(TOKEN_LBRACE, "'{'");

    NodoAST** miembros = NULL;
    int cantidad = 0, capacidad = 0;

    while (!coincidir(TOKEN_RBRACE)) {
        exigir(TOKEN_IDENTIFIER, "nombre del miembro");
        char* nombre_miembro = strdup(actual[-1].lexeme);
    
        if (coincidir(TOKEN_LPAREN)) {
            
            char* parametro = NULL;
    
            if (!coincidir(TOKEN_RPAREN)) {
                exigir(TOKEN_IDENTIFIER, "nombre del parámetro");
                parametro = strdup(actual[-1].lexeme);
                exigir(TOKEN_RPAREN, "')'");
            }
    
            exigir(TOKEN_ARROW, "'=>'");
            NodoAST* cuerpo = parsear_expresion();
            coincidir(TOKEN_SEMICOLON);  
    
            NodoAST* metodo = malloc(sizeof(NodoAST));
            metodo->tipo = NODO_FUNCION;
            metodo->linea = actual[-1].line;
            metodo->funcion.nombre = nombre_miembro;
            metodo->funcion.parametro = parametro;
            metodo->funcion.cuerpo = cuerpo;
    
            if (cantidad >= capacidad) {
                capacidad = capacidad == 0 ? 4 : capacidad * 2;
                miembros = realloc(miembros, sizeof(NodoAST*) * capacidad);
            }
    
            miembros[cantidad++] = metodo;
    
        } else if (coincidir(TOKEN_ASSIGN)) {
           
            NodoAST* valor = parsear_expresion();
            coincidir(TOKEN_SEMICOLON);  // opcional
    
            NodoAST* nodo_attr = malloc(sizeof(NodoAST));
            nodo_attr->tipo = NODO_ATRIBUTO;
            nodo_attr->linea = actual[-1].line;
            nodo_attr->atributo.nombre = nombre_miembro;
            nodo_attr->atributo.valor = valor;
    
            if (cantidad >= capacidad) {
                capacidad = capacidad == 0 ? 4 : capacidad * 2;
                miembros = realloc(miembros, sizeof(NodoAST*) * capacidad);
            }
    
            miembros[cantidad++] = nodo_attr;
    
        } else {
            fprintf(stderr, "[Error de sintaxis] Se esperaba '=' o '=>' después de '%s' en línea %d\n",
                    nombre_miembro, actual->line);
            exit(1);
        }
    }
    

    NodoAST* nodo = malloc(sizeof(NodoAST));
    nodo->tipo = NODO_TIPO;
    nodo->tipo_decl.nombre = nombre_tipo;
    nodo->tipo_decl.padre = padre_tipo;  
    nodo->tipo_decl.miembros = miembros;
    nodo->tipo_decl.cantidad = cantidad;
    nodo->tipo_decl.parametros = parametros;
    nodo->tipo_decl.cantidad_parametros = cantidad_parametros;
    nodo->linea = actual[-1].line;
    
    return nodo;
    
    return nodo;
}

static NodoAST* parsear_asignacion_set() {
    NodoAST* izquierdo = parsear_logico_or(); 
    
    if (izquierdo->tipo == NODO_VARIABLE && strcmp(izquierdo->variable.nombre, "self") == 0) {
        fprintf(stderr, "[Error de sintaxis] No se puede asignar directamente a 'self' en línea %d\n", izquierdo->linea);
        exit(1);
    }

    if (coincidir(TOKEN_COLON_EQUAL)) {
        NodoAST* derecho = parsear_asignacion_set(); 

        NodoAST* nodo = malloc(sizeof(NodoAST));
        nodo->tipo = NODO_SET;
        nodo->linea = izquierdo->linea;
        nodo->set.destino = izquierdo;
        nodo->set.valor = derecho;

        return nodo;
    }
  
    return izquierdo;
}

// Función principal
NodoAST* parsear(Token* tokens) {
    actual = tokens;

    if (coincidir(TOKEN_LET)) {
        actual--;
        return parsear_let();
    }

    if (coincidir(TOKEN_PRINT)) {
        actual--;
        return parsear_print();
    }
    
    
    return parsear_expresion();
}

void imprimir_ast(NodoAST* nodo, int nivel) {
    if (!nodo) return;

    for (int i = 0; i < nivel; i++) printf("  ");

    switch (nodo->tipo) {
        
        case NODO_LITERAL:
            printf("%sLITERAL%s: %.2f\n", CYAN, RESET, nodo->literal.valor);
            break;

        case NODO_LITERAL_STRING:
            printf("%sCADENA%s: \"%s\"\n", CYAN, RESET, nodo->literal_string.valor);
            break;

        case NODO_LITERAL_BOOL:
            printf("%sBOOLEANO%s: %s\n", CYAN, RESET, nodo->literal_bool.valor ? "true" : "false");
            break;

        case NODO_VARIABLE:
            printf("%sVARIABLE%s: %s\n", YELLOW, RESET, nodo->variable.nombre);
            break;

        case NODO_BINARIO:
            printf("%sBINARIO%s: %s\n", MAGENTA, RESET, nombre_token(nodo->binario.operador.type));
            imprimir_ast(nodo->binario.izquierdo, nivel + 1);
            imprimir_ast(nodo->binario.derecho, nivel + 1);
            break;

        case NODO_NOT:
            printf("%sNEGACIÓN%s:\n", VIOLET, RESET);
            imprimir_ast(nodo->binario.izquierdo, nivel + 1);
            break;

        case NODO_LET:
            printf("%sLET%s: %s\n", GREEN, RESET, nodo->let.nombre);
            imprimir_ast(nodo->let.valor, nivel + 1);
            imprimir_ast(nodo->let.cuerpo, nivel + 1);
            break;

        case NODO_ASIGNACION:
            printf("%sASIGNACIÓN%s: %s :=\n", BLUE, RESET, nodo->asignacion.nombre);
            imprimir_ast(nodo->asignacion.valor, nivel + 1);
            break;

        case NODO_SET:
            printf("%sASIGNACIÓN A PROPIEDAD%s :=\n",BLUE , RESET);
            printf("DESTINO:\n");
            imprimir_ast(nodo->set.destino, nivel + 1);
            printf("VALOR:\n");
            imprimir_ast(nodo->set.valor, nivel + 1);
            break;

        case NODO_PRINT:
            printf("%sPRINT%s:\n", CYAN, RESET);
            imprimir_ast(nodo->print.expresion, nivel + 1);
            break;

        case NODO_IF:
            printf("%sIF%s:\n", LIGHT_CYAN, RESET);
            imprimir_ast(nodo->ifthen.condicion, nivel + 1);
            printf("THEN:\n");
            imprimir_ast(nodo->ifthen.entonces, nivel + 1);
            printf("ELSE:\n");
            imprimir_ast(nodo->ifthen.sino, nivel + 1);
            break;

        case NODO_WHILE:
            printf("%sWHILE%s:\n", BLUE, RESET);
            imprimir_ast(nodo->bucle_while.condicion, nivel + 1);
            printf("CUERPO:\n");
            imprimir_ast(nodo->bucle_while.cuerpo, nivel + 1);
            break;

        case NODO_FOR:
            printf("%sFOR%s: %s in\n", BLUE, RESET, nodo->bucle_for.variable);
            imprimir_ast(nodo->bucle_for.iterable, nivel + 1);
            printf("CUERPO:\n");
            imprimir_ast(nodo->bucle_for.cuerpo, nivel + 1);
            break;

        case NODO_FUNCION:
            printf("%sFUNCIÓN%s: %s(%s)\n", VIOLET, RESET, nodo->funcion.nombre,
                nodo->funcion.parametro ? nodo->funcion.parametro : "");
            imprimir_ast(nodo->funcion.cuerpo, nivel + 1);
            break;

        case NODO_LLAMADA:
            if (nodo->llamada.nombre) {
                printf("%sLLAMADA%s: %s(...)\n", CYAN, RESET, nodo->llamada.nombre);
            } else {
                printf("%sLLAMADA A EXPRESIÓN%s:\n", CYAN, RESET);
                imprimir_ast(nodo->llamada.objeto, nivel + 1);
            }
            for (int i = 0; i < nodo->llamada.cantidad; i++) {
                printf("ARG %d:\n", i + 1);
                imprimir_ast(nodo->llamada.argumentos[i], nivel + 2);
            }
            break;

        case NODO_ACCESO:
            printf("%sACCESO%s: .%s\n", LIGHT_YELLOW, RESET, nodo->acceso.miembro);
            imprimir_ast(nodo->acceso.objeto, nivel + 1);
            break;

        case NODO_TIPO:
            printf("%sTIPO%s: %s", MAGENTA, RESET, nodo->tipo_decl.nombre);
            if (nodo->tipo_decl.padre) printf(" inherits %s", nodo->tipo_decl.padre);
            printf("\n");
            for (int i = 0; i < nodo->tipo_decl.cantidad; i++) {
                imprimir_ast(nodo->tipo_decl.miembros[i], nivel + 1);
            }
            break;

        case NODO_ATRIBUTO:
            printf("%sATRIBUTO%s: %s =\n", GREEN, RESET, nodo->atributo.nombre);
            imprimir_ast(nodo->atributo.valor, nivel + 1);
            break;

        case NODO_NEW:
            printf("%sINSTANCIA%s: new %s(...)\n", LIGHT_CYAN, RESET, nodo->nuevo.tipo_nombre);
            for (int i = 0; i < nodo->nuevo.cantidad; i++) {
                printf("ARG %d:\n", i + 1);
                imprimir_ast(nodo->nuevo.argumentos[i], nivel + 2);
            }
            break;

        case NODO_BLOQUE:
            printf("%sBLOQUE%s:\n", GRAY, RESET);
            for (int i = 0; i < nodo->bloque.cantidad; i++) {
                imprimir_ast(nodo->bloque.expresiones[i], nivel + 1);
            }
            break;

                
        default:
            printf("%s[ERROR]%s Tipo de nodo desconocido\n", RED, RESET);
            break;
        
    }
}

void liberar_ast(NodoAST* nodo) {
    if (!nodo) return;

    switch (nodo->tipo) {
        case NODO_LITERAL:
            // nada que liberar
            break;

        case NODO_LITERAL_STRING:
            free(nodo->literal_string.valor);
            break;

        case NODO_VARIABLE:
            free(nodo->variable.nombre);
            break;

        case NODO_BINARIO:
            liberar_ast(nodo->binario.izquierdo);
            liberar_ast(nodo->binario.derecho);
            break;

        case NODO_LET:
            free(nodo->let.nombre);
            liberar_ast(nodo->let.valor);
            liberar_ast(nodo->let.cuerpo);
            break;

        case NODO_PRINT:
            liberar_ast(nodo->print.expresion);
            break;

        case NODO_ASIGNACION:
            free(nodo->asignacion.nombre);
            liberar_ast(nodo->asignacion.valor);
            break;

        case NODO_FUNCION:
            free(nodo->funcion.nombre);
            free(nodo->funcion.parametro);
            liberar_ast(nodo->funcion.cuerpo);
            break;

        case NODO_LLAMADA:
            free(nodo->llamada.nombre);
            for (int i = 0; i < nodo->llamada.cantidad; i++) {
                liberar_ast(nodo->llamada.argumentos[i]);
            }
            free(nodo->llamada.argumentos);
            break;
        

        case NODO_BLOQUE:
            for (int i = 0; i < nodo->bloque.cantidad; i++) {
                liberar_ast(nodo->bloque.expresiones[i]);
            }
            free(nodo->bloque.expresiones);
            break;

        case NODO_IF:
            liberar_ast(nodo->ifthen.condicion);
            liberar_ast(nodo->ifthen.entonces);
            liberar_ast(nodo->ifthen.sino);
            break;

        default:
            
            break;
    }

    free(nodo);
}
