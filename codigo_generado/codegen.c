#include "codegen.h"
#include <stdio.h>
#include <string.h>

static int contador_temporales = 0;
static int contador_etiquetas = 0;

int nuevo_temp() {
    return contador_temporales++;
}

int nuevo_label() {
    return contador_etiquetas++;
}

// Nueva funcion auxiliar
int get_valor(Node* n) {
    if (!n) return -1;

    // Comparaciones ya devuelven i1
    if (n->tipo == NODE_EQ || n->tipo == NODE_NEQ ||
        n->tipo == NODE_LT || n->tipo == NODE_LTE ||
        n->tipo == NODE_GT || n->tipo == NODE_GTE) {
        return generar_codigo(n);
    }

    if (n->tipo == NODE_ACCESS) {
        int ptr = generar_codigo(n);
        int resultado = nuevo_temp();
        printf("  %%%d = load i32, i32* %%%d\n", resultado, ptr);
        return resultado;
    }

    return generar_codigo(n);
}

int generar_codigo(Node* n) {
    if (!n) return -1;

    switch (n->tipo) {
        case NODE_NUM: {
            int temp = nuevo_temp();
            printf("  %%%d = add i32 0, %d\n", temp, n->valor);
            return temp;
        }
        case NODE_VAR: {
            int temp = nuevo_temp();
            printf("  %%%d = load i32, i32* %%%s\n", temp, n->nombre);
            return temp;
        }
        case NODE_ASSIGN: {
            int valor = generar_codigo(n->der);
            printf("  store i32 %%%d, i32* %%%s\n", valor, n->nombre);
            return -1;
        }
        case NODE_ADD: {
            int izq = get_valor(n->izq);
            int der = get_valor(n->der);
            int temp = nuevo_temp();
            printf("  %%%d = add i32 %%%d, %%%d\n", temp, izq, der);
            return temp;
        }
        case NODE_SUB: {
            int izq = get_valor(n->izq);
            int der = get_valor(n->der);
            int temp = nuevo_temp();
            printf("  %%%d = sub i32 %%%d, %%%d\n", temp, izq, der);
            return temp;
        }
        case NODE_ACCESS: {
            int temp = nuevo_temp();
            int indice = (strcmp(n->nombre, "y") == 0) ? 1 : 0;
            printf("  %%%d = getelementptr %%Point, %%Point* %%%s, i32 0, i32 %d\n", temp, n->izq->nombre, indice);
            return temp;
        }
        case NODE_COLON_ASSIGN: {
            int ptr = generar_codigo(n->izq);
            int valor = get_valor(n->der);
            printf("  store i32 %%%d, i32* %%%d\n", valor, ptr);
            return -1;
        }
        case NODE_MUL: {
            int izq = get_valor(n->izq);
            int der = get_valor(n->der);
            int temp = nuevo_temp();
            printf("  %%%d = mul i32 %%%d, %%%d\n", temp, izq, der);
            return temp;
        }
        case NODE_DIV: {
            int izq = get_valor(n->izq);
            int der = get_valor(n->der);
            int temp = nuevo_temp();
            printf("  %%%d = sdiv i32 %%%d, %%%d\n", temp, izq, der);
            return temp;
        }
        case NODE_EQ: {
            int izq = get_valor(n->izq);
            int der = get_valor(n->der);
            int temp = nuevo_temp();
            printf("  %%%d = icmp eq i32 %%%d, %%%d\n", temp, izq, der);
            return temp;
        }
        case NODE_LT: {
            int izq = get_valor(n->izq);
            int der = get_valor(n->der);
            int temp = nuevo_temp();
            printf("  %%%d = icmp slt i32 %%%d, %%%d\n", temp, izq, der);
            return temp;
        }
        case NODE_LET: {
            if (strcmp(n->izq->nombre, "Point") == 0) {
                printf("  %%%s = alloca %%Point\n", n->nombre);
            } else {
                printf("  %%%s = alloca i32\n", n->nombre);
            }
        
            if (n->der) {
                int valor = generar_codigo(n->der); // genera el valor inicial
                printf("  store i32 %%%d, i32* %%%s\n", valor, n->nombre);
            }
            return -1;
        }
        
        case NODE_BLOCK: {
            generar_codigo(n->izq);
            generar_codigo(n->der);
            return -1;
        }
        case NODE_IF: {
            int cond = get_valor(n->izq);
            int label_then = nuevo_label();
            int label_else = nuevo_label();
            int label_end = nuevo_label();

            printf("  br i1 %%%d, label %%L%d, label %%L%d\n", cond, label_then, label_else);

            printf("L%d:\n", label_then);
            generar_codigo(n->der);
            printf("  br label %%L%d\n", label_end);

            printf("L%d:\n", label_else);
            if (n->extra)
                generar_codigo(n->extra);
            printf("  br label %%L%d\n", label_end);

            printf("L%d:\n", label_end);
            return -1;
        }
        case NODE_WHILE: {
            int label_start = nuevo_label();
            int label_body = nuevo_label();
            int label_end = nuevo_label();

            printf("  br label %%L%d\n", label_start);
            printf("L%d:\n", label_start);

            int cond = get_valor(n->izq);
            printf("  br i1 %%%d, label %%L%d, label %%L%d\n", cond, label_body, label_end);

            printf("L%d:\n", label_body);
            generar_codigo(n->der);
            printf("  br label %%L%d\n", label_start);

            printf("L%d:\n", label_end);
            return -1;
        }
        case NODE_NEQ: {  // Not Equal (!=)
            int izq = get_valor(n->izq);
            int der = get_valor(n->der);
            int temp = nuevo_temp();
            printf("  %%%d = icmp ne i32 %%%d, %%%d\n", temp, izq, der);
            return temp;
        }
        case NODE_GT: {  // Greater Than (>)
            int izq = get_valor(n->izq);
            int der = get_valor(n->der);
            int temp = nuevo_temp();
            printf("  %%%d = icmp sgt i32 %%%d, %%%d\n", temp, izq, der);
            return temp;
        }
        case NODE_GTE: { // Greater Than or Equal (>=)
            int izq = get_valor(n->izq);
            int der = get_valor(n->der);
            int temp = nuevo_temp();
            printf("  %%%d = icmp sge i32 %%%d, %%%d\n", temp, izq, der);
            return temp;
        }
        case NODE_LTE: { // Less Than or Equal (<=)
            int izq = get_valor(n->izq);
            int der = get_valor(n->der);
            int temp = nuevo_temp();
            printf("  %%%d = icmp sle i32 %%%d, %%%d\n", temp, izq, der);
            return temp;
        }
        case NODE_AND: { // Logical AND (&&)
            int izq = get_valor(n->izq);
            int der = get_valor(n->der);
        
            int temp = nuevo_temp();
            printf("  %%%d = and i1 %%%d, %%%d\n", temp, izq, der);
            return temp;
        }
        
        case NODE_OR: { // Logical OR (||)
            int izq = get_valor(n->izq);
            int der = get_valor(n->der);
        
            int temp = nuevo_temp();
            printf("  %%%d = or i1 %%%d, %%%d\n", temp, izq, der);
            return temp;
        }
        
        case NODE_NOT: { // Logical NOT (!)
            int valor = get_valor(n->izq);
        
            int temp = nuevo_temp();
            printf("  %%%d = xor i1 %%%d, true\n", temp, valor);
            return temp;
        }
        
        case NODE_PRINT: {
            int valor = get_valor(n->izq);
            printf("  call void @print(i32 %%%d)\n", valor);
            return -1;
        }
        default:
            printf("// [TODO] generación no implementada para tipo %d\n", n->tipo);
            return -1;
    }
}