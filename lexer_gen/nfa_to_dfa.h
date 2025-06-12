// nfa_to_dfa.h
#ifndef NFA_TO_DFA_H
#define NFA_TO_DFA_H

#include "regex_parser.h"
#include "../lexer/lexer.h"
#include "regex_to_dfa.h"
#include "utils.h"
#define MAX_DFA_STATES 512
#define MAX_DFA_TRANSITIONS 128

typedef struct TransicionDFA {
    int destino;
    char simbolo;
} TransicionDFA;

typedef struct DFA {
    EstadoDFA estados[MAX_DFA_STATES];
    int cantidad_estados;
    int estado_inicial;
} DFA;

// Conversión NFA → DFA
DFA convertir_nfa_a_dfa(EstadoNFA* inicio_nfa, int token_id);

#endif

