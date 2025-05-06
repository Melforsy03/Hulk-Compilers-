#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "../ast_nodes/ast_nodes.h"
// Crear nodo string

// === Helpers de construcción de nodos ===

NumberNode* crear_number(const char* lex) {
    NumberNode* node = malloc(sizeof(NumberNode));
    node->base.base.base.base.tipo = NODE_NUMBER;
    node->base.lex = strdup(lex);
    return node;
}

StringNode* crear_string(const char* texto) {
    StringNode* node = malloc(sizeof(StringNode));
    node->base.base.base.base.tipo = NODE_STRING;
    node->base.lex = strdup(texto);
    return node;
}

VarNode* crear_var(const char* name) {
    VarNode* node = malloc(sizeof(VarNode));
    node->base.base.base.base.tipo = NODE_VAR;
    node->base.lex = strdup(name);
    return node;
}

VarDeclarationNode* crear_declaracion(const char* name, ExpressionNode* val) {
    VarDeclarationNode* decl = malloc(sizeof(VarDeclarationNode));
    decl->base.base.tipo = NODE_VAR;
    decl->name = strdup(name);
    decl->value = val;
    decl->type = NULL;
    return decl;
}

ConcatNode* crear_concat(ExpressionNode* izq, ExpressionNode* der) {
    ConcatNode* nodo = malloc(sizeof(ConcatNode));
    nodo->base.base.base.base.tipo = NODE_CONCAT;
    nodo->base.base.left = izq;
    nodo->base.base.right = der;
    return nodo;
}

CallFuncNode* crear_print(ExpressionNode* arg) {
    CallFuncNode* node = malloc(sizeof(CallFuncNode));
    node->base.base.base.tipo = NODE_CALL_FUNC;
    node->name = strdup("print");
    node->arguments = arg;
    return node;
}

// === Armado del programa completo ===

LetInNode* crear_let() {
    // let number = 42
    VarDeclarationNode* decl1 = crear_declaracion("number", (ExpressionNode*)crear_number("42"));
    
    // text = "The meaning of life is"
    VarDeclarationNode* decl2 = crear_declaracion("text", (ExpressionNode*)crear_string("The meaning of life is"));

    // Lista de declaraciones
    VarDeclarationNode** lista = malloc(sizeof(VarDeclarationNode*) * 3);
    lista[0] = decl1;
    lista[1] = decl2;
    lista[2] = NULL;

    // Concatenación text @ number
    ExpressionNode* expr_concat = (ExpressionNode*)crear_concat(
        (ExpressionNode*)crear_var("text"),
        (ExpressionNode*)crear_var("number")
    );

    // print(text @ number)
    ExpressionNode* imprimir = (ExpressionNode*)crear_print(expr_concat);

    LetInNode* let = malloc(sizeof(LetInNode));
    let->base.base.tipo = NODE_LET;
    let->variables = lista;
    let->body = imprimir;
    return let;
}

ProgramNode* crear_programa() {
    ProgramNode* prog = malloc(sizeof(ProgramNode));
    prog->base.tipo = NODE_PROGRAM;
    prog->declarations = NULL;
    prog->expression = (ExpressionNode*)crear_let();
    return prog;
}

