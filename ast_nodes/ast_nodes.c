#include "ast_nodes.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char* node_type_to_string(NodeType type);
static void _print_ast(Node* node, int depth);

Node* create_node(Symbol* symbol, const char* lexeme, int child_count, Node** children) {
    Node* node = malloc(sizeof(Node));
    node->symbol = symbol;
    node->lexeme = lexeme ? strdup(lexeme) : NULL;
    node->child_count = child_count;
    node->children = child_count>0 ? malloc(child_count*sizeof(Node*)) : NULL;
    for(int i=0; i<child_count; i++) node->children[i] = children[i];
    node->row = node->column = 0;
    return node;
}

void print_ast_ll1(Node* root) {
    if (!root) {
        printf("AST is empty\n");
        return;
    }
    printf("\n=== Abstract Syntax Tree ===\n");
    _print_ast(root, 0);
}

// En ast_nodes.c, modifica _print_ast así:
static void _print_ast(Node* node, int depth) {
    if (!node) return;
    printf("|");

    // Indentación
    for (int i = 0; i < depth; i++) printf("---");
    
    // Tipo de nodo
    printf("%s", node_type_to_string(node->tipo));
    
    // Información específica para diferentes tipos de nodos
    switch(node->tipo) {
        case NODE_NUMBER:
        case NODE_BOOLEAN:
            if (node->lexeme) printf(" (%s)", node->lexeme);
            break;
            
        case NODE_PLUS:
        case NODE_MINUS:
        case NODE_MULT:
        case NODE_DIV:
        case NODE_POW:
        case NODE_LESS:
        case NODE_GREATER:
        case NODE_LESS_EQUAL:
        case NODE_GREATER_EQUAL:
        case NODE_EQUAL:
        case NODE_NOT_EQUAL:
        case NODE_AND:
        case NODE_OR: {
            BinaryNode* bin = (BinaryNode*)node;
            printf(" [op: %s]", bin->operator ? bin->operator : "");
            break;
        }
            
        case NODE_NEGATIVE:
        case NODE_POSITIVE:
        case NODE_NOT: {
            UnaryNode* unary = (UnaryNode*)node;
            printf(" [op: %s]", unary->operator ? unary->operator : "");
            break;
        }
            
        case NODE_PROGRAM: {
            ProgramNode* prog = (ProgramNode*)node;
            break;
        }
    }
    
    printf("\n");
    
    // Recorrer hijos según tipo de nodo
    switch(node->tipo) {
        case NODE_PLUS:
        case NODE_MINUS:
        case NODE_MULT:
        case NODE_DIV:
        case NODE_POW:
        case NODE_LESS:
        case NODE_GREATER:
        case NODE_LESS_EQUAL:
        case NODE_GREATER_EQUAL:
        case NODE_EQUAL:
        case NODE_NOT_EQUAL:
        case NODE_AND:
        case NODE_OR: {
            BinaryNode* bin = (BinaryNode*)node;
            _print_ast(bin->left, depth + 1);
            _print_ast(bin->right, depth + 1);
            break;
        }
            
        case NODE_NEGATIVE:
        case NODE_POSITIVE:
        case NODE_NOT: {
            UnaryNode* unary = (UnaryNode*)node;
            _print_ast(unary->operand, depth + 1);
            break;
        }
            
        case NODE_PROGRAM: {
            ProgramNode* prog = (ProgramNode*)node;
            _print_ast(prog->expression, depth + 1);
            break;
        }
            
        default:
            // Recorrido normal para otros nodos
            for (int i = 0; i < node->child_count; i++) {
                _print_ast(node->children[i], depth + 1);
            }
    }
}

static const char* node_type_to_string(NodeType type) {
    switch(type) {
        case NODE_PROGRAM: return "Program";
        case NODE_NUMBER: return "Number";
        case NODE_STRING: return "String";
        case NODE_BINARY: return "BinaryOp";
        case NODE_UNARY: return "UnaryOp";
        case NODE_MULT: return "Mult";
        case NODE_PLUS: return "Plus";
        // Añade más casos según necesites
        default: return "Unknown";
    }
}

void free_ast_ll1(Node* node) {
    if (!node) return;
    for (int i = 0; i < node->child_count; i++) {
        free_ast_ll1(node->children[i]);
    }
    free(node->children);
    free(node->lexeme);
    free(node);
}