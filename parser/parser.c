#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#define RESET_COLOR   "\x1b[0m"
#define RED_COLOR     "\x1b[31m"
#define GREEN_COLOR   "\x1b[32m"
#define YELLOW_COLOR  "\x1b[33m"
#define BLUE_COLOR    "\x1b[34m"
#define CYAN_COLOR    "\x1b[36m"
#define GRAY_COLOR    "\x1b[90m"
#define RESET_COLOR   "\x1b[0m"
#define GREEN_COLOR   "\x1b[32m"
#define YELLOW_COLOR  "\x1b[33m"
#define MAGENTA_COLOR "\x1b[35m"
// Token actual del análisis
static Token* actual;
static NodoAST* parsear_print();
static NodoAST* parsear_let();
static NodoAST* parsear_expresion();
static NodoAST* parsear_primario();
static NodoAST* parsear_bloque() ;
static NodoAST* parsear_asignacion();
static NodoAST* parsear_llamada();
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

    NodoAST* argumento = parsear_expresion();

    exigir(TOKEN_RPAREN, "')'");

    NodoAST* nodo = malloc(sizeof(NodoAST));
    nodo->tipo = NODO_LLAMADA;
    nodo->linea = actual[-1].line;
    nodo->llamada.nombre = nombre;
    nodo->llamada.argumento = argumento;
    return nodo;
}

static NodoAST* parsear_termino() {
    NodoAST* izquierdo = parsear_primario();

    while (actual->type == TOKEN_STAR || actual->type == TOKEN_SLASH) {
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

static NodoAST* parsear_expresion() {
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

static NodoAST* parsear_primario() {
    if (coincidir(TOKEN_NUMBER)) {
        return crear_literal(atof(actual[-1].lexeme), actual[-1].line);
    }

    if (actual->type == TOKEN_IDENTIFIER) {
        Token identificador = *actual;
        Token siguiente = actual[1];
    
        if (siguiente.type == TOKEN_COLON_EQUAL) {
            return parsear_asignacion();
        } else if (siguiente.type == TOKEN_LPAREN) {
            return parsear_llamada();
        } else if (siguiente.type == TOKEN_ASSIGN) {
            fprintf(stderr, "[Error de sintaxis] Se esperaba ':=' para asignación en línea %d, no '='\n", actual->line);
            exit(1);
        } else {
            avanzar();
            return crear_variable(identificador.lexeme, identificador.line);
        }
    }
    
    if (coincidir(TOKEN_PRINT)) {
        actual--; 
        return parsear_print();
    }
    if (coincidir(TOKEN_LPAREN)) {
        NodoAST* expr = parsear_expresion();
        exigir(TOKEN_RPAREN, "')'");
        return expr;
    }
    if (coincidir (TOKEN_LET)){
        actual --;
        return parsear_let();
    }
    if (coincidir(TOKEN_LBRACE)) {
        actual--;
        return parsear_bloque();
    }
    
    fprintf(stderr, "[Error de sintaxis] Expresion invalida en línea %d: '%s'\n",
            actual->line, actual->lexeme);
    exit(1);
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
        } else {
            expr = parsear_expresion();
        }

        expresiones[cantidad++] = expr;

        if (!coincidir(TOKEN_SEMICOLON) && actual->type != TOKEN_RBRACE) {
            fprintf(stderr, "[Error de sintaxis] Se esperaba ';' o '}' después de una expresión\n");
            exit(1);
        }
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

    exigir(TOKEN_COLON_EQUAL, "':='");

    NodoAST* valor = parsear_expresion();

    NodoAST* nodo = malloc(sizeof(NodoAST));
    nodo->tipo = NODO_ASIGNACION;
    nodo->linea = actual[-1].line;
    nodo->asignacion.nombre = nombre;
    nodo->asignacion.valor = valor;
    return nodo;
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
        case NODO_BLOQUE:
            printf("%sBLOQUE%s:\n", CYAN_COLOR, RESET_COLOR);
            for (int i = 0; i < nodo->bloque.cantidad; i++) {
                imprimir_ast(nodo->bloque.expresiones[i], nivel + 1);
            }
            break;
        case NODO_ASIGNACION:
            printf("%sASIGNACION%s: %s :=\n", YELLOW_COLOR, RESET_COLOR, nodo->asignacion.nombre);
            imprimir_ast(nodo->asignacion.valor, nivel + 1);
            break;
        
        case NODO_LITERAL:
            printf("%sLITERAL%s: %.2f\n", GREEN_COLOR, RESET_COLOR, nodo->literal.valor);
            break;

        case NODO_VARIABLE:
            printf("%sVARIABLE%s: %s\n", YELLOW_COLOR, RESET_COLOR, nodo->variable.nombre);
            break;

        case NODO_BINARIO:
            printf("%sBINARIO%s: %s\n", BLUE_COLOR, RESET_COLOR, nodo->binario.operador.lexeme);
            imprimir_ast(nodo->binario.izquierdo, nivel + 1);
            imprimir_ast(nodo->binario.derecho, nivel + 1);
            break;

        case NODO_PRINT:
            printf("%sPRINT%s:\n", MAGENTA_COLOR, RESET_COLOR);
            imprimir_ast(nodo->print.expresion, nivel + 1);
            break;

        case NODO_LET:
            printf("%sLET%s: %s =\n", CYAN_COLOR, RESET_COLOR, nodo->let.nombre);
            imprimir_ast(nodo->let.valor, nivel + 1);
            for (int i = 0; i < nivel; i++) printf("  ");
            printf("%sEN%s:\n", CYAN_COLOR, RESET_COLOR);
            imprimir_ast(nodo->let.cuerpo, nivel + 1);
            break;
           ;
        case NODO_LLAMADA:
           printf("%sLLAMADA%s: %s(...)\n", BLUE_COLOR, RESET_COLOR, nodo->llamada.nombre);
           imprimir_ast(nodo->llamada.argumento, nivel + 1);
           break;
       
        default:
            printf("%s[ERROR]%s Tipo de nodo desconocido\n", RED_COLOR, RESET_COLOR);
            break;
        
    }
}

void liberar_ast(NodoAST* nodo) {
    if (!nodo) return;

    switch (nodo->tipo) {
        case NODO_LITERAL:
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
        default:
            break;
    }

    free(nodo);
}
