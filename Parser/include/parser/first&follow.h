#ifndef FIRST_FOLLOW_H
#define FIRST_FOLLOW_H

#include "grammar.h"
#include "containerset.h"

// Calcula los conjuntos FIRST de todos los símbolos y producciones
ContainerSet** compute_firsts(Grammar* grammar);

// Calcula los conjuntos FOLLOW de todos los no terminales
ContainerSet** compute_follows(Grammar* grammar, ContainerSet** firsts);

// Función auxiliar para imprimir conjuntos FIRST o FOLLOW
void print_sets(Grammar* grammar, ContainerSet** sets, const char* title);

#endif
