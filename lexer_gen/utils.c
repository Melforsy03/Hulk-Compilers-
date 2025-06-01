// utils.c
#include "utils.h"
#include <stdlib.h>
#include <stdio.h> 
#include <string.h>

static int estado_id_global = 0;

char* my_strndup(const char* src, size_t n) {
    char* s = malloc(n + 1);
    if (!s) return NULL;
    strncpy(s, src, n);
    s[n] = '\0';
    return s;
}
EstadoNFA* nuevo_estado() {
    EstadoNFA* e = malloc(sizeof(EstadoNFA));
    e->id = estado_id_global++;
    e->es_final = 0;
    e->epsilon1 = e->epsilon2 = NULL;
    for (int i = 0; i < 128; i++) e->transiciones[i] = NULL;
    return e;
}

int agregar_transicion(EstadoDFA* estado, char simbolo, int destino) {
    int index = estado->num_transiciones;
    estado->transiciones[index].simbolo = simbolo;
    estado->transiciones[index].destino = destino;
    estado->num_transiciones++;
    return index;
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
