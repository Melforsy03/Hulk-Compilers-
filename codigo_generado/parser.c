#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "../ast_nodes/ast_nodes.h"
// Crear nodo string
StringNode* crear_string(const char* texto) {
    StringNode* str = malloc(sizeof(StringNode));
    memset(str, 0, sizeof(StringNode)); // Inicializar a cero
    
    str->base.base.base.base.tipo = NODE_STRING; // Asumiendo 2 niveles de herencia
    str->base.lex = strdup(texto);
    return str;
}

// Crear nodo variable
VarNode* crear_var(const char* name) {
    VarNode* var = malloc(sizeof(VarNode));
    memset(var, 0, sizeof(VarNode));
    
    var->base.base.base.base.tipo = NODE_VAR; // Misma estructura que StringNode
    var->base.lex = strdup(name);
    return var;
}

// Declaración de variable
VarDeclarationNode* crear_declaracion(const char* name, ExpressionNode* value) {
    VarDeclarationNode* decl = malloc(sizeof(VarDeclarationNode));
    memset(decl, 0, sizeof(VarDeclarationNode));
    
    decl->base.base.tipo = NODE_VAR; // Tipo específico para declaraciones
    decl->name = strdup(name);
    decl->value = value;
    decl->type = NULL;
    return decl;
}

// Nodo print (versión mejorada)
CallFuncNode* crear_print(ExpressionNode* arg) {
    CallFuncNode* call = malloc(sizeof(CallFuncNode));
    memset(call, 0, sizeof(CallFuncNode));
    
    call->base.base.base.tipo = NODE_CALL_FUNC;
    call->name = strdup("print");
    call->arguments = arg;
    return call;
}

// Crear let-in (versión mejorada)
LetInNode* crear_let_in() {
    // 1. Crear declaración: msg = "Hello World"
    VarDeclarationNode* decl = crear_declaracion(
        "msg", 
        (ExpressionNode*)crear_string("Hello World")
    );

    // 2. Lista de declaraciones (con tamaño dinámico correcto)
    VarDeclarationNode** lista = malloc(sizeof(VarDeclarationNode*) * 2);
    lista[0] = decl;
    lista[1] = NULL; // Marcador de fin

    // 3. Cuerpo: print(msg)
    ExpressionNode* cuerpo = (ExpressionNode*)crear_print(
        (ExpressionNode*)crear_var("msg")
    );

    // 4. Construir nodo let-in
    LetInNode* let = malloc(sizeof(LetInNode));
    memset(let, 0, sizeof(LetInNode));
    
    let->base.base.tipo = NODE_LET;
    let->variables = lista;
    let->body = cuerpo;

    return let;
}
ProgramNode* crear_programa() {
    ProgramNode* prog = malloc(sizeof(ProgramNode));
    prog->base.tipo = NODE_PROGRAM;
    prog->declarations = NULL;
    prog->expression = crear_let_in();
    return prog;
}


