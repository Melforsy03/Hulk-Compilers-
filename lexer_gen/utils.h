// utils.h
#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

#define MAX_DFA_STATES 512
#define MAX_TRANSICIONES 128
extern int estado_id_global;
extern int estado_dfa_id_global;

// Estructura para transiciones DFA
typedef struct TransicionDFA {
    int destino;
    char simbolo;
} TransicionDFA;


// Estructura para estados DFA
typedef struct {
    int id;
    int num_transiciones;
    TransicionDFA transiciones[MAX_TRANSICIONES];
    int es_final;
    int tipo;  // Para identificar el tipo de token
} EstadoDFA;

// Estructura para el DFA completo
typedef struct {
    EstadoDFA estados[MAX_DFA_STATES];
    int cantidad_estados;
    int estado_inicial;
} DFA;

// Estructura para estados NFA
typedef struct EstadoNFA {
    int id;
    int es_final;
    struct EstadoNFA* epsilon1;
    struct EstadoNFA* epsilon2;
    struct EstadoNFA* transiciones[128];
} EstadoNFA;

// Estructura para fragmentos NFA
typedef struct {
    EstadoNFA* inicio;
    EstadoNFA* fin;
} FragmentoNFA;

// Funciones utilitarias
EstadoNFA* nuevo_estado();
int agregar_transicion(EstadoDFA* estado, char simbolo, int destino);
void liberar_nfa(EstadoNFA* estado);
void liberar_nfa_recursivo(EstadoNFA* estado, int* visitados);
void liberar_dfa(DFA* dfa);
void agregar_epsilon(EstadoNFA* desde, EstadoNFA* hacia);

#endif // UTILS_H