// regex_parser.h
#ifndef REGEX_PARSER_H
#define REGEX_PARSER_H

typedef struct EstadoNFA EstadoNFA;

struct EstadoNFA {
    int id;
    EstadoNFA* transiciones[128];   // Transiciones por carácter ASCII
    EstadoNFA* epsilon1;            // Epsilon transition (opcional)
    EstadoNFA* epsilon2;            // Segunda epsilon (para |, *)
    int es_final;                   // Estado final
};

typedef struct {
    EstadoNFA* inicio;
    EstadoNFA* fin;
} FragmentoNFA;

// Parser principal
FragmentoNFA parse_regex(const char* regex);

// Función útil
EstadoNFA* nuevo_estado();

#endif
