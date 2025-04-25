#include <stdlib.h>
#include <string.h>
#include "ast.h"

Node* nuevo_nodo(NodeType tipo) {
    Node* n = malloc(sizeof(Node));
    memset(n, 0, sizeof(Node));
    n->tipo = tipo;
    return n;
}

Node* nodo_numero(int valor) {
    Node* n = nuevo_nodo(NODE_NUM);
    n->valor = valor;
    return n;
}

Node* nodo_var(const char* nombre) {
    Node* n = nuevo_nodo(NODE_VAR);
    strncpy(n->nombre, nombre, sizeof(n->nombre));
    return n;
}

Node* nodo_binop(NodeType tipo, Node* izq, Node* der) {
    Node* n = nuevo_nodo(tipo);
    n->izq = izq;
    n->der = der;
    return n;
}
