// regex_parser.c (versión corregida que maneja '|', '(', ')', '*' y '+' correctamente)
#include "regex_parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static const char* ptr;
static int estado_id_global = 0;

static FragmentoNFA parse_expr();
static FragmentoNFA parse_term();
static FragmentoNFA parse_factor();
static FragmentoNFA parse_base();

static FragmentoNFA crear_trans(char c) {
    EstadoNFA* ini = nuevo_estado();
    EstadoNFA* fin = nuevo_estado();
    ini->transiciones[(int)c] = fin;
    return (FragmentoNFA){ini, fin};
}

static FragmentoNFA concatenar(FragmentoNFA f1, FragmentoNFA f2) {
    f1.fin->epsilon1 = f2.inicio;
    return (FragmentoNFA){f1.inicio, f2.fin};
}

static FragmentoNFA alternar(FragmentoNFA f1, FragmentoNFA f2) {
    EstadoNFA* ini = nuevo_estado();
    EstadoNFA* fin = nuevo_estado();
    ini->epsilon1 = f1.inicio;
    ini->epsilon2 = f2.inicio;
    f1.fin->epsilon1 = fin;
    f2.fin->epsilon1 = fin;
    return (FragmentoNFA){ini, fin};
}

static FragmentoNFA kleene(FragmentoNFA f) {
    EstadoNFA* ini = nuevo_estado();
    EstadoNFA* fin = nuevo_estado();
    ini->epsilon1 = f.inicio;
    ini->epsilon2 = fin;
    f.fin->epsilon1 = f.inicio;
    f.fin->epsilon2 = fin;
    return (FragmentoNFA){ini, fin};
}

static FragmentoNFA mas(FragmentoNFA f) {
    EstadoNFA* ini = nuevo_estado();
    EstadoNFA* fin = nuevo_estado();
    ini->epsilon1 = f.inicio;
    f.fin->epsilon1 = f.inicio;
    f.fin->epsilon2 = fin;
    return (FragmentoNFA){ini, fin};
}

static FragmentoNFA clase() {
    char conjunto[128] = {0};
    int j = 0;

    if (*ptr != '[') { fprintf(stderr, "Error: clase debe comenzar con [\n"); exit(1); }
    ptr++;

    while (*ptr && *ptr != ']') {
        if (*(ptr+1) == '-' && *(ptr+2) && *(ptr+2) != ']') {
            for (char c = *ptr; c <= *(ptr+2); c++) conjunto[j++] = c;
            ptr += 3;
        } else {
            conjunto[j++] = *ptr++;
        }
    }
    if (*ptr != ']') { fprintf(stderr, "Error: clase sin cierre ]\n"); exit(1); }
    ptr++;

    FragmentoNFA out = crear_trans(conjunto[0]);
    for (int i = 1; i < j; i++) {
        out = alternar(out, crear_trans(conjunto[i]));
    }
    return out;
}

static FragmentoNFA parse_base() {
    if (*ptr == '(') {
        ptr++;
        FragmentoNFA f = parse_expr();
        if (*ptr != ')') { fprintf(stderr, "Error: paréntesis no cerrado\n"); exit(1); }
        ptr++;
        return f;
    } else if (*ptr == '[') {
        return clase();
    } else if (*ptr == '\\') {
        ptr++;
        return crear_trans(*ptr++);
    } else if (*ptr && strchr("|)*+", *ptr) == NULL) {
        return crear_trans(*ptr++);
    } else {
        fprintf(stderr, "Error: carácter inesperado '%c'\n", *ptr);
        exit(1);
    }
}

static FragmentoNFA parse_factor() {
    FragmentoNFA frag = parse_base(); 
    while (*ptr == '*' || *ptr == '+' || *ptr == '?') {
        if (*ptr == '*') {
            ptr++;
            EstadoNFA* inicio = nuevo_estado();
            EstadoNFA* fin = nuevo_estado();
            agregar_epsilon(inicio, frag.inicio);
            agregar_epsilon(inicio, fin);
            agregar_epsilon(frag.fin, frag.inicio);
            agregar_epsilon(frag.fin, fin);
            frag.inicio = inicio;
            frag.fin = fin;
        } else if (*ptr == '+') {
            ptr++;
            EstadoNFA* inicio = nuevo_estado();
            EstadoNFA* fin = nuevo_estado();
            agregar_epsilon(inicio, frag.inicio);
            agregar_epsilon(frag.fin, frag.inicio);
            agregar_epsilon(frag.fin, fin);
            frag.inicio = inicio;
            frag.fin = fin;
        } else if (*ptr == '?') {
            ptr++;
            EstadoNFA* inicio = nuevo_estado();
            EstadoNFA* fin = nuevo_estado();
            agregar_epsilon(inicio, frag.inicio);
            agregar_epsilon(inicio, fin);
            agregar_epsilon(frag.fin, fin);
            frag.inicio = inicio;
            frag.fin = fin;
        }
    }
    
    
    return frag;
}

static FragmentoNFA parse_term() {
    FragmentoNFA left = parse_factor();
    while (*ptr && *ptr != '|' && *ptr != ')') {
        FragmentoNFA right = parse_factor();
        left = concatenar(left, right);
    }
    return left;
}

static FragmentoNFA parse_expr() {
    FragmentoNFA left = parse_term();
    while (*ptr == '|') {
        ptr++;
        FragmentoNFA right = parse_term();
        left = alternar(left, right);
    }
    return left;
}

FragmentoNFA parse_regex(const char* regex) {
    ptr = regex;
    FragmentoNFA f = parse_expr();
    f.fin->es_final = 1;
    return f;
}
