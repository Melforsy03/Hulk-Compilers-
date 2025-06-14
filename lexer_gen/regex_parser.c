// regex_parser.c (versión corregida que maneja '|', '(', ')', '*' y '+' correctamente)
#include "regex_parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "utils.h"
static const char* ptr;
static int estado_id_global = 0;

static FragmentoNFA parse_expr();
static FragmentoNFA parse_term();
static FragmentoNFA parse_factor();
static FragmentoNFA parse_base();
static char manejar_escape() {
    ptr++;
    if (*ptr == '\0') {
        fprintf(stderr, "Error: secuencia de escape incompleta\n");
        exit(1);
    }
    switch (*ptr) {
        case 'n': ptr++; return '\n';
        case 't': ptr++; return '\t';
        case 'r': ptr++; return '\r';
        case '\\': ptr++; return '\\';
        default: return *ptr++;
    }
}
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
    char presentes[128] = {0};
    int negada = 0;

    if (*ptr != '[') {
        fprintf(stderr, "Error: clase debe comenzar con [\n");
        exit(1);
    }
    ptr++;

    if (*ptr == '^') {
        negada = 1;
        ptr++;
    }

    while (*ptr && *ptr != ']') {
        if (*(ptr+1) == '-' && *(ptr+2) && *(ptr+2) != ']') {
            char inicio = *ptr;
            char fin = *(ptr+2);
            if (inicio > fin) {
                fprintf(stderr, "Error: rango inválido [%c-%c]\n", inicio, fin);
                exit(1);
            }
            for (char c = inicio; c <= fin; c++) {
                presentes[(unsigned char)c] = 1;
            }
            ptr += 3;
        } else {
            char c = *ptr++;
            presentes[(unsigned char)c] = 1;
        }
    }

    if (*ptr != ']') {
        fprintf(stderr, "Error: clase sin cierre ]\n");
        exit(1);
    }
    ptr++;

    FragmentoNFA out;
    int inicializado = 0;

    for (int c = 0; c < 128; c++) {
        int incluir = (!negada && presentes[c]) || (negada && !presentes[c]);
        if (incluir) {
            FragmentoNFA temp = crear_trans((char)c);
            if (!inicializado) {
                out = temp;
                inicializado = 1;
            } else {
                out = alternar(out, temp);
            }
        }
    }

    if (!inicializado) {
        fprintf(stderr, "Error: clase vacía o mal definida\n");
        exit(1);
    }

    return out;
}

static FragmentoNFA parse_base() {
    if (*ptr == '(') {
        ptr++;
        FragmentoNFA f = parse_expr();
        if (*ptr != ')') {
            fprintf(stderr, "Error: paréntesis no cerrado\n");
            exit(1);
        }
        ptr++;
        return f;
    } else if (*ptr == '[') {
        return clase();
    } else if (*ptr == '\\') {
        char escaped = manejar_escape();
        return crear_trans(escaped);
    } else if (*ptr && !strchr("|)*+?", *ptr)) {
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
            frag = kleene(frag);
        } else if (*ptr == '+') {
            ptr++;
            frag = mas(frag);
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
    FragmentoNFA frag;
    int iniciado = 0;

    while (*ptr && *ptr != ')' && *ptr != '|') {
        FragmentoNFA parte = parse_factor();
        if (!iniciado) {
            frag = parte;
            iniciado = 1;
        } else {
            frag = concatenar(frag, parte);
        }
    }

    if (!iniciado) {
        // Retornar NFA vacío (cadena vacía)
        EstadoNFA* i = nuevo_estado();
        EstadoNFA* f = nuevo_estado();
        agregar_epsilon(i, f);
        frag.inicio = i;
        frag.fin = f;
    }

    return frag;
}


static FragmentoNFA parse_expr() {
    FragmentoNFA izquierda = parse_term();

    while (*ptr == '|') {
        ptr++;  // consume '|'
        FragmentoNFA derecha = parse_term();
        izquierda = alternar(izquierda, derecha);
    }

    return izquierda;
}


FragmentoNFA parse_regex(const char* regex) {
    ptr = regex;
    FragmentoNFA f = parse_expr();
    f.fin->es_final = 1;
    return f;
}
