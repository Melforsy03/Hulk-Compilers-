#include "item.h"
#include <stdlib.h>
#include <stdio.h>

Item* create_item(Production* production, int pos)
{
    Item* item = (Item*)malloc(sizeof(Item));
    item->production = production;
    item->pos = pos;
    return item;
}

Item* next_item(Item* item) 
{
    if (item->pos < item->production->right_len) 
        return create_item(item->production, item->pos + 1);
    
    return NULL; // No se puede avanzar más
}

int is_reduce_item(Item* item) 
{
    return item->pos >= item->production->right_len;
}

int compare_items(Item* a, Item* b) 
{
    return (a->production == b->production) && (a->pos == b->pos);
}

void print_item(Item* item) 
{
    printf("%s -> ", item->production->left->name);

    for (int i = 0; i < item->production->right_len; ++i) 
    {
        if (i == item->pos) printf(". ");
        printf("%s ", item->production->right[i]->name);
    }

    if (item->pos == item->production->right_len) printf(". ");

    printf("\n");
}

void free_item(Item* item) 
{
    if (item) 
        free(item);
}
