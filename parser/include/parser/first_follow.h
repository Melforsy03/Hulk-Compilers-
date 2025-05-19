#ifndef FIRST_FOLLOW_H
#define FIRST_FOLLOW_H

#include "grammar.h"
#include "containerset.h"

// Calcula los conjuntos FIRST de todos los símbolos y producciones
ContainerSet* compute_local_first(Grammar* grammar, ContainerSet** firsts, Symbol** alpha, int alpha_size);

// Calcula los conjuntos FOLLOW de todos los no terminales
ContainerSet** compute_firsts(Grammar* grammar);

// Función auxiliar para imprimir conjuntos FIRST o FOLLOW
void print_sets(Grammar* grammar, ContainerSet** sets, const char* title);

void free_sets(ContainerSet** sets, int count);

#endif
