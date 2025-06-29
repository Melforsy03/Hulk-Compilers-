#include "../grammar/grammar.h"
#include "parser_table.h"
#include <stdio.h>
#include <stdlib.h>

ParseTable* build_parse_table(Grammar* g, ContainerSet** firsts, ContainerSet** follows) {
    // Inicializar tabla
    ParseTable* table = malloc(sizeof(ParseTable));
    table->non_term_count = g->nonterminals_count;
    table->term_count = g->terminals_count + 1;  // +1 para EOF

    table->entries = malloc(sizeof(TableEntry**) * table->non_term_count);
    for (int i = 0; i < table->non_term_count; i++) {
        table->entries[i] = calloc(table->term_count, sizeof(TableEntry*));
    }

    // Llenar tabla
    for (int i = 0; i < g->production_count; i++) {
        Production* p = g->productions[i];
        ContainerSet* first_alpha = compute_local_first(g, firsts, p->right, p->right_len);

        // Para cada terminal 'a' en FIRST(α)
        for (int j = 0; j < first_alpha->size; j++) {
            Symbol* a = first_alpha->symbols[j];
            if (a->type != EPSILON) {
                int nt_idx = get_non_term_index(g, p->left);
                int t_idx = get_term_index(g, a);
                table->entries[nt_idx][t_idx] = malloc(sizeof(TableEntry));
                table->entries[nt_idx][t_idx]->production = p;
            }
        }

        // Si ε ∈ FIRST(α), añadir para todo 'b' en FOLLOW(A)
        if (first_alpha->contains_epsilon) {
            int nt_idx = get_non_term_index(g, p->left);
            ContainerSet* follow_a = follows[nt_idx];
            for (int j = 0; j < follow_a->size; j++) {
                Symbol* b = follow_a->symbols[j];
                int t_idx = get_term_index(g, b);
                if (!table->entries[nt_idx][t_idx]) {
                    table->entries[nt_idx][t_idx] = malloc(sizeof(TableEntry));
                    table->entries[nt_idx][t_idx]->production = p;
                }
            }
        }
    }
    return table;
}

void print_parse_table(ParseTable* table, Grammar* g) {
    if (!table || !g) return;

    printf("\n=== Tabla de Parsing LL(1) ===\n");
    printf("%-15s", "NT/T");
    
    // Imprimir terminales (encabezados)
    for (int j = 0; j < g->terminals_count; j++) {
        printf("%-10s", g->terminals[j]->name);
    }
    printf("%-10s", "$");  // EOF
    
    printf("\n");

    // Imprimir filas (no terminales)
    for (int i = 0; i < table->non_term_count; i++) {
        printf("%-15s", g->nonterminals[i]->name);
        
        for (int j = 0; j < table->term_count; j++) {
            if (table->entries[i][j]) {
                printf("%-10s", table->entries[i][j]->production->right[0]->name); // Solo primer símbolo por simplicidad
            } else {
                printf("%-10s", "-");
            }
        }
        printf("\n");
    }
}

void free_parse_table(ParseTable* table) {
    if (!table) return;
    
    for (int i = 0; i < table->non_term_count; i++) {
        for (int j = 0; j < table->term_count; j++) {
            if (table->entries[i][j]) {
                free(table->entries[i][j]);
            }
        }
        free(table->entries[i]);
    }
    free(table->entries);
    free(table);
}