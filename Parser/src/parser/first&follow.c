#include "first_follow.h"
#include <stdlib.h>
#include <stdio.h>

// Crea el conjunto FIRST para una secuencia de símbolos (alpha)
ContainerSet* compute_local_first(ContainerSet** firsts, Symbol** alpha, int alpha_size) 
{
    ContainerSet* first_alpha = create_containerset();

    if (alpha_size == 0) 
    {
        set_epsilon(first_alpha);
        return first_alpha;
    }

    int add_epsilon = 1;
    for (int i = 0; i < alpha_size; i++) 
    {
        ContainerSet* first_sym = firsts[alpha[i]->type == EPSILON ? 0 : i]; // cuidado
        containerset_update(first_alpha, first_sym);

        if (!first_sym->contains_epsilon) 
        {
            add_epsilon = 0;
            break;
        }
    }

    if (add_epsilon) 
    {
        set_epsilon(first_alpha);
    }

    return first_alpha;
}

// Calcula FIRST para toda la gramática
ContainerSet** compute_firsts(Grammar* grammar) 
{
    int n = grammar->symbol_count;

    ContainerSet** firsts = (ContainerSet**)malloc(sizeof(ContainerSet*) * n);

    for (int i = 0; i < n; ++i) 
    {
        Symbol* sym = grammar->symbols[i];
        firsts[i] = create_containerset();

        if (sym->type == TERMINAL || sym->type == EPSILON || sym->type == EOF_SYM) 
        {
            add_symbol_to_set(firsts[i], sym);
        }
    }

    int changed = 1;
    while (changed) 
    {
        changed = 0;

        for (int i = 0; i < grammar->production_count; ++i) 
        {
            Production* p = grammar->productions[i];
            int left_index = -1;

            for (int j = 0; j < n; ++j) {
                if (grammar->symbols[j] == p->left) {
                    left_index = j;
                    break;
                }
            }

            ContainerSet* first_left = firsts[left_index];
            ContainerSet* local_first = compute_local_first(firsts, p->right, p->right_len);

            changed |= containerset_hard_update(first_left, local_first);
            free_containerset(local_first);
        }
    }

    return firsts;
}

// Calcula FOLLOW para toda la gramática
ContainerSet** compute_follows(Grammar* grammar, ContainerSet** firsts) 
{
    int n = grammar->symbol_count;
    ContainerSet** follows = (ContainerSet**)malloc(sizeof(ContainerSet*) * n);

    for (int i = 0; i < n; ++i) 
        follows[i] = create_containerset();

    // El símbolo inicial tiene $ en su FOLLOW
    for (int i = 0; i < n; ++i) 
        if (grammar->symbols[i] == grammar->start_symbol) 
        {
            add_symbol_to_set(follows[i], grammar->eof);
            break;
        }

    int changed = 1;
    while (changed) 
    {
        changed = 0;

        for (int i = 0; i < grammar->production_count; ++i) 
        {
            Production* p = grammar->productions[i];
            Symbol** symbols = p->right;
            int len = p->right_len;

            for (int j = 0; j < len; ++j) 
            {
                Symbol* B = symbols[j];

                if (B->type == NON_TERMINAL) 
                {
                    int B_index = -1;
                    int left_index = -1;

                    for (int k = 0; k < n; ++k) 
                    {
                        if (grammar->symbols[k] == B) B_index = k;
                        if (grammar->symbols[k] == p->left) left_index = k;
                    }

                    // beta = symbols after B
                    Symbol** beta = &symbols[j + 1];
                    int beta_len = len - (j + 1);

                    ContainerSet* first_beta = compute_local_first(firsts, beta, beta_len);
                    ContainerSet* follow_B = follows[B_index];

                    // FIRST(beta) - {ε} se agrega a FOLLOW(B)
                    for (int t = 0; t < first_beta->size; ++t) 
                    {
                        changed |= add_symbol_to_set(follow_B, first_beta->symbols[t]);
                    }

                    // Si FIRST(beta) contiene ε, agregamos FOLLOW(A) a FOLLOW(B)
                    if (first_beta->contains_epsilon || beta_len == 0) 
                    {
                        changed |= containerset_update(follow_B, follows[left_index]);
                    }

                    free_containerset(first_beta);
                }
            }
        }
    }

    return follows;
}

// Imprime conjuntos FIRST o FOLLOW
void print_sets(Grammar* grammar, ContainerSet** sets, const char* title) 
{
    printf("\n%s:\n", title);
    for (int i = 0; i < grammar->symbol_count; ++i) 
        if (grammar->symbols[i]->type == NON_TERMINAL) 
            print_containerset(sets[i], grammar->symbols[i]->name);
}

void free_sets(ContainerSet** sets, int count) 
{
    if (!sets) return;
    for (int i = 0; i < count; ++i) 
        if (sets[i]) free_containerset(sets[i]);

    free(sets);
}
