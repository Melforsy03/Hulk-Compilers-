#include "item.h"
#include <stdlib.h>
#include <stdio.h>

Item* create_item(Production* production, int pos, ContainerSet* lookaheads) {
    Item* item = malloc(sizeof(Item));
    item->production = production;
    item->pos = pos;
    item->lookaheads = lookaheads ? copy_containerset(lookaheads) : create_containerset();
    return item;
}

Item* next_item(Item* item) {
    if (item->pos < item->production->right_len) {
        Item* new_item = create_item(item->production, item->pos + 1, copy_containerset(item->lookaheads));
        return new_item;
    }
    return NULL;
}

int is_reduce_item(Item* item) 
{
    return item->pos >= item->production->right_len;
}

int compare_items(Item* a, Item* b) {
    if (a->pos != b->pos) return 0;
    if (a->production->left != b->production->left) return 0;
    if (a->production->right_len != b->production->right_len) return 0;

    for (int i = 0; i < a->production->right_len; ++i) {
        if (a->production->right[i] != b->production->right[i]) return 0;
    }

    return 1;
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
Symbol* get_next_symbol(Item* item) {
    if (item->pos < item->production->right_len) {
        return item->production->right[item->pos];
    }
    return NULL;
}