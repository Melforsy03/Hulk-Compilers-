#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ll1_table.h"
#include "first_follow.h"
#include "cst.h"
#include "lexer.h"

// Estructura del parser LL(1)
typedef struct {
    const char* input;          // Texto fuente
    Token current_token;        // Token actual
    LL1Table* ll1_table;        // Tabla LL(1)
    const Grammar* grammar;     // Gramática
} LL1Parser;
#define MAX_STACK 1024
void init_parser(LL1Parser* parser, const char* input, LL1Table* table, const Grammar* grammar) {
    parser->input = input;
    parser->ll1_table = table;
    parser->grammar = grammar;
    parser->current_token = next_token(&parser->input);
}
#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

const char* token_to_symbol(Token t) {
    switch (t.type) {
        // ✅ Palabras clave
        case TOKEN_LET:         return "let";
        case TOKEN_IN:          return "in";
        case TOKEN_FUNCTION:    return "function";
        case TOKEN_TYPE:        return "type";
        case TOKEN_IF:          return "if";
        case TOKEN_THEN:        return "then";
        case TOKEN_ELIF:        return "elif";
        case TOKEN_ELSE:        return "else";
        case TOKEN_WHILE:       return "while";
        case TOKEN_FOR:         return "for";
        case TOKEN_TRUE:        return "true";
        case TOKEN_FALSE:       return "false";
        case TOKEN_NEW:         return "new";
        case TOKEN_INHERITS:    return "inherits";
        case TOKEN_SELF:        return "self";
        case TOKEN_BASE:        return "base";
        case TOKEN_RETURN:      return "return";
        case TOKEN_RANGE:       return "range";
        case TOKEN_PROTOCOL:   return "protocol";
        case TOKEN_EXTENDS:     return "extends";
        case TOKEN_TAN:         return "tan";
        case TOKEN_COS:         return "cos";
        case TOKEN_SIN:         return "sin";
        case TOKEN_LOG:         return "log";
        case TOKEN_PI:          return "PI";
        case TOKEN_COT:         return "cot";
        case TOKEN_PRINT:       return "print";
        case TOKEN_AS:          return "as";
        case TOKEN_IS:          return "is";
        // ✅ Identificadores y literales
        case TOKEN_IDENTIFIER:  return "IDENTIFIER";
        case TOKEN_NUMBER:      return "NUMBER";
        case TOKEN_STRING:      return "STRING";

        // ✅ Operadores
        case TOKEN_ASSIGN:      return "=";
        case TOKEN_PLUS:        return "+";
        case TOKEN_MINUS:       return "-";
        case TOKEN_STAR:        return "*";
        case TOKEN_SLASH:       return "/";
        case TOKEN_MODULO:      return "%";
        case TOKEN_POWER:       return "^";
        case TOKEN_DSTAR:       return "**";
        case TOKEN_ARROW:      return "=>";
        case TOKEN_COLON_EQUAL: return ":=";
        case TOKEN_AT_AT:       return "@@";
        case TOKEN_EQUAL_EQUAL: return "==";
        case TOKEN_NOT_EQUAL:   return "!=";
        case TOKEN_LESS_EQUAL:  return "<=";
        case TOKEN_GREATER_EQUAL: return ">=";
        case TOKEN_AND:         return "&&";
        case TOKEN_OR:          return "or";
        case TOKEN_NOT:         return "!";
        case TOKEN_AT:          return "@";
        case TOKEN_DOT:         return ".";
        case TOKEN_PUNTOS:      return ":";
        

        // ✅ Delimitadores
        case TOKEN_SEMICOLON:   return ";";
        case TOKEN_COMMA:       return ",";
        case TOKEN_LPAREN:      return "(";
        case TOKEN_RPAREN:      return ")";
        case TOKEN_LBRACE:      return "{";
        case TOKEN_RBRACE:      return "}";
        case TOKEN_LBRACKET:    return "[";
        case TOKEN_RBRACKET:    return "]";

        // ✅ Comparadores
        case TOKEN_LESS:        return "<";
        case TOKEN_GREATER:     return ">";

        // ✅ Fin de entrada
        case TOKEN_EOF:         return "EOF";
        case TOKEN_WHITESPACE:  return "WHITESPACE";

        default:
            fprintf(stderr, "Error: token_to_symbol() recibió token desconocido: %d\n", t.type);
            exit(1);
    }
}
Production* find_ll1_entry(const LL1Table* table, const char* non_terminal, const char* terminal) {
    

    for (int i = 0; i < table->num_entries; ++i) {
        if (strcmp(table->entries[i].non_terminal, non_terminal) == 0 &&
            strcmp(table->entries[i].terminal, terminal) == 0) {
            return table->entries[i].production;
        }
    }
    return NULL;
}
void insert_cst_child_at(CSTNode* parent, CSTNode* child, int index) {
    if (!parent || !child) {
        fprintf(stderr, "Error: parent o child es NULL\n");
        return;
    }

    if (index < 0 || index > parent->num_children) {
        fprintf(stderr, "Error: índice %d fuera de rango (num_children=%d)\n", index, parent->num_children);
        return;
    }

    if (parent->num_children >= parent->capacity) {
        parent->capacity *= 2;
        parent->children = realloc(parent->children, sizeof(CSTNode*) * parent->capacity);
        if (!parent->children) {
            fprintf(stderr, "Error: realloc falló en insert_cst_child_at\n");
            exit(1);
        }
    }

    // Desplaza hijos
    for (int i = parent->num_children; i > index; i--) {
        parent->children[i] = parent->children[i - 1];
    }

    parent->children[index] = child;
    parent->num_children++;
}
CSTNode* parse(LL1Parser* parser, const char* start_symbol) {
    const char* symbol_stack[MAX_STACK];
    CSTNode* node_stack[MAX_STACK];
    int stack_top = -1;

    // Inicializa pila con símbolo final $
    symbol_stack[++stack_top] = "$";
    node_stack[stack_top] = NULL;

    // Nodo raíz inicial
    CSTNode* root = create_cst_node(start_symbol);
    symbol_stack[++stack_top] = start_symbol;
    node_stack[stack_top] = root;

    // printf("\n🔍 TOKEN inicial: type=%d lexema=%s\n",
    //        parser->current_token.type, parser->current_token.lexema);

    while (stack_top >= 0) {
        const char* top = symbol_stack[stack_top];
        CSTNode* parent = node_stack[stack_top--];

        if (!top) {
            fprintf(stderr, "❌ Error: símbolo NULL en pila.\n");
            free_cst(root);
            return NULL;
        }

        if (strcmp(top, "$") == 0) {
            if (parser->current_token.type != TOKEN_EOF) {
                fprintf(stderr, "❌ Error: entrada incompleta (esperaba EOF)\n");
                free_cst(root);
                return NULL;
            }
            // printf("✅ Fin de entrada OK.\n");
            break;
        }

        const char* token_symbol = token_to_symbol(parser->current_token);
        // printf("TOP: %-20s | TOKEN: %-10s | Lookahead type=%d\n",
        //        top, token_symbol, parser->current_token.type);

        if (is_symbol_terminal(parser->grammar, top) || strcmp(top, "ε") == 0) {
            if (strcmp(top, "ε") == 0) {
                // printf("  ↪️  Derivando ε\n");
                CSTNode* epsilon_node = create_cst_node("ε");
                add_cst_child(parent, epsilon_node);
                continue;
            }

            if (strcmp(top, token_symbol) == 0) {
                // printf("  ✅ Matched terminal: %s\n", top);
                parent->token = malloc(sizeof(Token));
                *parent->token = parser->current_token;

                parser->current_token = next_token(&parser->input);
                // printf("  ➡️  SIGUIENTE TOKEN: %s\n",
                //        parser->current_token.lexema ? parser->current_token.lexema : "(null)");
            } else {
                fprintf(stderr, "❌ Error: esperado '%s', encontrado '%s'\n",
                        top, token_symbol);
                free_cst(root);
                return NULL;
            }
        } else {
            // printf("LOOKUP: M[%s, %s]\n", top, token_symbol);
            Production* prod = find_ll1_entry(parser->ll1_table, top, token_symbol);
            if (!prod) {
                fprintf(stderr, "❌ Error: no hay producción para [%s, %s]\n",
                        top, token_symbol);
                free_cst(root);
                return NULL;
            }

            // printf("  ➡️  Expansión: %s ::= ", prod->lhs);
            // for (int k = 0; k < prod->rhs_len; ++k) {
            //     printf("%s ", prod->rhs[k]);
            // }
            // printf("\n");

            // ✅ Expansión inversa robusta para la pila
            for (int i = prod->rhs_len - 1; i >= 0; --i) {
                const char* rhs_symbol = prod->rhs[i];

                if (!rhs_symbol || strlen(rhs_symbol) == 0) {
                    fprintf(stderr, "❌ Error: RHS[%d] en producción '%s' está vacío o NULL.\n",
                            i, prod->lhs);
                    free_cst(root);
                    return NULL;
                }

                // printf("    ↳ PUSH: %s\n", rhs_symbol);

                if (stack_top + 1 >= MAX_STACK) {
                    fprintf(stderr, "❌ Error: desbordamiento de pila.\n");
                    free_cst(root);
                    return NULL;
                }

                CSTNode* child = create_cst_node(rhs_symbol);
                insert_cst_child_at(parent, child, 0);

                symbol_stack[++stack_top] = rhs_symbol;
                node_stack[stack_top] = child;
            }
        }
    }

    printf("\n✅ Parseo completado sin errores.\n");
    return root;
}
