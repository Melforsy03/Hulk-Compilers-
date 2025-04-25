#include "parser.h"
#include "ast.h"
#include <string.h>

Node* parse() {
    // === let p = new Point ===
    Node* let_p = nuevo_nodo(NODE_LET);
    strcpy(let_p->nombre, "p");
    let_p->izq = nodo_var("Point");

    // === p.x := 42 ===
    Node* access_px = nuevo_nodo(NODE_ACCESS);
    strcpy(access_px->nombre, "x");
    access_px->izq = nodo_var("p");

    Node* assign_px = nuevo_nodo(NODE_COLON_ASSIGN);
    assign_px->izq = access_px;
    assign_px->der = nodo_numero(42);

    // === p.y := 100 ===
    Node* access_py = nuevo_nodo(NODE_ACCESS);
    strcpy(access_py->nombre, "y");
    access_py->izq = nodo_var("p");

    Node* assign_py = nuevo_nodo(NODE_COLON_ASSIGN);
    assign_py->izq = access_py;
    assign_py->der = nodo_numero(100);

    // === if (p.x == 42) then print(1) else print(0) ===
    Node* access_px2 = nuevo_nodo(NODE_ACCESS);
    strcpy(access_px2->nombre, "x");
    access_px2->izq = nodo_var("p");

    Node* cond = nodo_binop(NODE_EQ, access_px2, nodo_numero(42));

    Node* then_print = nuevo_nodo(NODE_PRINT);
    then_print->izq = nodo_numero(1);

    Node* else_print = nuevo_nodo(NODE_PRINT);
    else_print->izq = nodo_numero(0);

    Node* if_stmt = nuevo_nodo(NODE_IF);
    if_stmt->izq = cond;
    if_stmt->der = then_print;
    if_stmt->extra = else_print;

    // === Secuencia completa ===
    Node* seq1 = nuevo_nodo(NODE_BLOCK);
    seq1->izq = assign_px;
    seq1->der = assign_py;

    Node* seq2 = nuevo_nodo(NODE_BLOCK);
    seq2->izq = seq1;
    seq2->der = if_stmt;

    let_p->der = seq2;

    return let_p;
}
