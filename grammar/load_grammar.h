#ifndef LOAD_GRAMMAR_H
#define LOAD_GRAMMAR_H

#include "grammar.h"

// Declaración de la función para eliminar espacios en una cadena
void trim(char* str);

// Declaración de la función para cargar una gramática desde un archivo
Grammar* load_grammar_from_file(const char* filename);

#endif 