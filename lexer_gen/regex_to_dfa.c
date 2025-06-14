// regex_to_dfa.c (v1 - soporte de clases y *, +, |)
// MUY SIMPLIFICADO - ideal para regex tipo identificadores, números, etc.
#include "regex_to_dfa.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// ---------- NFA DEFINITIONS ---------- //


static int id_estado = 0;

FragmentoNFA caracter(char c) {
    EstadoNFA* inicio = nuevo_estado();
    EstadoNFA* fin = nuevo_estado();
    inicio->transiciones[(int)c] = fin;
    return (FragmentoNFA){inicio, fin};
}

FragmentoNFA concatenar(FragmentoNFA f1, FragmentoNFA f2) {
    f1.fin->epsilon1 = f2.inicio;
    return (FragmentoNFA){f1.inicio, f2.fin};
}

FragmentoNFA alternar(FragmentoNFA f1, FragmentoNFA f2) {
    EstadoNFA* inicio = nuevo_estado();
    EstadoNFA* fin = nuevo_estado();
    inicio->epsilon1 = f1.inicio;
    inicio->epsilon2 = f2.inicio;
    f1.fin->epsilon1 = fin;
    f2.fin->epsilon1 = fin;
    return (FragmentoNFA){inicio, fin};
}

FragmentoNFA estrella(FragmentoNFA f) {
    EstadoNFA* inicio = nuevo_estado();
    EstadoNFA* fin = nuevo_estado();
    inicio->epsilon1 = f.inicio;
    inicio->epsilon2 = fin;
    f.fin->epsilon1 = f.inicio;
    f.fin->epsilon2 = fin;
    return (FragmentoNFA){inicio, fin};
}

FragmentoNFA mas(FragmentoNFA f) {
    EstadoNFA* inicio = nuevo_estado();
    EstadoNFA* fin = nuevo_estado();
    inicio->epsilon1 = f.inicio;
    f.fin->epsilon1 = f.inicio;
    f.fin->epsilon2 = fin;
    return (FragmentoNFA){inicio, fin};
}

FragmentoNFA clase(const char* chars) {
    FragmentoNFA out = caracter(chars[0]);
    for (int i = 1; chars[i]; i++) {
        out = alternar(out, caracter(chars[i]));
    }
    return out;
}

// ---------- PARSER REGEX A NFA ---------- //

FragmentoNFA parse_literal(const char* regex) {
    FragmentoNFA out = caracter(regex[0]);
    for (int i = 1; regex[i]; i++) {
        out = concatenar(out, caracter(regex[i]));
    }
    return out;
}

// ---------- NFA to DFA (simplificado) ---------- //
DFA compilar_regex(char* regex, int token_id) {
    FragmentoNFA nfa = parse_regex(regex);                // 1) Parseo completo de la regex
    nfa.fin->es_final = 1;                                // 2) Marco el estado final
    return convertir_nfa_a_dfa(nfa.inicio, token_id);     // 3) Convierto NFA → DFA
}