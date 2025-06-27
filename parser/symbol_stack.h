#ifndef SYMBOL_STACK_H 
#define SYMBOL_STACK_H

#include "grammar.h" 
#include <stdbool.h>

#define STACK_MAX 1024

typedef struct { Symbol data[STACK_MAX]; int top; } SymbolStack;

static inline void stack_init(SymbolStack* s) { 
    s->top = 0; 
} 
static inline bool stack_empty(const SymbolStack* s) { 
    return s->top == 0; 
} 
static inline void stack_push(SymbolStack* s, Symbol x) { 
    if (s->top < STACK_MAX) s->data[s->top++] = x; 
} 
static inline Symbol stack_pop(SymbolStack* s) { 
    return (s->top>0) ? s->data[--s->top] : (Symbol){0,0}; 
} 
static inline Symbol stack_top(const SymbolStack* s) { 
    return (s->top>0) ? s->data[s->top-1] : (Symbol){0,0}; 
}

#endif // SYMBOL_STACK_H