// regex_parser.h
#ifndef REGEX_PARSER_H
#define REGEX_PARSER_H
#include "utils.h"



// Parser principal
FragmentoNFA parse_regex(const char* regex);
FragmentoNFA crear_identificador() ;
// Función útil
EstadoNFA* nuevo_estado();

#endif
