// cst.h
#ifndef CST_H
#define CST_H

#include "lexer.h"
#define MAX_SYMBOL 128
typedef struct CSTNode {
    char symbol[32];
    struct CSTNode** children;
    int num_children;
    int capacity;
    Token* token;
} CSTNode;


CSTNode* create_cst_node(const char* symbol);
void add_cst_child(CSTNode* parent, CSTNode* child);
void free_cst(CSTNode* node);
void print_cst(const CSTNode* node, int indent);

#endif
