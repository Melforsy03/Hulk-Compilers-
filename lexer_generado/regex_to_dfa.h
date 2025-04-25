
#ifndef REGEX_TO_DFA_H
#define REGEX_TO_DFA_H

#include <stdbool.h>

#define MAX_STATES 256
#define MAX_TRANSITIONS 256
#define MAX_TOKEN_NAME 32

typedef struct {
    int destino;
    char simbolo;
} Transicion;

typedef struct {
    int id;
    bool final;
    int token_id;
    Transicion transiciones[MAX_TRANSITIONS];
    int cantidad_transiciones;
} EstadoDFA;

typedef struct {
    EstadoDFA estados[MAX_STATES];
    int cantidad_estados;
} DFA;

typedef struct {
    char nombre_token[MAX_TOKEN_NAME];
    char* regex;
} DefinicionToken;

DFA compilar_regex(char* regex, int token_id);
int agregar_transicion(EstadoDFA* estado, char simbolo, int destino);

#endif
