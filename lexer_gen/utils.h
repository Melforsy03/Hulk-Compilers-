// utils.h
#ifndef UTILS_H
#define UTILS_H

#include "regex_parser.h"
#include "nfa_to_dfa.h"
#include <stddef.h>   // Para size_t
#include <stdlib.h>   // Para malloc, free, etc.
#include <string.h>   // Para strdup, memcpy, etc.
#include <stddef.h>
// Crea un nuevo estado NFA
EstadoNFA* nuevo_estado();
typedef struct EstadoDFA EstadoDFA;
// Agrega una transición a un estado DFA
int agregar_transicion(EstadoDFA* estado, char simbolo, int destino);
void agregar_epsilon(EstadoNFA* desde, EstadoNFA* hacia);
char* my_strndup(const char* src, size_t n);

#endif
