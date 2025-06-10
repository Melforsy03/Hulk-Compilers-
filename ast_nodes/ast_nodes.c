#include "ast_nodes.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

Node* create_node(Symbol* symbol, const char* lexeme, int child_count, Node** children) {
    Node* node = malloc(sizeof(Node));
    node->symbol = symbol;
    node->lexeme = lexeme ? strdup(lexeme) : NULL;
    
    node->child_count = child_count;
    node->children = child_count>0? malloc(child_count*sizeof(Node*)) : NULL;
    for(int i=0;i<child_count;i++) node->children[i]=children[i];
    node->row=node->column=0;
    return node;
}

static void _print(Node* node,int depth) {
    if(!node) return;
    printf("llego");
    for(int i=0;i<depth;i++) 
        printf("  ");
    printf("%s", node->symbol->name);
    if(node->lexeme) 
        printf("(%s)", node->lexeme);
    printf("\n");
    for(int i=0;i<node->child_count;i++) _print(node->children[i],depth+1);
}

static void _free(Node* node) {
    if(!node) return;
    for(int i=0;i<node->child_count;i++) _free(node->children[i]);
    free(node->children);
    free(node->lexeme);
    free(node);
}

void print_ast_root(Node* root) {
    _print(root,0);
    _free(root);
}