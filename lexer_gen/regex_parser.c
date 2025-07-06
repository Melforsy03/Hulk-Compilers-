// regex_parser.c con correcciones para el parsing de strings
#include "regex_parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static const char* ptr;

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
    
    // Usar agregar_epsilon en lugar de asignación directa
    agregar_epsilon(ini, f1.inicio);
    agregar_epsilon(ini, f2.inicio);
    agregar_epsilon(f1.fin, fin);
    agregar_epsilon(f2.fin, fin);
    
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

static FragmentoNFA opcional(FragmentoNFA f) {
    EstadoNFA* ini = nuevo_estado();
    EstadoNFA* fin = nuevo_estado();
    ini->epsilon1 = f.inicio;
    ini->epsilon2 = fin;
    f.fin->epsilon1 = fin;
    return (FragmentoNFA){ini, fin};
}

static char manejar_escape(int en_regex) {
    if (*ptr != '\\') {
        fprintf(stderr, "Error: se esperaba secuencia de escape\n");
        exit(1);
    }
    ptr++; // Saltar el backslash
    
    if (*ptr == '\0') {
        fprintf(stderr, "Error: secuencia de escape incompleta\n");
        exit(1);
    }

    switch (*ptr) {
        case 'n': ptr++; return '\n';
        case 't': ptr++; return '\t';
        case 'r': ptr++; return '\r';
        case '\\': ptr++; return '\\';
        case '"': ptr++; return '"';
        case '\'': ptr++; return '\'';
        case '.': 
            ptr++;
            return '.'; // literal punto
        case '0': ptr++; return '\0';
        case 'x': {
            ptr++;
            char hex[3] = {0};
            if (!isxdigit(*ptr)) {
                fprintf(stderr, "Error: secuencia hexadecimal incompleta\n");
                exit(1);
            }
            hex[0] = *ptr++;
            if (isxdigit(*ptr)) {
                hex[1] = *ptr++;
            }
            return (char)strtol(hex, NULL, 16);
        }
        default:
            // Para caracteres no especiales, devolver el carácter literal
            return *ptr++;
    }
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
        char c;
        if (*ptr == '\\') {
            c = manejar_escape(1);
        } else {
            c = *ptr++;
        }

        if (*ptr == '-' && *(ptr + 1) != ']' && *(ptr + 1) != '\0') {
            ptr++;
            char c2;
            if (*ptr == '\\') {
                c2 = manejar_escape(1);
            } else {
                c2 = *ptr++;
            }
            for (char ch = c; ch <= c2; ch++) {
                presentes[(unsigned char)ch] = 1;
            }
        } else {
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

    for (int c = 1; c < 127; c++) {
        int pertenece = presentes[c];
        if ((negada && !pertenece) || (!negada && pertenece)) {
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

static FragmentoNFA parse_string() {
    if (*ptr != '"') {
        fprintf(stderr, "Error: string debe comenzar con comillas\n");
        exit(1);
    }
    ptr++; // Saltar comilla inicial
    
    EstadoNFA* inicio = nuevo_estado();
    EstadoNFA* actual = inicio;
    EstadoNFA* nuevo;
    
    while (*ptr && *ptr != '"') {
        nuevo = nuevo_estado();
        
        if (*ptr == '\\') {
            // Manejar secuencia de escape
            ptr++;
            if (!*ptr) {
                fprintf(stderr, "Error: escape incompleto\n");
                exit(1);
            }
            // Solo transición para el carácter escapado
            actual->transiciones[(unsigned char)*ptr] = nuevo;
            ptr++;
        } else {
            // Cualquier carácter excepto comilla o newline
            if (*ptr == '\n') {
                fprintf(stderr, "Error: salto de línea en string\n");
                exit(1);
            }
            actual->transiciones[(unsigned char)*ptr] = nuevo;
            ptr++;
        }
        actual = nuevo;
    }
    
    if (*ptr != '"') {
        fprintf(stderr, "Error: string no cerrado\n");
        exit(1);
    }
    ptr++; // Saltar comilla final
    
    return (FragmentoNFA){inicio, actual};
}
static FragmentoNFA crear_trans_rango(char inicio, char fin) {
    FragmentoNFA resultado;
    int inicializado = 0;
    
    for (char c = inicio; c <= fin; c++) {
        FragmentoNFA temp = crear_trans(c);
        if (!inicializado) {
            resultado = temp;
            inicializado = 1;
        } else {
            resultado = alternar(resultado, temp);
        }
    }
    
    return resultado;
}
FragmentoNFA crear_identificador() {
    // Crear componentes para la primera letra (debe ser letra o _)
    FragmentoNFA letra_may = crear_trans_rango('A', 'Z');
    FragmentoNFA letra_min = crear_trans_rango('a', 'z');
    FragmentoNFA guion = crear_trans('_');
    
    // Combinar opciones para primer carácter
    FragmentoNFA primer_caracter = alternar(letra_may, letra_min);
    primer_caracter = alternar(primer_caracter, guion);
    
    // Crear componentes para caracteres siguientes (pueden ser letras, números o _)
    FragmentoNFA digito = crear_trans_rango('0', '9');
    FragmentoNFA resto_caracteres = alternar(alternar(letra_may, letra_min), alternar(digito, guion));
    
    // Aplicar cerradura de Kleene a los caracteres siguientes
    FragmentoNFA resto_repeticiones = kleene(resto_caracteres);
    
    // Concatenar primer carácter con el resto
    return concatenar(primer_caracter, resto_repeticiones);
}
static FragmentoNFA parse_base() {
    if (*ptr == '\0') {
        fprintf(stderr, "Error: se esperaba un carácter pero se encontró fin de cadena\n");
        exit(1);
    }
    if (strncmp(ptr, "IDENTIFIER_PATTERN", 18) == 0) {
        ptr += 18;
        return crear_identificador();
    }
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
        char escaped = manejar_escape(1);
        return crear_trans(escaped);
    } else if (*ptr == '.') {
        ptr++;
        FragmentoNFA out;
        int inicializado = 0;
        for (int c = 1; c < 127; c++) {
            if (c != '\n') {
                FragmentoNFA temp = crear_trans((char)c);
                if (!inicializado) {
                    out = temp;
                    inicializado = 1;
                } else {
                    out = alternar(out, temp);
                }
            }
        }
        return out;
    } else if (*ptr == '"') { 
        return parse_string();
    } else if (strchr("|)*+?", *ptr)) {
        fprintf(stderr, "Error: carácter inesperado '%c'\n", *ptr);
        exit(1);
    } else {
        return crear_trans(*ptr++);
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
            frag = opcional(frag);
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
        EstadoNFA* ini = nuevo_estado();
        EstadoNFA* fin = nuevo_estado();
        agregar_epsilon(ini, fin);
        frag.inicio = ini;
        frag.fin = fin;
    }

    return frag;
}

static FragmentoNFA parse_expr() {
    FragmentoNFA izquierda = parse_term();
    
    while (*ptr == '|') {
        ptr++;
        FragmentoNFA derecha = parse_term();
        izquierda = alternar(izquierda, derecha);
    }

    return izquierda;
}

FragmentoNFA parse_regex(const char* regex) {
    if (!regex || *regex == '\0') {
        fprintf(stderr, "Error: regex vacía o nula\n");
        exit(1);
    }
    
    ptr = regex;
    FragmentoNFA f = parse_expr();
    f.fin->es_final = 1;
    return f;
}