
#ifndef REGEX_TO_DFA_H
#define REGEX_TO_DFA_H

#include <stdbool.h>
#include "nfa_to_dfa.h"
#include "utils.h"
#define MAX_STATES 256
#define MAX_TRANSITIONS 256
#define MAX_TOKEN_NAME 32

typedef struct {
    char nombre_token[MAX_TOKEN_NAME];
    char* regex;
} DefinicionToken;
 
typedef struct DFA DFA;
DFA compilar_regex(char* regex, int token_id);

FragmentoNFA parse_literal(const char* regex);
#endif
