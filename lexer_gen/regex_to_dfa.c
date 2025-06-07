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
    DFA dfa;
    memset(&dfa, 0, sizeof(DFA));

    if (regex[0] == '[') {
        // Clase de caracteres [a-zA-Z_]
        char buffer[128] = {0};
        int j = 0;
        for (int i = 1; regex[i] && regex[i] != ']'; i++) {
            if (regex[i+1] == '-' && regex[i+2] && regex[i+2] != ']') {
                for (char c = regex[i]; c <= regex[i+2]; c++)
                    buffer[j++] = c;
                i += 2;
            } else {
                buffer[j++] = regex[i];
            }
        }
        FragmentoNFA f = clase(buffer);
        f.fin->es_final = 1;
        DFA d;
        d.cantidad_estados = 0;
        EstadoDFA* s = &d.estados[d.cantidad_estados++];
        s->id = 0;
        for (int i = 0; i < j; i++)
            agregar_transicion(s, buffer[i], 1);
        EstadoDFA* s1 = &d.estados[d.cantidad_estados++];
        s1->id = 1;
        s1->es_final = true;
        s1->id = token_id;
        return d;
    }

    // Fallback: cadena literal
    int len = strlen(regex);
    for (int i = 0; i <= len; i++) {
        dfa.estados[i].id = i;
        dfa.estados[i].num_transiciones = 0;
        dfa.estados[i].es_final = false;
        dfa.estados[i].id = -1;
        dfa.cantidad_estados++;
    }
    for (int i = 0; i < len; i++) {
        agregar_transicion(&dfa.estados[i], regex[i], i + 1);
    }
    dfa.estados[len].es_final = true;
    dfa.estados[len].id = token_id;
    return dfa;
}
