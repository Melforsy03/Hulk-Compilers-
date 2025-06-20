// utils.c
#include "utils.h"
#include <stdlib.h>
#include <stdio.h> 
#include <string.h>

 int estado_id_global = 0;

EstadoNFA* nuevo_estado() {
    EstadoNFA* e = calloc(1, sizeof(EstadoNFA));
    e->id = estado_id_global++;
    return e;
}

int agregar_transicion(EstadoDFA* estado, char simbolo, int destino) {
    int index = estado->num_transiciones;
    estado->transiciones[index].simbolo = simbolo;
    estado->transiciones[index].destino = destino;
    estado->num_transiciones++;
    return index;
}

// ... (funciones existentes) ...

void liberar_nfa(EstadoNFA* estado) {
    if (!estado) return;
    
    static int max_ids = 1024;
    int* visitados = calloc(max_ids, sizeof(int));
    liberar_nfa_recursivo(estado, visitados);
    free(visitados);
}

void liberar_nfa_recursivo(EstadoNFA* estado, int* visitados) {
    if (!estado || visitados[estado->id]) return;
    
    visitados[estado->id] = 1;
    
    liberar_nfa_recursivo(estado->epsilon1, visitados);
    liberar_nfa_recursivo(estado->epsilon2, visitados);
    
    free(estado);
}

void liberar_dfa(DFA* dfa) {
    // Liberar transiciones si fueron allocadas dinámicamente
    for (int i = 0; i < dfa->cantidad_estados; i++) {
        if (dfa->estados[i].transiciones) {
            free(dfa->estados[i].transiciones);
        }
    }
}
void agregar_epsilon(EstadoNFA* desde, EstadoNFA* hacia) {
    if (desde->epsilon1 == NULL)
        desde->epsilon1 = hacia;
    else if (desde->epsilon2 == NULL)
        desde->epsilon2 = hacia;
    else {
        fprintf(stderr, "Error: solo se permiten dos transiciones epsilon por estado\n");
        exit(1);
    }
}
