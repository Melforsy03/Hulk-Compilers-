#ifndef AST_H
#define AST_H
#include "cst.h"
typedef enum {
    // Operadores y expresiones
    AST_BINOP,            // Operador binario (+, -, *, /, etc.)
    AST_UNARYOP,          // Operador unario (-, !, etc.)
    AST_ASSIGN,           // Asignación (=)
    AST_IDENTIFIER,       // Identificador (variable)
    AST_NUMBER,           // Número literal
    AST_STRING,           // Cadena literal
    AST_BOOL,             // Literal booleano (true/false)
    AST_IS,
    AST_AS,
    // Control de flujo
    AST_IF,               // If expression
    AST_WHILE,            // While loop
    AST_FOR,              // For loop
    AST_CONDITION,        // Expresión de condición
    AST_BLOCK,            // Bloque de instrucciones { ... }

    // Funciones y declaraciones
    AST_FUNCTION_DECL,    // Declaración de función
    AST_FUNCTION_CALL,    // Llamada de función
    AST_PARAM_LIST,       // Lista de parámetros
    AST_ARGUMENT_LIST,    // Lista de argumentos

    // Programa y listas
    AST_PROGRAM,          // Nodo raíz del programa
    AST_STATEMENT_LIST,   // Lista de sentencias
    AST_LET,
    AST_VAR_DECL_LIST,
    AST_VAR_DECL,
    AST_ASSIGN_OP,
    AST_VECTOR,
     AST_TYPE_DECL,
     AST_INDEX,
    AST_MEMBER,
    AST_BASES,
     AST_INHERITS,
    AST_NEW_EXPR,
    AST_MEMBER_ACCESS,
    AST_TYPE_SPEC,
    // Otros
    AST_RETURN,           // Return statement
    AST_PRINT,            // Print statement
    AST_RANGE,            // Range o secuencia

    // Extiende con más tipos según tu gramática
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    char value[32];      // Nombre de variable, número, operador, etc.
    struct ASTNode** children;
    int num_children;
    int capacity;
} ASTNode;

// Creación y manipulación
ASTNode* create_ast_node(ASTNodeType type, const char* value);
void add_ast_child(ASTNode* parent, ASTNode* child);
void free_ast(ASTNode* node);
void print_ast(const ASTNode* node, int indent);
const char* node_type_to_str(ASTNodeType type);
ASTNode* parse_binop_chain(CSTNode* cst, const char* symbol);
ASTNode* parse_atom_suffixes(ASTNode* base, CSTNode* suffixes);
ASTNode* parse_atom_suffix(ASTNode* base, CSTNode* suffix);
void add_argument_list(ASTNode* parent, CSTNode* node) ;
// Transformación CST -> AST

ASTNode* cst_to_ast(CSTNode* cst);

#endif
