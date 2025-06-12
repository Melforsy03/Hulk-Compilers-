#ifndef FIRST_FOLLOW_H
#define FIRST_FOLLOW_H

#include "../grammar/grammar.h"
#include "containerset.h"

ContainerSet* compute_local_first(Grammar* grammar, ContainerSet** firsts, Symbol** alpha, int alpha_size);

ContainerSet** compute_firsts(Grammar* grammar);

ContainerSet** compute_follows(Grammar* grammar, ContainerSet** firsts); 

void print_sets(Grammar* grammar, ContainerSet** sets, const char* title);

void free_sets(ContainerSet** sets, int count);

#endif
