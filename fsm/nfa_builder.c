// nfa_builder.c
#include "regex_parser.h"
#include <stdio.h>
#include <stdlib.h>

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

int id_global = 0;

EstadoNFA* nuevo_estado() {
    EstadoNFA* e = malloc(sizeof(EstadoNFA));
    e->id = id_global++;
    e->num_transiciones = 0;
    return e;
}

void agregar_transicion(EstadoNFA* origen, char simbolo, EstadoNFA* destino) {
    if (origen->num_transiciones >= MAX_TRANSICIONES) {
        fprintf(stderr, "Estado %d: demasiadas transiciones\n", origen->id);
        exit(1);
    }
    origen->transiciones[origen->num_transiciones].simbolo = simbolo;
    origen->transiciones[origen->num_transiciones].destino = destino;
    origen->num_transiciones++;
}

NFA construir_nfa(Nodo* nodo) {
    if (!nodo) exit(1);

    switch (nodo->tipo) {
        case REGEX_CHAR: {
            EstadoNFA* i = nuevo_estado();
            EstadoNFA* f = nuevo_estado();
            agregar_transicion(i, nodo->caracter, f);
            return (NFA){i, f};
        }
        case REGEX_ANY: {
            EstadoNFA* i = nuevo_estado();
            EstadoNFA* f = nuevo_estado();
            for (char c = 32; c < 127; c++) {
                agregar_transicion(i, c, f);
            }
            return (NFA){i, f};
        }
        case REGEX_CLASS: {
            EstadoNFA* i = nuevo_estado();
            EstadoNFA* f = nuevo_estado();
            for (char c = nodo->clase.desde; c <= nodo->clase.hasta; c++) {
                agregar_transicion(i, c, f);
            }
            return (NFA){i, f};
        }
        case REGEX_CONCAT: {
            NFA a = construir_nfa(nodo->binario.izq);
            NFA b = construir_nfa(nodo->binario.der);
            agregar_transicion(a.fin, '\0', b.inicio);
            return (NFA){a.inicio, b.fin};
        }
        case REGEX_ALT: {
            EstadoNFA* i = nuevo_estado();
            EstadoNFA* f = nuevo_estado();
            NFA a = construir_nfa(nodo->binario.izq);
            NFA b = construir_nfa(nodo->binario.der);
            agregar_transicion(i, '\0', a.inicio);
            agregar_transicion(i, '\0', b.inicio);
            agregar_transicion(a.fin, '\0', f);
            agregar_transicion(b.fin, '\0', f);
            return (NFA){i, f};
        }
        case REGEX_STAR: {
            EstadoNFA* i = nuevo_estado();
            EstadoNFA* f = nuevo_estado();
            NFA a = construir_nfa(nodo->unico);
            agregar_transicion(i, '\0', a.inicio);
            agregar_transicion(i, '\0', f);
            agregar_transicion(a.fin, '\0', a.inicio);
            agregar_transicion(a.fin, '\0', f);
            return (NFA){i, f};
        }
        case REGEX_PLUS: {
            NFA a = construir_nfa(nodo->unico);
            EstadoNFA* f = nuevo_estado();
            agregar_transicion(a.fin, '\0', a.inicio);
            agregar_transicion(a.fin, '\0', f);
            return (NFA){a.inicio, f};
        }
        case REGEX_OPTIONAL: {
            EstadoNFA* i = nuevo_estado();
            EstadoNFA* f = nuevo_estado();
            NFA a = construir_nfa(nodo->unico);
            agregar_transicion(i, '\0', a.inicio);
            agregar_transicion(i, '\0', f);
            agregar_transicion(a.fin, '\0', f);
            return (NFA){i, f};
        }
    }
    exit(1);
}

void imprimir_nfa(EstadoNFA* estado, int* visitados, int max) {
    if (!estado || visitados[estado->id]) return;
    visitados[estado->id] = 1;
    for (int i = 0; i < estado->num_transiciones; i++) {
        char c = estado->transiciones[i].simbolo;
        EstadoNFA* d = estado->transiciones[i].destino;
        printf("(q%d) -%s-> (q%d)\n", estado->id, c ? (char[2]){c, 0} : "E", d->id);
        imprimir_nfa(d, visitados, max);
    }
}
