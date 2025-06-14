// nfa_to_dfa.c
#include "nfa_to_dfa.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    EstadoNFA** estados;
    int cantidad;
} ConjuntoEstados;

static int estado_dfa_id_global = 0;

static void epsilon_closure(EstadoNFA* estado, EstadoNFA** set, int* count, int* visited) {
    if (visited[estado->id]) return;
    visited[estado->id] = 1;
    set[(*count)++] = estado;

    if (estado->epsilon1) epsilon_closure(estado->epsilon1, set, count, visited);
    if (estado->epsilon2) epsilon_closure(estado->epsilon2, set, count, visited);
}

static void mover(EstadoNFA** entrada, int entrada_count, char simbolo, EstadoNFA** resultado, int* resultado_count) {
    *resultado_count = 0;
    for (int i = 0; i < entrada_count; i++) {
        EstadoNFA* e = entrada[i];
        EstadoNFA* destino = e->transiciones[(int)simbolo];
        if (destino) resultado[(*resultado_count)++] = destino;
    }
}

static int conjuntos_son_iguales(EstadoNFA** a, int na, EstadoNFA** b, int nb) {
    if (na != nb) return 0;
    for (int i = 0; i < na; i++) {
        int encontrado = 0;
        for (int j = 0; j < nb; j++) {
            if (a[i]->id == b[j]->id) {
                encontrado = 1;
                break;
            }
        }
        if (!encontrado) return 0;
    }
    return 1;
}

static int buscar_conjunto_existente(ConjuntoEstados* conjuntos, int num_conjuntos, EstadoNFA** estados, int n) {
    for (int i = 0; i < num_conjuntos; i++) {
        if (conjuntos_son_iguales(conjuntos[i].estados, conjuntos[i].cantidad, estados, n)) {
            return i;
        }
    }
    return -1;
}

static void agregar_transicion_dfa(EstadoDFA* estado, char simbolo, int destino) {
    estado->transiciones[estado->num_transiciones].simbolo = simbolo;
    estado->transiciones[estado->num_transiciones].destino = destino;
    estado->num_transiciones++;
}


// --- Funciones corregidas ---
DFA convertir_nfa_a_dfa(EstadoNFA* inicio_nfa, int token_id) {
    DFA dfa;
    dfa.cantidad_estados = 0;

    ConjuntoEstados conjuntos[MAX_DFA_STATES];
    int num_conjuntos = 0;

    // Estado inicial
    EstadoNFA* iniciales[512];
    int iniciales_count = 0;
    int visitado[1024] = {0};
    epsilon_closure(inicio_nfa, iniciales, &iniciales_count, visitado);

    conjuntos[0].estados = malloc(sizeof(EstadoNFA*) * iniciales_count);
    memcpy(conjuntos[0].estados, iniciales, sizeof(EstadoNFA*) * iniciales_count);
    conjuntos[0].cantidad = iniciales_count;

    dfa.estados[0].id = estado_dfa_id_global++;
    dfa.estado_inicial = 0;
    dfa.cantidad_estados = 1;
    dfa.estados[0].num_transiciones = 0;
    dfa.estados[0].es_final = 0;
    dfa.estados[0].tipo = -1;

    // Verificar si es final
    for (int i = 0; i < iniciales_count; i++) {
        if (iniciales[i]->es_final) {
            dfa.estados[0].es_final = 1;
            dfa.estados[0].tipo = token_id;
            break;
        }
    }

    num_conjuntos = 1;
    int sin_procesar = 1;

    for (int idx = 0; idx < sin_procesar; idx++) {
        for (char c = 1; c < 127; c++) {
            EstadoNFA* movidos[512];
            int movidos_count = 0;
            mover(conjuntos[idx].estados, conjuntos[idx].cantidad, c, movidos, &movidos_count);

            if (movidos_count == 0) continue;

            EstadoNFA* cerrados[512];
            int cerrados_count = 0;
            int vis[1024] = {0};
            for (int i = 0; i < movidos_count; i++) {
                epsilon_closure(movidos[i], cerrados, &cerrados_count, vis);
            }

            int id_existente = buscar_conjunto_existente(conjuntos, num_conjuntos, cerrados, cerrados_count);

            if (id_existente >= 0) {
                agregar_transicion_dfa(&dfa.estados[idx], c, id_existente);
            } else {
                conjuntos[num_conjuntos].estados = malloc(sizeof(EstadoNFA*) * cerrados_count);
                memcpy(conjuntos[num_conjuntos].estados, cerrados, sizeof(EstadoNFA*) * cerrados_count);
                conjuntos[num_conjuntos].cantidad = cerrados_count;

                dfa.estados[num_conjuntos].id = estado_dfa_id_global++;
                dfa.estados[num_conjuntos].num_transiciones = 0;
                dfa.estados[num_conjuntos].es_final = 0;
                dfa.estados[num_conjuntos].tipo = -1;

                for (int i = 0; i < cerrados_count; i++) {
                    if (cerrados[i]->es_final) {
                        dfa.estados[num_conjuntos].es_final = 1;
                        dfa.estados[num_conjuntos].tipo = token_id;
                        break;
                    }
                }

                agregar_transicion_dfa(&dfa.estados[idx], c, num_conjuntos);
                dfa.cantidad_estados++;
                num_conjuntos++;
                sin_procesar++;
            }
        }
    }

    // Liberar memoria
    for (int i = 1; i < num_conjuntos; i++) {
        free(conjuntos[i].estados);
    }
    free(conjuntos[0].estados);

    return dfa;
}