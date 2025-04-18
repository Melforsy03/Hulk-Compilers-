#ifndef DFA_CONVERTER_H
#define DFA_CONVERTER_H

#include "nfa.h" 
#define MAX_ESTADOS_NFA 256
#define ALFABETO 128
#define MAX_ESTADOS_DFA 256
extern int total_dfa;
typedef struct {
    int count;
    EstadoNFA* estados[MAX_ESTADOS_NFA];
} Conjunto;

typedef struct {
    int id;
    Conjunto conjunto_estados;
    int transiciones[ALFABETO]; // destino por char
    int es_final;
} EstadoDFA;
typedef struct EstadoNFA EstadoNFA;
extern EstadoDFA dfa[MAX_ESTADOS_DFA];
void construir_dfa(EstadoNFA* inicio_nfa, EstadoNFA* fin_nfa);
void imprimir_dfa();


#endif
