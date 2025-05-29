#include "production.h"
#include <stdio.h>
#include <stdlib.h>

Production* create_production(Symbol* left, Symbol** right, int right_len) 
{
    Production* p = (Production*)malloc(sizeof(Production));
    
    static int prod_counter = 0;
    p->left = left;
    p->right = (Symbol**)malloc(sizeof(Symbol*) * right_len);
    for (int i = 0; i < right_len; ++i) 
        p->right[i] = right[i];
    
    p->right_len = right_len;
    p->number = prod_counter++;

    return p;
}

void print_production(Production* p) 
{
    print_symbol(p->left);
    printf(" -> ");
    if (p->right_len == 0) 
        printf("epsilon");
    else 
        for (int i = 0; i < p->right_len; ++i) 
            printf("%s ", p->right[i]->name);
    
    printf("\n");
}

void free_production(Production* p) 
{
    if (p) 
    {
        free(p->right);
        free(p);
    }
}

