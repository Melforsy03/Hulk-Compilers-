// precedence.h
#ifndef PRECEDENCE_H
#define PRECEDENCE_H

typedef enum {
    LEFT,
    RIGHT,
    NONASSOC
} Assoc;

typedef struct {
    char* symbol;
    int precedence;
    Assoc assoc;
} OperatorPrecedence;

extern OperatorPrecedence operator_table[];

int get_precedence(const char* symbol);
Assoc get_assoc(const char* symbol);

#endif // PRECEDENCE_H
