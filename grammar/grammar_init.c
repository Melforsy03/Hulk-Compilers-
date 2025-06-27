#include "grammar_init.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Grammar* init_arithmetic_grammar() {
    Grammar* g = create_grammar();

    // Terminales
    Symbol* t_plus = add_symbol(g, "PLUS", TERMINAL);
    Symbol* t_minus = add_symbol(g, "MINUS", TERMINAL);
    Symbol* t_mult = add_symbol(g, "STAR", TERMINAL);
    Symbol* t_div = add_symbol(g, "DIV", TERMINAL);
    Symbol* t_pow = add_symbol(g, "POW", TERMINAL);
    Symbol* t_lparen = add_symbol(g, "LPAREN", TERMINAL);
    Symbol* t_rparen = add_symbol(g, "RPAREN", TERMINAL);
    Symbol* t_num = add_symbol(g, "NUMBER", TERMINAL);
    Symbol* t_less = add_symbol(g, "LESS", TERMINAL);
    Symbol* t_greater = add_symbol(g, "GREATER", TERMINAL);
    Symbol* t_less_eq = add_symbol(g, "LESS_EQ", TERMINAL);
    Symbol* t_greater_eq = add_symbol(g, "GREATER_EQ", TERMINAL);
    Symbol* t_equal = add_symbol(g, "EQUAL", TERMINAL);
    Symbol* t_not_equal = add_symbol(g, "NOT_EQUAL", TERMINAL);
    Symbol* t_and = add_symbol(g, "AND", TERMINAL);
    Symbol* t_or = add_symbol(g, "OR", TERMINAL);
    Symbol* t_not = add_symbol(g, "NOT", TERMINAL);
    Symbol* t_true = add_symbol(g, "TRUE", TERMINAL);
    Symbol* t_false = add_symbol(g, "FALSE", TERMINAL);
    Symbol* t_comma = add_symbol(g, "COMMA", TERMINAL);
    Symbol* t_semicolon = add_symbol(g, "SEMICOLON", TERMINAL);
    Symbol* t_concat = add_symbol(g, "CONCAT", TERMINAL);
    Symbol* t_string = add_symbol(g, "STRING", TERMINAL);
    
    
    // No terminales
    Symbol* nt_program = add_symbol(g, "Program", NON_TERMINAL);
    Symbol* nt_expr = add_symbol(g, "Expr", NON_TERMINAL);
    Symbol* nt_expr_prime = add_symbol(g, "Expr'", NON_TERMINAL);
    Symbol* nt_term = add_symbol(g, "Term", NON_TERMINAL);
    Symbol* nt_term_prime = add_symbol(g, "Term'", NON_TERMINAL);
    Symbol* nt_factor = add_symbol(g, "Factor", NON_TERMINAL);
    Symbol* nt_pow = add_symbol(g, "Pow", NON_TERMINAL);
    Symbol* nt_atom = add_symbol(g, "Atom", NON_TERMINAL);
    Symbol* nt_comparison = add_symbol(g, "Comparison", NON_TERMINAL);
    Symbol* nt_equality = add_symbol(g, "Equality", NON_TERMINAL);
    Symbol* nt_and_expr = add_symbol(g, "AndExpr", NON_TERMINAL);
    Symbol* nt_or_expr = add_symbol(g, "OrExpr", NON_TERMINAL);
    Symbol* nt_not_expr = add_symbol(g, "NotExpr", NON_TERMINAL);
    Symbol* nt_string_expr = add_symbol(g, "StringExpr", NON_TERMINAL);
    
    
    // Producciones
    // Program → Expr
    Symbol* right_program[] = {nt_expr};
    add_production(g, nt_program, right_program, 1);
    
    // Expr → OrExpr
    Symbol* right_expr[] = {nt_or_expr};
    add_production(g, nt_expr, right_expr, 1);
    
    // OrExpr → AndExpr ('||' AndExpr)*
    Symbol* right_or[] = {nt_and_expr, t_or, nt_or_expr};
    Symbol* right_or_base[] = {nt_and_expr};
    add_production(g, nt_or_expr, right_or, 3);
    add_production(g, nt_or_expr, right_or_base, 1);
    
    // AndExpr → Equality ('&&' Equality)*
    Symbol* right_and[] = {nt_equality, t_and, nt_and_expr};
    Symbol* right_and_base[] = {nt_equality};
    add_production(g, nt_and_expr, right_and, 3);
    add_production(g, nt_and_expr, right_and_base, 1);
    
    // Equality → Comparison (('==' | '!=') Comparison)*
    Symbol* right_eq[] = {nt_comparison, t_equal, nt_equality};
    Symbol* right_not_eq[] = {nt_comparison, t_not_equal, nt_equality};
    Symbol* right_eq_base[] = {nt_comparison};
    add_production(g, nt_equality, right_eq, 3);
    add_production(g, nt_equality, right_not_eq, 3);
    add_production(g, nt_equality, right_eq_base, 1);
    
    // Comparison → Expr' (('<' | '>' | '<=' | '>=') Comparison)*
    Symbol* right_less[] = {nt_expr_prime, t_less, nt_comparison};
    Symbol* right_greater[] = {nt_expr_prime, t_greater, nt_comparison};
    Symbol* right_less_eq[] = {nt_expr_prime, t_less_eq, nt_comparison};
    Symbol* right_greater_eq[] = {nt_expr_prime, t_greater_eq, nt_comparison};
    Symbol* right_comp_base[] = {nt_expr_prime};
    add_production(g, nt_comparison, right_less, 3);
    add_production(g, nt_comparison, right_greater, 3);
    add_production(g, nt_comparison, right_less_eq, 3);
    add_production(g, nt_comparison, right_greater_eq, 3);
    add_production(g, nt_comparison, right_comp_base, 1);
    
    
    // Expr' → '+' Term Expr' | '-' Term Expr' | ε
    Symbol* right_expr_prime1[] = {t_plus, nt_term, nt_expr_prime};
    Symbol* right_expr_prime2[] = {t_minus, nt_term, nt_expr_prime};
    Symbol* right_expr_prime3[] = {g->epsilon};  // ε
    add_production(g, nt_expr_prime, right_expr_prime1, 3);
    add_production(g, nt_expr_prime, right_expr_prime2, 3);
    add_production(g, nt_expr_prime, right_expr_prime3, 1);
    
    // Term → Factor Term'
    Symbol* right_term[] = {nt_factor, nt_term_prime};
    add_production(g, nt_term, right_term, 2);
    
    // Term' → '*' Factor Term' | '/' Factor Term' | ε
    Symbol* right_term_prime1[] = {t_mult, nt_factor, nt_term_prime};
    Symbol* right_term_prime2[] = {t_div, nt_factor, nt_term_prime};
    Symbol* right_term_prime3[] = {g->epsilon};
    add_production(g, nt_term_prime, right_term_prime1, 3);
    add_production(g, nt_term_prime, right_term_prime2, 3);
    add_production(g, nt_term_prime, right_term_prime3, 1);
    
    // Factor → '-' Factor | '+' Factor | NotExpr
    Symbol* right_factor1[] = {t_minus, nt_factor};
    Symbol* right_factor2[] = {t_plus, nt_factor};
    Symbol* right_not[] = {t_not, nt_not_expr};
    add_production(g, nt_factor, right_not, 2);
    add_production(g, nt_factor, right_factor1, 2);
    add_production(g, nt_factor, right_factor2, 2);
    
    // NotExpr → Pow | '!' NotExpr
    Symbol* right_not_expr1[] = {nt_pow};
    Symbol* right_not_expr2[] = {t_not, nt_not_expr};
    add_production(g, nt_not_expr, right_not_expr1, 1);
    add_production(g, nt_not_expr, right_not_expr2, 2);
    
    // Pow → Atom '^' Pow | Atom
    Symbol* right_pow1[] = {nt_atom, t_pow, nt_pow};
    Symbol* right_pow2[] = {nt_atom};
    add_production(g, nt_pow, right_pow1, 3);
    add_production(g, nt_pow, right_pow2, 1);
    
    // Atom → NUMBER | '(' Expr ')' | 'true' | 'false'
    Symbol* right_true[] = {t_true};
    Symbol* right_false[] = {t_false};
    Symbol* right_atom1[] = {t_num};
    Symbol* right_atom2[] = {t_lparen, nt_expr, t_rparen};
    add_production(g, nt_atom, right_true, 1);
    add_production(g, nt_atom, right_false, 1);
    add_production(g, nt_atom, right_atom1, 1);
    add_production(g, nt_atom, right_atom2, 3);

    // StringExpr → STRING ('@' Expr)*
    Symbol* right_str_concat[] = {t_string, t_concat, nt_expr};
    Symbol* right_str_base[] = {t_string};
    add_production(g, nt_string_expr, right_str_concat, 3);
    add_production(g, nt_string_expr, right_str_base, 1);

    // Símbolo inicial
    g->start_symbol = nt_program;
    return g;
}