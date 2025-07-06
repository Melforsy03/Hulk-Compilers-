// cst.c
#include "cst.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CSTNode* create_cst_node(const char* symbol) {
    CSTNode* node = malloc(sizeof(CSTNode));
    strcpy(node->symbol, symbol);
    node->num_children = 0;
    node->capacity = 4;  // Capacidad inicial, por ejemplo
    node->children = malloc(sizeof(CSTNode*) * node->capacity);
    node->token = NULL;
    return node;
}

void add_cst_child(CSTNode* parent, CSTNode* child) {
    if (parent->num_children >= parent->capacity) {
        parent->capacity *= 2;  // Duplicar capacidad
        parent->children = realloc(parent->children, sizeof(CSTNode*) * parent->capacity);
        if (!parent->children) {
            fprintf(stderr, "Error: realloc falló en add_cst_child\n");
            exit(1);
        }
    }
    parent->children[parent->num_children++] = child;
}

void free_cst(CSTNode* node) {
    for (int i = 0; i < node->num_children; ++i) {
        free_cst(node->children[i]);
    }
    free(node->children);
    free(node->token); // Solo si hiciste copia del token
    free(node);
}
void print_cst(const CSTNode* node, int indent) {
    if (!node) return;

    for (int i = 0; i < indent; ++i) printf("  ");

    printf("%s", node->symbol);

    if (node->token && node->token->lexema) {
        printf(" [token: %s]", node->token->lexema);
    }

    printf("\n");

    for (int i = 0; i < node->num_children; ++i) {
        print_cst(node->children[i], indent + 1);
    }
}
