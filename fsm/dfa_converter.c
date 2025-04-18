#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "regex_parser.h"
#include "dfa_converter.h"

int total_dfa = 0;
EstadoDFA dfa[MAX_ESTADOS_DFA];

int contiene_estado(Conjunto* c, EstadoNFA* e) {
    for (int i = 0; i < c->count; i++) {
        if (c->estados[i] == e) return 1;
    }
    return 0;
}
void agregar_estado(Conjunto* c, EstadoNFA* e) {
    if (!contiene_estado(c, e)) {
        if (c->count < MAX_ESTADOS_NFA) {
            c->estados[c->count++] = e;
        } else {
            fprintf(stderr, "Error: Demasiados estados NFA en el conjunto\n");
            exit(EXIT_FAILURE);
        }
    }
}
void epsilon_closure(EstadoNFA* e, Conjunto* resultado) {
    if (!e || !resultado) return;
    agregar_estado(resultado, e);
    for (int i = 0; i < e->num_transiciones; i++) {
        if (e->transiciones[i].simbolo == '\0') {
            EstadoNFA* destino = e->transiciones[i].destino;
            if (!contiene_estado(resultado, destino)) {
                epsilon_closure(destino, resultado);
            }
        }
    }
}

void mover(Conjunto* origen, char c, Conjunto* destino) {
    for (int i = 0; i < origen->count; i++) {
        EstadoNFA* e = origen->estados[i];
        for (int j = 0; j < e->num_transiciones; j++) {
            if (e->transiciones[j].simbolo == c) {
                epsilon_closure(e->transiciones[j].destino, destino);
            }
        }
    }
}

int conjuntos_iguales(Conjunto* a, Conjunto* b) {
    if (a->count != b->count) return 0;
    for (int i = 0; i < a->count; i++) {
        if (!contiene_estado(b, a->estados[i])) return 0;
    }
    return 1;
}

int buscar_dfa_por_conjunto(Conjunto* c) {
    for (int i = 0; i < total_dfa; i++) {
        if (conjuntos_iguales(&dfa[i].conjunto_estados, c)) return i;
    }
    return -1;
}

Conjunto copiar_conjunto(Conjunto* src) {
    Conjunto nuevo = {0};
    for (int i = 0; i < src->count; i++) {
        nuevo.estados[i] = src->estados[i];
    }
    nuevo.count = src->count;
    return nuevo;
}

void construir_dfa(EstadoNFA* inicio_nfa, EstadoNFA* fin_nfa) {
    Conjunto inicial = {0};
    epsilon_closure(inicio_nfa, &inicial);
    if (total_dfa >= MAX_ESTADOS_DFA) {
        fprintf(stderr, "Error: Demasiados estados DFA\n");
        exit(EXIT_FAILURE);
    }
    dfa[0].id = 0;
    dfa[0].conjunto_estados = copiar_conjunto(&inicial);
    dfa[0].es_final = contiene_estado(&inicial, fin_nfa);
    memset(dfa[0].transiciones, -1, sizeof(dfa[0].transiciones));
    total_dfa = 1;

    for (int i = 0; i < total_dfa; i++) {
        for (char c = 32; c < 127; c++) {
            Conjunto nuevo = {0};
            mover(&dfa[i].conjunto_estados, c, &nuevo);
            if (nuevo.count == 0) continue;

            int j = buscar_dfa_por_conjunto(&nuevo);
            if (j == -1) {
                j = total_dfa++;
                dfa[j].id = j;
                dfa[j].conjunto_estados = copiar_conjunto(&nuevo);
                dfa[j].es_final = contiene_estado(&nuevo, fin_nfa);
                memset(dfa[j].transiciones, -1, sizeof(dfa[j].transiciones));
            }
            dfa[i].transiciones[(int)c] = j;
        }
    }
}
void imprimir_dfa() {
    for (int i = 0; i < total_dfa; i++) {
        printf("Estado DFA %d%s\n", i, dfa[i].es_final ? " (final)" : "");
        for (int c = 32; c < 127; c++) {
            int dest = dfa[i].transiciones[c];
            if (dest != -1) {
                printf("  '%c' -> %d\n", c, dest);
            }
        }
    }
}
