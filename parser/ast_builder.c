#include "ast_builder.h"
#include "../parser/lr1_table.h"
#include "../grammar/grammar.h"
#include <stdlib.h>
#include <string.h>

// ----- Fábricas de nodos -----
ProgramNode* ast_make_program(DeclarationNode** decls, int decl_count,ExpressionNode* expr, int row, int col){
    
    ProgramNode* n = malloc(sizeof *n);
    n->base.row    = row;
    n->base.column = col;
    n->base.tipo   = NODE_PROGRAM;       
    DeclarationNode** list = malloc((decl_count + 1) * sizeof *list);
    for (int i = 0; i < decl_count; ++i) {
        list[i] = decls[i];
    }
    list[decl_count] = NULL;

    n->declarations = list;
    n->expression   = expr;              // expr es ExpressionNode*

    return n;
}


FunctionDeclarationNode*
ast_make_function(const char* name, DeclarationNode** params, int param_count, ExpressionNode* body, const char* returnType, int row, int col)
{
    FunctionDeclarationNode* n = malloc(sizeof *n);

    n->base.base.row    = row;
    n->base.base.column = col;
    n->base.base.tipo   = NODE_FUNCTION_DECLARATION;

    n->name = strdup(name);
    DeclarationNode** list = malloc((param_count + 1) * sizeof *list);
    for (int i = 0; i < param_count; ++i) {
        list[i] = params[i];
    }
    list[param_count] = NULL;

    n->params          = list;
    n->param_counter   = param_count;
    n->body            = body;
    n->returnType      = returnType ? strdup(returnType) : NULL;

    return n;
}

BinaryNode* ast_make_binary(NodeType kind, ExpressionNode* left, ExpressionNode* right, const char* op, int row, int col) {
    BinaryNode* n = malloc(sizeof(BinaryNode));
    n->base.base.row    = row;
    n->base.base.column = col;
    n->base.base.tipo   = kind;
    n->left             = left;
    n->right            = right;
    n->operator         = op ? strdup(op) : NULL;
    return n;
}

UnaryNode* ast_make_unary(NodeType kind, ExpressionNode* operand, int row, int col) {
    UnaryNode* n = malloc(sizeof(UnaryNode));
    n->base.base.row = row;
    n->base.base.column = col;
    n->base.base.tipo = kind;
    n->operand = operand;
    return n;
}


LetInNode* ast_make_let_in(VarDeclarationNode** declarations, int decl_count, ExpressionNode* body, int row, int col) {
    LetInNode* node = malloc(sizeof(LetInNode));
    node->base.base.row = row;
    node->base.base.column = col;
    node->base.base.symbol = create_symbol("LetExpr", NON_TERMINAL);
    node->base.base.lexeme = "let";
    node->base.base.tipo ;
    
    // Copiar array de declaraciones
    VarDeclarationNode** n = malloc((decl_count + 1) * sizeof(VarDeclarationNode*));
    for (int i = 0; i < decl_count; i++) {
        n[i] = declarations[i];
    }
    n[decl_count] = NULL;
    node->variable_counter = decl_count;
    
    node->variables = n;
    node->body = body;
    return node;
}

VarDeclarationNode* ast_make_var_decl(const char* name, ExpressionNode* init, const char* type, int row, int col) {
    VarDeclarationNode* node = malloc(sizeof(VarDeclarationNode));
    node->base.base.row = row;
    node->base.base.column = col;
    node->base.base.symbol = create_symbol("VarDecl", NON_TERMINAL);
    node->base.base.lexeme = strdup(name);
    node->base.base.tipo = NODE_VAR_DECLARATION;

    node->name = strdup(name);
    node->value = init;
    node->type = type ? strdup(type) : NULL;
    return node;
}


ConditionalNode* 
ast_make_conditional(ExpressionNode** conditions,  ExpressionNode** expressions, int condition_count, ExpressionNode* default_expr, int row, int col) {
    ConditionalNode* node = malloc(sizeof(ConditionalNode));
    node->base.base.row = row;
    node->base.base.column = col;
    node->base.base.symbol = create_symbol("If", NON_TERMINAL);
    node->base.base.lexeme = "if";
    node->base.base.tipo = NODE_CONDITIONAL;

    // Copiar arrays de condiciones y expresiones
    ExpressionNode** list = malloc(condition_count * sizeof(ExpressionNode*));
    ExpressionNode** expr = malloc(condition_count * sizeof(ExpressionNode*));
    
     
    for (int i = 0; i < condition_count; i++) {
        list[i] = conditions[i];
        expr[i] = expressions[i];
    }
    
    node->conditions = list;
    node->expressions = expr;
    node->condition_counter = condition_count;
    node->expression_counter = condition_count;
    node->default_expre = default_expr;
    return node;
}

WhileNode* ast_make_while(ExpressionNode* condition, ExpressionNode* body, int row, int col) {
    WhileNode* node = malloc(sizeof(WhileNode));
    node->base.base.row = row;
    node->base.base.column = col;
    node->base.base.symbol = create_symbol("While", NON_TERMINAL);
    node->base.base.lexeme = "while";
    node->base.base.tipo = NODE_POW;

    node->condition = condition;
    node->body = body;
    return node;
}

ForNode* ast_make_for(const char* var_name, ExpressionNode* iterable, ExpressionNode* body, int row, int col) {
    ForNode* node = malloc(sizeof(ForNode));
    node->base.base.row = row;
    node->base.base.column = col;
    node->base.base.symbol = create_symbol("For", NON_TERMINAL);
    node->base.base.lexeme = "for";
    node->base.base.tipo = NODE_FOR;

    node->item = strdup(var_name);
    node->iterable = iterable;
    node->body = body;
    return node;
}

LiteralNode* ast_make_literal(NodeType lit_kind, const char* lexeme, int row, int col) {
    LiteralNode* n = malloc(sizeof(LiteralNode));
    n->base.base.base.row    = row;
    n->base.base.base.column = col;
    n->base.base.base.tipo   = lit_kind;
    n->lex              = strdup(lexeme);
    return n;
}

// // ----- Conexión lexer → parser → AST tipado -----
// Node* build_ast(TokenList* tokens) {
//     int tok_count = token_list_length(tokens);
//     Symbol** input = tokens_to_symbols(tokens);

//     ActionEntryLR1* actions;
//     int action_count;
//     // parser devuelve un nodo tipado (ProgramNode*)
//     Node* root = parser(&lr1_table, input, tok_count, &actions, &action_count);
//     return root;
// }