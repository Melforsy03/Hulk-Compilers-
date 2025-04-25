#ifndef AST_H
#define AST_H

typedef enum {
    NODE_PROGRAM,
    NODE_LET,
    NODE_FUNCTION_DEF,
    NODE_TYPE_DEF,
    NODE_INHERITS,
    NODE_ASSIGN,
    NODE_COLON_ASSIGN,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_RANGE,
    NODE_RETURN,
    NODE_PRINT,
    NODE_ADD,
    NODE_SUB,
    NODE_MUL,
    NODE_DIV,
    NODE_POW,
    NODE_EQ,
    NODE_NEQ,
    NODE_LT,
    NODE_LTE,
    NODE_GT,
    NODE_GTE,
    NODE_AND,
    NODE_OR,
    NODE_NOT,
    NODE_CALL,
    NODE_VAR,
    NODE_NUM,
    NODE_STRING,
    NODE_SELF,
    NODE_BASE,
    NODE_BLOCK,
    NODE_ACCESS
} NodeType;

typedef struct Node {
    NodeType tipo;
    struct Node* izq;
    struct Node* der;
    struct Node* extra;
    int valor;
    char nombre[64];
    char string_valor[256];
} Node;

// Constructores
Node* nuevo_nodo(NodeType tipo);
Node* nodo_numero(int valor);
Node* nodo_var(const char* nombre);
Node* nodo_binop(NodeType tipo, Node* izq, Node* der);

#endif
