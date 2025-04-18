#ifndef NFA_H
#define NFA_H
#include "regex_parser.h"
#define MAX_TRANSICIONES 40

typedef struct EstadoNFA EstadoNFA;

struct EstadoNFA {
    int id;
    struct Transicion {
        char simbolo;
        EstadoNFA* destino;
    } transiciones[MAX_TRANSICIONES];
    int num_transiciones;
};

typedef struct {
    EstadoNFA* inicio;
    EstadoNFA* fin;
} NFA;
NFA construir_nfa(Nodo* nodo);
#endif
