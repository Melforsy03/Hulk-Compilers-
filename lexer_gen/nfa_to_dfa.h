// nfa_to_dfa.h
#ifndef NFA_TO_DFA_H
#define NFA_TO_DFA_H

#include "regex_parser.h"
#include "../lexer/lexer.h"
#include "utils.h"
 extern int estado_dfa_id_global;
// Conversión NFA → DFA
DFA convertir_nfa_a_dfa(EstadoNFA* inicio_nfa, int token_id);

#endif

