#include "first_follow.h"
#include <stdlib.h>
#include <stdio.h>

// Crea el conjunto FIRST para una secuencia de símbolos (alpha)
ContainerSet* compute_local_first(Grammar* grammar, ContainerSet** firsts, Symbol** alpha, int alpha_size) 
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
        int index = -1;
        for (int j = 0; j < grammar->symbol_count; ++j) 
        {
            if (symbol_equals(grammar->symbols[j], alpha[i])) 
            {
                index = j;
                break;
            }
        }

        if (index >= 0) 
        {
            ContainerSet* first_sym = firsts[index];
            containerset_update(first_alpha, first_sym);

            if (!first_sym->contains_epsilon) 
            {
                add_epsilon = 0;
                break;
            }
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
            ContainerSet* local_first = compute_local_first(grammar,firsts, p->right, p->right_len);

            changed |= containerset_hard_update(first_left, local_first);
            free_containerset(local_first);
        }
    }

    return firsts;
}

// Calcula FOLLOW para toda la gramática
ContainerSet** compute_follows(Grammar* grammar, ContainerSet** firsts) 
{
    if (!grammar || !firsts) {
        fprintf(stderr, "Error: Gramática o conjuntos FIRST nulos\n");
        return NULL;
    }

    // Verificar que el símbolo EOF existe
    if (!grammar->eof) {
        fprintf(stderr, "Error: Símbolo EOF no inicializado\n");
        return NULL;
    }

    int n = grammar->symbol_count;
    ContainerSet** follows = (ContainerSet**)malloc(sizeof(ContainerSet*) * n);
    if (!follows) {
        fprintf(stderr, "Error: Memoria insuficiente para follows\n");
        return NULL;
    }

    // Inicialización segura
    for (int i = 0; i < n; ++i) {
        follows[i] = create_containerset();
        if (!follows[i]) {
            fprintf(stderr, "Error: No se pudo crear conjunto FOLLOW para %s\n", 
                   grammar->symbols[i]->name);
            free_sets(follows, i);
            return NULL;
        }
    }

    // Paso 1: Añadir $ al símbolo inicial
    int start_found = 0;
    for (int i = 0; i < n; ++i) {
        if (grammar->symbols[i] == grammar->start_symbol) {
            add_symbol_to_set(follows[i], grammar->eof);
            start_found = 1;
            break;
        }
    }
    if (!start_found) {
        fprintf(stderr, "Error: Símbolo inicial no encontrado\n");
        free_sets(follows, n);
        return NULL;
    }

    int changed;
    do {
        changed = 0;
        for (int i = 0; i < grammar->production_count; ++i) {
            Production* p = grammar->productions[i];
            if (!p) continue;

            // Encontrar índice del símbolo izquierdo
            int A_index = -1;
            for (int k = 0; k < n; ++k) {
                if (grammar->symbols[k] == p->left) {
                    A_index = k;
                    break;
                }
            }
            if (A_index == -1) continue;

            for (int j = 0; j < p->right_len; ++j) {
                Symbol* B = p->right[j];
                if (!B || B->type != NON_TERMINAL) continue;

                // Encontrar índice de B
                int B_index = -1;
                for (int k = 0; k < n; ++k) {
                    if (grammar->symbols[k] == B) {
                        B_index = k;
                        break;
                    }
                }
                if (B_index == -1) continue;

                // Calcular FIRST de beta
                Symbol** beta = p->right + j + 1;
                int beta_len = p->right_len - j - 1;
                ContainerSet* first_beta = compute_local_first(grammar, firsts, beta, beta_len);
                if (!first_beta) continue;

                // Paso 2: Añadir FIRST(beta)-{epsilon} a FOLLOW(B)
                for (int t = 0; t < first_beta->size; ++t) {
                    if (first_beta->symbols[t]->type != EPSILON) {
                        changed |= add_symbol_to_set(follows[B_index], first_beta->symbols[t]);
                    }
                }

                // Paso 3: Si epsilon ∈ FIRST(beta), añadir FOLLOW(A) a FOLLOW(B)
                if (first_beta->contains_epsilon || beta_len == 0) {
                    changed |= containerset_update(follows[B_index], follows[A_index]);
                }

                free_containerset(first_beta);
            }
        }
    } while (changed);

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
