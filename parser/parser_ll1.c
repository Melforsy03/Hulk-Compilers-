#include "parser_ll1.h"
#include <stdio.h>
#include <stdlib.h>

// Prototipos de funciones internas
static void advance(ParserLL1* p);
static void match(ParserLL1* p, TokenType expected, const char* error_msg);
static NumberNode* parse_number(ParserLL1* p);
static Node* parse_atom(ParserLL1* p);
static Node* parse_pow(ParserLL1* p);
static Node* parse_not_expr(ParserLL1* p);
static Node* parse_factor(ParserLL1* p);
static Node* parse_term(ParserLL1* p);
static Node* parse_term_prime(ParserLL1* p, Node* left);
static Node* parse_comparison(ParserLL1* p);
static Node* parse_equality(ParserLL1* p);
static Node* parse_and_expr(ParserLL1* p);
static Node* parse_or_expr(ParserLL1* p);
static Node* parse_expression_prime(ParserLL1* p, Node* left);

// Avanza al siguiente token usando tu lexer existente
static void advance(ParserLL1* p) {
    if (p->current_token.type != TOKEN_EOF) {
        p->current_token = next_token(p->input);
    }
}

// Verifica token actual
static void match(ParserLL1* p, TokenType expected, const char* error_msg) {
    if (p->current_token.type != expected) {
        fprintf(stderr, "Error en línea %d: %s\n", 
                p->current_token.line, error_msg);
        exit(1);
    }
    advance(p);
}

// Crea nodo número usando tu estructura existente
static NumberNode* parse_number(ParserLL1* p) {
    printf("esta en number===\n");
    NumberNode* num = malloc(sizeof(NumberNode));
    num->base.base.base.base.tipo = NODE_NUMBER;
    num->base.base.base.base.row = p->current_token.line;
    num->base.base.base.base.column = p->current_token.column;
    num->base.base.base.base.lexeme = strdup(p->current_token.lexema);
    match(p, TOKEN_NUMBER, "Se esperaba número");
    return num;
}

// Función para crear nodos booleanos
static BooleanNode* parse_boolean(ParserLL1* p) {
    BooleanNode* bool_node = malloc(sizeof(BooleanNode));
    bool_node->base.base.base.base.tipo = NODE_BOOLEAN;
    bool_node->base.base.base.base.row = p->current_token.line;
    bool_node->base.base.base.base.column = p->current_token.column;
    
    if (p->current_token.type == TOKEN_TRUE) {
        bool_node->base.lex = "true";
        match(p, TOKEN_TRUE, "Se esperaba 'true'");
    } else {
        bool_node->base.lex = "false";
        match(p, TOKEN_FALSE, "Se esperaba 'false'");
    }
    
    return bool_node;
}

// Atom → NUMBER | '(' Expr ')' | 'true' | 'false'
static Node* parse_atom(ParserLL1* p) {
    printf("esta en atom====\n");
    if (p->current_token.type == TOKEN_NUMBER) {
        return (Node*)parse_number(p);
    } 
    else if (p->current_token.type == TOKEN_LPAREN) {
        match(p, TOKEN_LPAREN, "Se esperaba '('");
        Node* expr = parse_expression(p);
        match(p, TOKEN_RPAREN, "Se esperaba ')'");
        return expr;
    }
    else if (p->current_token.type == TOKEN_TRUE || p->current_token.type == TOKEN_FALSE) {
        return (Node*)parse_boolean(p);
    }
    else {
        fprintf(stderr, "Error: Token inesperado en línea %d\n", p->current_token.line);
        exit(1);
    }
}

// Pow → Atom '^' Pow | Atom
static Node* parse_pow(ParserLL1* p) {
    printf("esta en pow===\n");
    Node* left = parse_atom(p);
    
    if (p->current_token.type == TOKEN_POWER) {
        match(p, TOKEN_POWER, "Se esperaba '^'");
        PowNode* pow = malloc(sizeof(PowNode));
        pow->base.base.base.base.tipo = NODE_POW;
        pow->base.base.base.base.lexeme = "^";
        pow->base.base.operator = "^";
        pow->base.base.left = left;
        pow->base.base.right = parse_pow(p); // Derecha-asociativo
        return (Node*)pow;
    }
    return left;
}

// NotExpr → '!' NotExpr | Pow
static Node* parse_not_expr(ParserLL1* p) {
    printf("esta en not====\n");
    if (p->current_token.type == TOKEN_NOT) {
        match(p, TOKEN_NOT, "Se esperaba '!'");
        NotNode* not = malloc(sizeof(NotNode));
        not->base.base.base.base.tipo = NODE_NOT;
        not->base.base.operand = parse_not_expr(p);
        return (Node*)not;
    }
    return parse_pow(p);
}

// Factor → '-' Factor | '+' Factor | '!' NotExpr | NotExpr
static Node* parse_factor(ParserLL1* p) {
    printf("esta en factor====\n");
    if (p->current_token.type == TOKEN_MINUS) {
        match(p, TOKEN_MINUS, "Se esperaba '-'");
        NegativeNode* neg = malloc(sizeof(NegativeNode));
        neg->base.base.base.base.tipo = NODE_NEGATIVE;
        neg->base.base.operand = parse_factor(p);
        return (Node*)neg;
    }
    else if (p->current_token.type == TOKEN_PLUS) {
        match(p, TOKEN_PLUS, "Se esperaba '+'");
        PositiveNode* pos = malloc(sizeof(PositiveNode));
        pos->base.base.base.base.tipo = NODE_POSITIVE;
        pos->base.base.operand = parse_factor(p);
        return (Node*)pos;
    }
    return parse_not_expr(p);
}

// Term → Factor Term'
static Node* parse_term(ParserLL1* p) {
    printf("esta en term ====\n");
    Node* left = parse_factor(p);
    return parse_term_prime(p, left);
}

// Term' → * Factor Term' | / Factor Term' | ε
static Node* parse_term_prime(ParserLL1* p, Node* left) {
    printf("esta en term prime ====\nr");
    if (p->current_token.type == TOKEN_STAR) {
        match(p, TOKEN_STAR, "Se esperaba '*'");
        MultNode* mult = malloc(sizeof(MultNode));
        mult->base.base.base.base.tipo = NODE_MULT;
        mult->base.base.base.base.lexeme = "*";
        mult->base.base.operator = "*";
        mult->base.base.left = left;
        mult->base.base.right = parse_factor(p);
        return parse_term_prime(p, (Node*)mult); // Izquierda-asociativo
    } 
    else if (p->current_token.type == TOKEN_SLASH) {
        match(p, TOKEN_SLASH, "Se esperaba '/'");
        DivNode* div = malloc(sizeof(DivNode));
        div->base.base.base.base.tipo = NODE_DIV;
        div->base.base.base.base.lexeme = "/";
        div->base.base.operator = "/";
        div->base.base.left = left;
        div->base.base.right = parse_factor(p);
        return parse_term_prime(p, (Node*)div); // Izquierda-asociativo
    }
    return left;
}

// Expr' → '+' Term Expr' | '-' Term Expr' | ε
static Node* parse_expression_prime(ParserLL1* p, Node* left) {
    printf("esta en expr prime ====\n");
    
    if (p->current_token.type == TOKEN_PLUS) {
        match(p, TOKEN_PLUS, "Se esperaba '+'");
        PlusNode* plus = malloc(sizeof(PlusNode));
        plus->base.base.base.base.tipo = NODE_PLUS;
        plus->base.base.base.base.lexeme = "+";
        plus->base.base.operator = "+";
        plus->base.base.left = left;
        plus->base.base.right = parse_term(p);
        return parse_expression_prime(p, (Node*)plus);
    } 
    else if (p->current_token.type == TOKEN_MINUS) {
        match(p, TOKEN_MINUS, "Se esperaba '-'");
        MinusNode* minus = malloc(sizeof(MinusNode));
        minus->base.base.base.base.tipo = NODE_MINUS;
        minus->base.base.base.base.lexeme = "-";
        minus->base.base.operator = "-";
        minus->base.base.left = left;
        minus->base.base.right = parse_term(p);
        return parse_expression_prime(p, (Node*)minus);
    }
    return left;
}
// Comparison → Expr' (('<' | '>' | '<=' | '>=') Expr')*
static Node* parse_comparison(ParserLL1* p) {
    printf("esta en comp ====\n");
    Node* left = parse_term(p);
    left = parse_expression_prime(p, left);
    
    while (1) {
        TokenType op = p->current_token.type;
        if (op == TOKEN_LESS || op == TOKEN_GREATER || 
            op == TOKEN_LESS_EQUAL || op == TOKEN_GREATER_EQUAL) {
            
            match(p, op, "Se esperaba operador de comparación");
            NodeType type;
            const char* op_str;
            
            switch(op) {
                case TOKEN_LESS: type = NODE_LESS; op_str = "<"; break;
                case TOKEN_GREATER: type = NODE_GREATER; op_str = ">"; break;
                case TOKEN_LESS_EQUAL: type = NODE_LESS_EQUAL; op_str = "<="; break;
                case TOKEN_GREATER_EQUAL: type = NODE_GREATER_EQUAL; op_str = ">="; break;
                default: break;
            }
            
            ComparisonBinaryNode* comp = malloc(sizeof(ComparisonBinaryNode));
            comp->base.base.base.lexeme = op_str;
            comp->base.operator = op_str;
            comp->base.base.base.tipo = type;
            comp->base.left = left;
            comp->base.right = parse_expression_prime(p, left);
            //comp->base.operator = strdup(op_str);
            left = (Node*)comp;
        } 
        else {
            break;
        }
    }
    return left;
}

// Equality → Comparison (('==' | '!=') Comparison)*
static Node* parse_equality(ParserLL1* p) {
    printf("esta en equal ====\n");
    Node* left = parse_comparison(p);
    
    while (1) {
        TokenType op = p->current_token.type;
        if (op == TOKEN_EQUAL_EQUAL || op == TOKEN_NOT_EQUAL) {
            
            match(p, op, "Se esperaba operador de igualdad");
            NodeType type = (op == TOKEN_EQUAL_EQUAL) ? NODE_EQUAL : NODE_NOT_EQUAL;
            const char* op_str = (op == TOKEN_EQUAL_EQUAL) ? "==" : "!=";
            
            EqualityBinaryNode* eq = malloc(sizeof(EqualityBinaryNode));
            eq->base.base.base.tipo = type;
            eq->base.left = left;
            eq->base.base.base.lexeme = op_str;
            eq->base.operator = op_str;
            eq->base.right = parse_comparison(p);
            eq->base.operator = strdup(op_str);
            left = (Node*)eq;
        } else {
            break;
        }
    }
    return left;
}

// AndExpr → Equality ('&&' Equality)*
static Node* parse_and_expr(ParserLL1* p) {
    printf("esta en an ====\nd");
    Node* left = parse_equality(p);
    
    while (p->current_token.type == TOKEN_AND) {
        match(p, TOKEN_AND, "Se esperaba '&&'");
        AndNode* and = malloc(sizeof(AndNode));
        and->base.base.base.base.tipo = NODE_AND;
        and->base.base.left = left;
        and->base.base.right = parse_equality(p);
        and->base.base.base.base.lexeme = "&&";
        and->base.base.operator = "&&";
        left = (Node*)and;
    }
    return left;
}

// OrExpr → AndExpr ('||' AndExpr)*
static Node* parse_or_expr(ParserLL1* p) {
    printf("esta en or ====\n");
    Node* left = parse_and_expr(p);
    
    while (p->current_token.type == TOKEN_OR) {
        match(p, TOKEN_OR, "Se esperaba '||'");
        OrNode* or = malloc(sizeof(OrNode));
        or->base.base.base.base.tipo = NODE_OR;
        or->base.base.left = left;
        or->base.base.right = parse_and_expr(p);
        or->base.base.base.base.lexeme = "||";
        or->base.base.operator = "||";
        left = (Node*)or;
    }
    return left;
}

// Expr → OrExpr
Node* parse_expression(ParserLL1* p) {
    printf("esta en expr ====\n");
    return parse_or_expr(p);
}

// Program → Expr
ProgramNode* parse_program(ParserLL1* p) {
    printf("esta en progra ====\n");
    Node* expr = parse_expression(p);
    ProgramNode* prog = malloc(sizeof(ProgramNode));
    prog->base.tipo = NODE_PROGRAM;
    prog->expression = expr;
    return prog;
}

ParserLL1* parser_ll1_new(const char** input, Grammar* grammar) {
    ParserLL1* p = malloc(sizeof(ParserLL1));
    p->input = input;
    p->grammar = grammar;
    p->current_token = next_token(input);
    return p;
}

void parser_ll1_free(ParserLL1* p) {
    if (p) free(p);
}