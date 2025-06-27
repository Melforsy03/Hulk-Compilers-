#include "parser_ll1.h" 
#include <stdio.h>

AST* parse_ll1(const Grammar* G, const ParseTable* pt, TokenStream* ts) { 
    SymbolStack stack; stack_init(&stack); 
    // Pila de AST ASTStack asts; 
    aststack_init(&asts);

```
// Apilar EOF y símbolo inicial
stack_push(&stack, (Symbol){ .type = TERM,    .id = G->eof_term    });
stack_push(&stack, (Symbol){ .type = NONTERM, .id = G->start_symbol });

Token tk = ts_peek(ts);
while (!stack_empty(&stack)) {
    Symbol X = stack_top(&stack);

    if (X.type == TERM) {
        if (X.id == tk.type) {
            stack_pop(&stack);
            AST* leaf = ast_new(AST_LEAF, tk.lexeme);
            aststack_push(&asts, leaf);
            ts_next(ts);
            tk = ts_peek(ts);
        } else {
            fprintf(stderr, "Unexpected token '%s'n", tk.lexeme);
            return NULL;
        }
    } else {
        int prod = pt->table[X.id][tk.type];
        if (prod < 0) {
            fprintf(stderr, "Syntax error at nonterminal %d with token '%s'n", X.id, tk.lexeme);
            return NULL;
        }
        stack_pop(&stack);
        const Production* P = &G->prods[prod];
        AST* node = ast_new(AST_NODE, G->nterms[X.id]);

        // Apilar cuerpo en orden inverso y preparar AST niños
        for (int i = P->body_len - 1; i >= 0; --i) {
            stack_push(&stack, P->body[i]);
            AST* child = (P->body[i].type == TERM)
                             ? ast_new(AST_LEAF, NULL)
                             : ast_new(AST_NODE, G->nterms[P->body[i].id]);
            aststack_push(&asts, child);
        }
        // Conectar hijos al nodo
        int n = P->body_len;
        for (int i = n - 1; i >= 0; --i) {
            AST* child = aststack_pop(&asts);
            ast_add_child(node, child);
        }
        aststack_push(&asts, node);
    }
}
return aststack_pop(&asts);
```

}