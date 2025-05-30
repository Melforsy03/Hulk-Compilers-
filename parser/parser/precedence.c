
#include <string.h>
#include "precedence.h"

OperatorPrecedence operator_table[] = {
    {"+", 1, LEFT},
    {"-", 1, LEFT},
    {"*", 2, LEFT},
    {"/", 2, LEFT},
    {"%", 2, LEFT},
    {"^", 3, RIGHT},
    {"**", 3, RIGHT},
    {"==", 0, NONASSOC},
    {"!=", 0, NONASSOC},
    {">", 0, NONASSOC},
    {">=", 0, NONASSOC},
    {"<", 0, NONASSOC},
    {"<=", 0, NONASSOC},
    {NULL, 0, NONASSOC}
};

int get_precedence(const char* symbol) {
    for (int i = 0; operator_table[i].symbol != NULL; ++i)
        if (strcmp(operator_table[i].symbol, symbol) == 0)
            return operator_table[i].precedence;
    return -1;
}

Assoc get_assoc(const char* symbol) {
    for (int i = 0; operator_table[i].symbol != NULL; ++i)
        if (strcmp(operator_table[i].symbol, symbol) == 0)
            return operator_table[i].assoc;
    return NONASSOC;
}
