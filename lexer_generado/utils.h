// utils.h
#ifndef UTILS_H
#define UTILS_H

#include "regex_parser.h"
#include "nfa_to_dfa.h"

// Crea un nuevo estado NFA
EstadoNFA* nuevo_estado();

// Agrega una transición a un estado DFA
int agregar_transicion(EstadoDFA* estado, char simbolo, int destino);
void agregar_epsilon(EstadoNFA* desde, EstadoNFA* hacia);

#endif
