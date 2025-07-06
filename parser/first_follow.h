#ifndef FIRST_FOLLOW_H
#define FIRST_FOLLOW_H

#include "grammar.h"

#define MAX_SET 2048
#define MAX_SYMBOLS 2048

typedef struct {
    char non_terminal[32];
    char first[MAX_SET][32];
    int num_first;

    char follow[MAX_SET][32];
    int num_follow;
} FirstFollowEntry;

typedef struct {
    FirstFollowEntry entries[MAX_SYMBOLS];
    int num_entries;
} FirstFollowTable;

// Asegúrate que las declaraciones coincidan con las definiciones
void init_first_follow_table(FirstFollowTable* table, const Grammar* g);
void compute_first(const Grammar* g, FirstFollowTable* table);
void compute_follow(const Grammar* g, FirstFollowTable* table);
void print_first_follow(const FirstFollowTable* table);

// Declaración de la función unificada para verificar si un símbolo es terminal
int is_symbol_terminal(const Grammar* g, const char* symbol); // <--- CORREGIDO
void add_to_set(char set[MAX_SET][32], int* size, const char* symbol); // <--- Asegurarse de que esté declarada
FirstFollowEntry* get_entry(FirstFollowTable* table, const char* non_terminal);

#endif