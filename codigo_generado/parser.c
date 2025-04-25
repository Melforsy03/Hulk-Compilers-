#include "parser.h"
#include "ast.h"
#include <string.h>

Node* parse() {
    Node* cond = nodo_binop(NODE_EQ, nodo_numero(4), nodo_numero(5));

    Node* then_print = nuevo_nodo(NODE_PRINT);
    then_print->izq = nodo_numero(99);

    Node* else_print = nuevo_nodo(NODE_PRINT);
    else_print->izq = nodo_numero(11);

    Node* if_stmt = nuevo_nodo(NODE_IF);
    if_stmt->izq = cond;
    if_stmt->der = then_print;
    if_stmt->extra = else_print;

    return if_stmt;
}






