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

DFA convertir_nfa_a_dfa(EstadoNFA* inicio_nfa, int token_id) {
    DFA dfa;
    dfa.cantidad_estados = 0;

    ConjuntoEstados conjuntos[MAX_DFA_STATES];
    int num_conjuntos = 0;

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
    dfa.estados[0].id = -1;

    for (int i = 0; i < iniciales_count; i++) {
        if (iniciales[i]->es_final) {
            dfa.estados[0].es_final = 1;
            dfa.estados[0].id = token_id;
            break;
        }
    }

    num_conjuntos = 1;
    int sin_procesar = 1;

    for (int idx = 0; idx < sin_procesar; idx++) {
        EstadoDFA* estado_actual = &dfa.estados[idx];
        ConjuntoEstados* conjunto_actual = &conjuntos[idx];

        for (char c = 1; c < 127; c++) {
            EstadoNFA* movidos[512];
            int movidos_count = 0;

            mover(conjunto_actual->estados, conjunto_actual->cantidad, c, movidos, &movidos_count);
            if (movidos_count == 0) continue;

            EstadoNFA* cerrados[512];
            int cerrados_count = 0;
            int vis[1024] = {0};

            for (int i = 0; i < movidos_count; i++) {
                epsilon_closure(movidos[i], cerrados, &cerrados_count, vis);
            }

            int id_existente = buscar_conjunto_existente(conjuntos, num_conjuntos, cerrados, cerrados_count);
            int id_nuevo;

            if (id_existente >= 0) {
                agregar_transicion_dfa(estado_actual, c, id_existente);
            } else {
                id_nuevo = num_conjuntos;
                conjuntos[id_nuevo].estados = malloc(sizeof(EstadoNFA*) * cerrados_count);
                memcpy(conjuntos[id_nuevo].estados, cerrados, sizeof(EstadoNFA*) * cerrados_count);
                conjuntos[id_nuevo].cantidad = cerrados_count;

                EstadoDFA* nuevo_estado = &dfa.estados[dfa.cantidad_estados];
                nuevo_estado->id = estado_dfa_id_global++;
                nuevo_estado->num_transiciones = 0;
                nuevo_estado->es_final = 0;
                nuevo_estado->id = -1;

                for (int i = 0; i < cerrados_count; i++) {
                    if (cerrados[i]->es_final) {
                        nuevo_estado->es_final = 1;
                        nuevo_estado->id = token_id;
                        break;
                    }
                }

                agregar_transicion_dfa(estado_actual, c, id_nuevo);
                dfa.cantidad_estados++;
                num_conjuntos++;
                sin_procesar++;
            }
        }
    }

    return dfa;
}
