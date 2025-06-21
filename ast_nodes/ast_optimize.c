#include "ast_optimize.h"
#include "ast_nodes.h"
#include "parser/containerset.h"   
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int is_paren(const char* n) {
    return strcmp(n,"LPAREN")==0
        || strcmp(n,"RPAREN")==0
        || strcmp(n,"LBRACE")==0
        || strcmp(n,"RBRACE")==0
        || strcmp(n,"LBRACKET")==0
        || strcmp(n,"RBRACKET")==0
        || strcmp(n,"COMMA")==0
        || strcmp(n,"SEMICOLON")==0;
}

static const char* SKIP_NTS[] = {
    "Arithmetic","Term","Pow","Sign","Factor",
    "Expr_block","Non_empty_expr_list","Expr_item_list",
    "Check_type","Params","Params_list","Signature",
    "Atom", "Let_expr", "Expr_item_list", "Or_expr",
    "And_expr", "Aritm_comp", "Concat", "Expr",
    NULL
};
static int is_skip_nt(const char* n){
    for(int i=0; SKIP_NTS[i]; i++)
        if(strcmp(n,SKIP_NTS[i])==0) return 1;
    return 0;
}

static const char* BIN_NTS[] = {
    "Or_expr","And_expr","Aritm_comp",
    "Concat","Arithmetic","Term","Pow", 
    "Assignment", "Expr",
    NULL
};
static int is_bin_nt(const char* n){
    for(int i=0; BIN_NTS[i]; i++)
        if(strcmp(n,BIN_NTS[i])==0) return 1;
    return 0;
}

Symbol* lookup_symbol(const char* name) {
    static Symbol sym;
    sym.name = (char*)name;
    return &sym;
}

Node* optimize_ast(Node* node) {
    if (!node) return NULL;

    for (int i = 0; i < node->child_count; ++i)
        node->children[i] = optimize_ast(node->children[i]);

    if (is_paren(node->symbol->name) && node->child_count == 1) {
        Node* only = node->children[0];
        free(node->children);
        free(node);
        return only;
    }

    if (node->child_count == 1 && is_skip_nt(node->symbol->name)) {
        Node* only = node->children[0];
        free(node->children);
        free(node);
        return only;
    }

    if (is_bin_nt(node->symbol->name) && node->child_count == 3) {
        
        Node* left  = node->children[0];
        Node* op    = node->children[1];
        Node* right = node->children[2];

        Node** kids = malloc(2 * sizeof(Node*));
        kids[0] = left;
        kids[1] = right;

        node->symbol      = op->symbol;
        node->child_count = 2;
        free(node->children);
        node->children    = kids;
        free(op);
        return node;
    }

    // if (strcmp(node->symbol->name,"Conditional")==0) {

    //     Node* cond = node->children[0];
    //     Node* then_expr = node->children[1];
    //     Node* cc = node->children[2];  
    //     int cc_n = cc->child_count;
    //     Node** kids = malloc((2 + cc_n)*sizeof(Node*));
    //     kids[0] = cond;
    //     kids[1] = then_expr;

    //     for(int i=0;i<cc_n;i++) kids[2+i] = cc->children[i];
    //     free(cc->children);
    //     free(cc);
    //     free(node->children);
    //     node->children    = kids;
    //     node->child_count = 2 + cc_n;
    //     node->symbol = lookup_symbol("IF");
    //     return node;
    // }

    if (strcmp(node->symbol->name,"Method_signature")==0) {
        Node* idnode = node->children[0];
        Node* plist  = node->children[1];
        Node* ret    = node->child_count==3 ? node->children[2] : NULL;
        int p_n = plist->child_count;
        int r_n = ret ? ret->child_count : 0;
        Node** kids = malloc((1+p_n+r_n)*sizeof(Node*));
        kids[0] = idnode;
        for(int i=0;i<p_n;i++) kids[1+i] = plist->children[i];
        for(int i=0;i<r_n;i++) kids[1+p_n+i] = ret->children[i];
        if(ret){ free(ret->children); free(ret); }
        free(plist->children); free(plist);
        free(node->children);
        node->children    = kids;
        node->child_count = 1 + p_n + r_n;
        return node;
    }
    
    if (strcmp(node->symbol->name,"Call_func")==0) {
        Node* idnode = node->children[0];
        Node* args   = node->child_count==4 ? node->children[2] : NULL;
        int a_n = args ? args->child_count : 0;
        Node** kids = malloc((1+a_n)*sizeof(Node*));
        kids[0] = idnode;
        for(int i=0;i<a_n;i++) kids[1+i] = args->children[i];
        if(args){ free(args->children); free(args); }
        free(node->children);
        node->children    = kids;
        node->child_count = 1 + a_n;
        return node;
    }

    return node;
}