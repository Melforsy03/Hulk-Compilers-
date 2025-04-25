// nfa_to_dfa.h
#ifndef NFA_TO_DFA_H
#define NFA_TO_DFA_H

#include "regex_parser.h"

#define MAX_DFA_STATES 512
#define MAX_DFA_TRANSITIONS 128

typedef struct {
    int destino;
    char simbolo;
} TransicionDFA;

typedef struct {
    int id;
    int num_transiciones;
    TransicionDFA transiciones[MAX_DFA_TRANSITIONS];
    int es_final;
    int token_id;
} EstadoDFA;

typedef struct {
    EstadoDFA estados[MAX_DFA_STATES];
    int cantidad_estados;
    int estado_inicial;
} DFA;

// Conversión NFA → DFA
DFA convertir_nfa_a_dfa(EstadoNFA* inicio_nfa, int token_id);

#endif
