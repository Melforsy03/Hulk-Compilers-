// parser.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "parser.h"
#include "../lexer/lexer.h"
#include "../ast_nodes/ast_nodes.h"

// Token actual del lexer
Token current_token;
const char* input_source;

// -------------------------------------
// Helpers
// -------------------------------------
Token lookahead_token;

void init_parser(const char* source) {
    input_source = source;
    current_token = next_token(&input_source);
    lookahead_token = next_token(&input_source);
}

void advance() {
    current_token = lookahead_token;
    lookahead_token = next_token(&input_source);
}

Token peek_token() {
    return lookahead_token;
}

bool lookahead_token_is_call() {
    Token peek = peek_token();
    return peek.type == TOKEN_LPAREN;
}

void syntax_error(const char* expected) {
    fprintf(stderr, "Syntax error at line %d, col %d: expected %s, got '%s'\n",
            current_token.line, current_token.column, expected, current_token.lexema);
    exit(EXIT_FAILURE);
}

void match(int token_type) {
    if (current_token.type == token_type) {
        advance();
    } else {
        syntax_error("token does not match");
    }
}
// Program -> Type_function_list Expr_item_list EOF

ProgramNode* parse_program() {
    ProgramNode* program = malloc(sizeof(ProgramNode));
    program->base.tipo = NODE_PROGRAM;
    program->base.row = current_token.line;
    program->base.column = current_token.column;

    int decl_count = 0;
    DeclarationNode** decls = parse_type_function_list(&decl_count);
    program->declarations = decls;

    // ✅ Ahora sí parsea la parte de expresión principal
    program->expression = parse_expr_item_list();

    if (current_token.type != TOKEN_EOF) {
        syntax_error("EOF");
    }

    return program;
}

DeclarationNode** parse_type_function_list(int* count) {
    DeclarationNode** list = NULL;
    *count = 0;

    while (current_token.type == TOKEN_FUNCTION || current_token.type == TOKEN_TYPE) {
    if (current_token.type == TOKEN_FUNCTION) {
        DeclarationNode* decl = parse_func();
        if (!decl) syntax_error("NULL function decl!");
        printf("[DEBUG] Func decl type: %d\n", decl->base.tipo); // ✅ VERIFICA
        list[(*count)++] = decl;
    } else {
        DeclarationNode* decl = parse_type();
        if (!decl) syntax_error("NULL type decl!");
        printf("[DEBUG] Type decl type: %d\n", decl->base.tipo);
        list[(*count)++] = decl;
    }
    }
    return list;
}

DeclarationNode* parse_func() {
    match(TOKEN_FUNCTION);

    FunctionDeclarationNode* func = malloc(sizeof(FunctionDeclarationNode));
    func->base.base.tipo = NODE_FUNCTION_DECLARATION;
    func->base.base.row = current_token.line;
    func->base.base.column = current_token.column;

    // Method_signature -> IDENTIFIER LPAREN Params_list RPAREN
    if (current_token.type != TOKEN_IDENTIFIER) syntax_error("IDENTIFIER");
    func->name = strdup(current_token.lexema);
    match(TOKEN_IDENTIFIER);

    match(TOKEN_LPAREN);

    // Por simplicidad: solo parámetros simples separados por coma
    func->params = NULL;
    func->param_counter = 0;

    if (current_token.type != TOKEN_RPAREN) {
        // Implementa parse_params_list() si quieres luego
        syntax_error("Params parsing not implemented yet");
    }
    
    

    match(TOKEN_RPAREN);


    func->body = NULL;

    return (DeclarationNode*) func;
}
ExpressionNode* parse_expr_item_list() {
    if (current_token.type == TOKEN_LBRACE) {
        return parse_expr_block();
    } else if (current_token.type == TOKEN_IF ||
               current_token.type == TOKEN_WHILE ||
               current_token.type == TOKEN_FOR ||
               current_token.type == TOKEN_LET ||
               current_token.type == TOKEN_IDENTIFIER ||
               current_token.type == TOKEN_NUMBER ||
               current_token.type == TOKEN_TRUE ||
               current_token.type == TOKEN_FALSE ||
               current_token.type == TOKEN_STRING ||
               current_token.type == TOKEN_LPAREN) {

        ExpressionNode* expr = parse_expr();

        // 🟢 Verifica qué tipo de nodo parseaste
        NodeType tipo = expr->base.tipo;

        // Solo requiere ';' si NO es control de flujo ni bloque
        if (tipo != NODE_EXPRESSION_BLOCK &&
            tipo != NODE_FOR &&
            tipo != NODE_WHILE &&
            tipo != NODE_CONDITIONAL) {
            match(TOKEN_SEMICOLON);
        }

        return expr;

    } else {
        // epsilon
        return NULL;
    }
}

ExpressionNode* parse_expr_block() {
    // 🚩 Crea el nodo del bloque
    ExpressionBlockNode* block = malloc(sizeof(ExpressionBlockNode));
    block->base.base.base.tipo = NODE_EXPRESSION_BLOCK;

    // Reserva espacio para expresiones
    ExpressionNode** expressions = malloc(sizeof(ExpressionNode*) * 64); // ajusta el tamaño si quieres
    int count = 0;

    // Abre bloque
    match(TOKEN_LBRACE);

    // 🚩 Recorre mientras NO veas RBRACE o EOF
    while (current_token.type != TOKEN_RBRACE && current_token.type != TOKEN_EOF) {
        
        // 🚩 Parsear expresión
        ExpressionNode* expr = parse_expr();

        // Si parse_expr falló, rompe para no meter NULLs
        if (!expr) {
            printf("[DEBUG] parse_expr devolvió NULL dentro de bloque\n");
            break;
        }

        // Guarda la expresión en el array
        expressions[count++] = expr;

        // 🚩 Si hay punto y coma, consumirlo
        if (current_token.type == TOKEN_SEMICOLON) {
            match(TOKEN_SEMICOLON);
        }
    }

    // 🚩 Termina con NULL para no recorrer basura después
    expressions[count] = NULL;

    // Cierra bloque
    match(TOKEN_RBRACE);

    // Asigna expresiones
    block->expressions = expressions;

    return (ExpressionNode*) block;
}

ExpressionNode* parse_or_expr() {
    ExpressionNode* left = parse_and_expr();

    while (current_token.type == TOKEN_OR) {
        match(TOKEN_OR);

        OrNode* or_node = malloc(sizeof(OrNode));
        or_node->base.base.base.base.tipo = NODE_OR;
        or_node->base.base.left = left;
        or_node->base.base.right = parse_and_expr();

        left = (ExpressionNode*) or_node;
    }

    return left;
}
ExpressionNode* parse_and_expr() {
    ExpressionNode* left = parse_check_type();

    while (current_token.type == TOKEN_AND) {
        match(TOKEN_AND);

        AndNode* and_node = malloc(sizeof(AndNode));
        and_node->base.base.base.base.tipo = NODE_AND;
        and_node->base.base.left = left;
        and_node->base.base.right = parse_check_type();

        left = (ExpressionNode*) and_node;
    }

    return left;
}
ExpressionNode* parse_check_type() {
    ExpressionNode* left = parse_aritm_comp();

    if (current_token.type == TOKEN_IS) {
        match(TOKEN_IS);
        if (current_token.type != TOKEN_IDENTIFIER) syntax_error("IDENTIFIER");

        CheckTypeNode* node = malloc(sizeof(CheckTypeNode));
        node->base.base.tipo = NODE_CHECK_TYPE;
        node->left = left;

        // Right es VarNode con el tipo
        VarNode* type_var = malloc(sizeof(VarNode));
        type_var->base.base.base.base.tipo = NODE_VAR;
        type_var->base.base.base.base.lexeme = strdup(current_token.lexema);
        type_var->base.base.base.base.row = current_token.line;
        type_var->base.base.base.base.column = current_token.column;
        match(TOKEN_IDENTIFIER);

        node->right = (ExpressionNode*) type_var;

        return (ExpressionNode*) node;
    }

    return left;
}
ExpressionNode* parse_aritm_comp() {
    ExpressionNode* left = parse_concat();

    while (current_token.type == TOKEN_EQUAL_EQUAL ||
           current_token.type == TOKEN_NOT_EQUAL ||
           current_token.type == TOKEN_GREATER ||
           current_token.type == TOKEN_GREATER_EQUAL ||
           current_token.type == TOKEN_LESS ||
           current_token.type == TOKEN_LESS_EQUAL) {

        char* op = current_token.lexema;
        advance();

        ExpressionNode* right = parse_concat();

        BinaryNode* comp_node = malloc(sizeof(BinaryNode));
        comp_node->base.base.tipo = NODE_BINARY;
        comp_node->operator = op;
        comp_node->left = left;
        comp_node->right = right;

        left = (ExpressionNode*) comp_node;
    }

    return left;
}

ExpressionNode* parse_concat() {
    ExpressionNode* left = parse_arithmetic();

    while (current_token.type == TOKEN_AT || current_token.type == TOKEN_AT_AT) {
        int op = current_token.type;
        match(op);

        ExpressionNode* right = parse_arithmetic();

        if (op == TOKEN_AT) {
            ConcatNode* node = malloc(sizeof(ConcatNode));
            node->base.base.base.base.tipo = NODE_CONCAT;
            node->base.base.left = left;
            node->base.base.right = right;
            left = (ExpressionNode*) node;
        } else {
            DoubleConcatNode* node = malloc(sizeof(DoubleConcatNode));
            node->base.base.base.base.tipo = NODE_DOUBLE_CONCAT;
            node->base.base.left = left;
            node->base.base.right = right;
            left = (ExpressionNode*) node;
        }
    }

    return left;
}
ExpressionNode* parse_arithmetic() {
    ExpressionNode* left = parse_term();

    while (current_token.type == TOKEN_PLUS || current_token.type == TOKEN_MINUS) {
        int op = current_token.type;
        match(op);

        ExpressionNode* right = parse_term();

        if (op == TOKEN_PLUS) {
            PlusNode* node = malloc(sizeof(PlusNode));
            node->base.base.base.base.tipo = NODE_PLUS;
            node->base.base.left = left;
            node->base.base.right = right;
            left = (ExpressionNode*) node;
        } else {
            MinusNode* node = malloc(sizeof(MinusNode));
            node->base.base.base.base.tipo = NODE_MINUS;
            node->base.base.left = left;
            node->base.base.right = right;
            left = (ExpressionNode*) node;
        }
    }

    return left;
}
ExpressionNode* parse_term() {
    ExpressionNode* left = parse_pow();

    while (current_token.type == TOKEN_STAR || current_token.type == TOKEN_SLASH || current_token.type == TOKEN_MODULO) {
        int op = current_token.type;
        match(op);

        ExpressionNode* right = parse_pow();

        if (op == TOKEN_STAR) {
            MultNode* node = malloc(sizeof(MultNode));
            node->base.base.base.base.tipo = NODE_MULT;
            node->base.base.left = left;
            node->base.base.right = right;
            left = (ExpressionNode*) node;
        } else if (op == TOKEN_SLASH) {
            DivNode* node = malloc(sizeof(DivNode));
            node->base.base.base.base.tipo = NODE_DIV;
            node->base.base.left = left;
            node->base.base.right = right;
            left = (ExpressionNode*) node;
        } else {
            ModNode* node = malloc(sizeof(ModNode));
            node->base.base.base.base.tipo = NODE_MOD;
            node->base.base.left = left;
            node->base.base.right = right;
            left = (ExpressionNode*) node;
        }
    }

    return left;
}

ExpressionNode* parse_pow() {
    ExpressionNode* left = parse_sign();

    while (current_token.type == TOKEN_POWER || current_token.type == TOKEN_DSTAR) {
        int op = current_token.type;
        match(op);

        ExpressionNode* right = parse_sign();

        PowNode* node = malloc(sizeof(PowNode));
        node->base.base.base.base.tipo = NODE_POW;
        node->base.base.left = left;
        node->base.base.right = right;
        left = (ExpressionNode*) node;
    }

    return left;
}
ExpressionNode* parse_sign() {
    if (current_token.type == TOKEN_MINUS) {
        match(TOKEN_MINUS);
        ExpressionNode* operand = parse_factor();

        NegativeNode* node = malloc(sizeof(NegativeNode));
        node->base.base.base.base.tipo = NODE_NEGATIVE;
        node->base.base.operand = operand;
        return (ExpressionNode*) node;

    } else if (current_token.type == TOKEN_PLUS) {
        match(TOKEN_PLUS);
        ExpressionNode* operand = parse_factor();

        PositiveNode* node = malloc(sizeof(PositiveNode));
        node->base.base.base.base.tipo = NODE_POSITIVE;
        node->base.base.operand = operand;
        return (ExpressionNode*) node;

    } else {
        return parse_factor();
    }
}
ExpressionNode* parse_factor() {
    if (current_token.type == TOKEN_NOT) {
        match(TOKEN_NOT);
        ExpressionNode* operand = parse_atom();

        NotNode* node = malloc(sizeof(NotNode));
        node->base.base.base.base.tipo = NODE_NOT;
        node->base.base.operand = operand;
        return (ExpressionNode*) node;

    } else {
        return parse_atom();
    }
}
ExpressionNode* parse_conditional() {
    match(TOKEN_IF);
    match(TOKEN_LPAREN);
    ExpressionNode* condition = parse_expr();
    match(TOKEN_RPAREN);

    ExpressionNode* if_body = parse_expr();

    ConditionalNode* cond_node = malloc(sizeof(ConditionalNode));
    cond_node->base.base.tipo = NODE_CONDITIONAL;
    cond_node->base.base.row = current_token.line;
    cond_node->base.base.column = current_token.column;

    // Inicializar listas
    cond_node->conditions = malloc(sizeof(ExpressionNode*) * 10);
    cond_node->expressions = malloc(sizeof(ExpressionNode*) * 10);
    cond_node->condition_counter = 0;
    cond_node->expression_counter = 0;

    // Agregar IF
    ((ExpressionNode**)cond_node->conditions)[cond_node->condition_counter++] = condition;
    ((ExpressionNode**)cond_node->expressions)[cond_node->expression_counter++] = if_body;

    // Manejar ELIF y ELSE
    ExpressionNode* else_block = parse_cond_other_case(cond_node);

    cond_node->default_expre = else_block;

    return (ExpressionNode*) cond_node;
}
ExpressionNode* parse_cond_other_case(ConditionalNode* cond_node) {
    if (current_token.type == TOKEN_ELIF) {
        match(TOKEN_ELIF);
        match(TOKEN_LPAREN);
        ExpressionNode* elif_condition = parse_expr();
        match(TOKEN_RPAREN);

        ExpressionNode* elif_body = parse_expr();

        // Agregar ELIF
        ((ExpressionNode**)cond_node->conditions)[cond_node->condition_counter++] = elif_condition;
        ((ExpressionNode**)cond_node->expressions)[cond_node->expression_counter++] = elif_body;

        // Llamada recursiva para más ELIFs o ELSE
        return parse_cond_other_case(cond_node);

    } else if (current_token.type == TOKEN_ELSE) {
        match(TOKEN_ELSE);
        return parse_expr(); // Bloque ELSE
    } else {
        return NULL; // epsilon, no hay else
    }
}

ExpressionNode* parse_while_loop() {
    match(TOKEN_WHILE);
    match(TOKEN_LPAREN);
    ExpressionNode* condition = parse_expr();
    match(TOKEN_RPAREN);
    ExpressionNode* body = parse_expr();

    WhileNode* node = malloc(sizeof(WhileNode));
    node->base.base.tipo = NODE_WHILE;
    node->base.base.row = current_token.line;
    node->base.base.column = current_token.column;

    node->condition = condition;
    node->body = body;

    return (ExpressionNode*) node;
}
ExpressionNode* parse_for_loop() {
    match(TOKEN_FOR);
    match(TOKEN_LPAREN);

    if (current_token.type != TOKEN_IDENTIFIER) syntax_error("IDENTIFIER");
    char* var_name = strdup(current_token.lexema);
    match(TOKEN_IDENTIFIER);

    match(TOKEN_IN);

    ExpressionNode* iterable = parse_expr();

    match(TOKEN_RPAREN);

    ExpressionNode* body = parse_expr();

    ForNode* node = malloc(sizeof(ForNode));
    node->base.base.tipo = NODE_FOR;
    node->base.base.row = current_token.line;
    node->base.base.column = current_token.column;

    node->item = var_name;
    node->iterable = iterable;
    node->body = body;

    return (ExpressionNode*) node;
}
ExpressionNode* parse_let_expr() {
    match(TOKEN_LET);

    LetInNode* let_node = malloc(sizeof(LetInNode));
    let_node->base.base.tipo = NODE_LET_IN;
    let_node->base.base.row = current_token.line;
    let_node->base.base.column = current_token.column;

    // Inicializa lista de VarDeclarationNode*
    let_node->variables = malloc(sizeof(VarDeclarationNode*) * 10);
    let_node->variable_counter = 0;

    parse_var_declaration_list(let_node);
    ((VarDeclarationNode**)let_node->variables)[let_node->variable_counter] = NULL;

    match(TOKEN_IN);

    let_node->body = parse_expr();

    return (ExpressionNode*) let_node;
}
void parse_var_declaration_list(LetInNode* let_node) {
    if (current_token.type == TOKEN_IDENTIFIER) {
        VarDeclarationNode* var_decl = malloc(sizeof(VarDeclarationNode));
        var_decl->base.base.tipo = NODE_VAR_DECLARATION;
        var_decl->base.base.row = current_token.line;
        var_decl->base.base.column = current_token.column;

        var_decl->name = strdup(current_token.lexema);
        match(TOKEN_IDENTIFIER);

        if (current_token.type == TOKEN_PUNTOS) {
            // Soporta tipo: IDENTIFIER : IDENTIFIER = Expr
            match(TOKEN_PUNTOS);
            if (current_token.type != TOKEN_IDENTIFIER) syntax_error("IDENTIFIER");
            var_decl->type = strdup(current_token.lexema);
            match(TOKEN_IDENTIFIER);
        } else {
            var_decl->type = NULL;
        }

        if (current_token.type == TOKEN_ASSIGN || current_token.type == TOKEN_COLON_EQUAL) {
            match(current_token.type);
        } else {
            syntax_error("ASSIGN or COLON_EQUAL expected");
        }

        var_decl->value = parse_expr();

        ((VarDeclarationNode**)let_node->variables)[let_node->variable_counter++] = var_decl;

        if (current_token.type == TOKEN_COMMA) {
            match(TOKEN_COMMA);
            parse_var_declaration_list(let_node);
        }
        // else: fin
    }
    // else: epsilon
}
ExpressionNode* parse_call_func() {
    if (current_token.type != TOKEN_IDENTIFIER) syntax_error("Expected IDENTIFIER");

    char* func_name = strdup(current_token.lexema);
    match(TOKEN_IDENTIFIER);

    match(TOKEN_LPAREN);

    CallFuncNode* call_node = malloc(sizeof(CallFuncNode));
    call_node->base.base.base.tipo = NODE_CALL_FUNC;
    call_node->base.base.base.row = current_token.line;
    call_node->base.base.base.column = current_token.column;

    call_node->name = func_name;

    // Inicializa lista de argumentos
    call_node->arguments = malloc(sizeof(ExpressionNode*) * 10);
    call_node->arguments_counter = 0;

    if (current_token.type != TOKEN_RPAREN) {
        parse_arguments(call_node);
    }

    match(TOKEN_RPAREN);

    return (ExpressionNode*) call_node;
}

void parse_arguments(CallFuncNode* call_node) {
    ExpressionNode* arg = parse_expr();
    ((ExpressionNode**)call_node->arguments)[call_node->arguments_counter++] = arg;

    while (current_token.type == TOKEN_COMMA) {
        match(TOKEN_COMMA);
        ExpressionNode* next_arg = parse_expr();
        ((ExpressionNode**)call_node->arguments)[call_node->arguments_counter++] = next_arg;
    }
}
ExpressionNode* parse_index_object(ExpressionNode* atom) {
    match(TOKEN_LBRACKET);
    ExpressionNode* index = parse_expr();
    match(TOKEN_RBRACKET);

    IndexObjectNode* node = malloc(sizeof(IndexObjectNode));
    node->base.base.base.tipo = NODE_INDEX_OBJECT;
    node->base.base.base.row = current_token.line;
    node->base.base.base.column = current_token.column;

    node->object = atom;
    node->pos = index;

    return (ExpressionNode*) node;
}
ExpressionNode* parse_vector() {
    match(TOKEN_LBRACKET);

    if (current_token.type == TOKEN_RBRACKET) {
        match(TOKEN_RBRACKET);
        ExplicitVectorNode* vec = malloc(sizeof(ExplicitVectorNode));
        vec->base.base.base.tipo = NODE_EXPLICIT_VECTOR;
        vec->items = NULL;
        vec->item_counter = 0;
        return (ExpressionNode*) vec;
    }

    // Lookahead para vector implícito: [ expr FOR IDENTIFIER IN expr ]
    if (current_token.type == TOKEN_FOR) {
        // [ expr FOR IDENTIFIER IN expr ]
        ExpressionNode* expr = parse_expr();
        match(TOKEN_FOR);

        if (current_token.type != TOKEN_IDENTIFIER) syntax_error("IDENTIFIER");
        char* item_name = strdup(current_token.lexema);
        match(TOKEN_IDENTIFIER);

        match(TOKEN_IN);

        ExpressionNode* iterable = parse_expr();

        match(TOKEN_RBRACKET);

        ImplicitVectorNode* vec = malloc(sizeof(ImplicitVectorNode));
        vec->base.base.base.tipo = NODE_IMPLICIT_VECTOR;

        vec->expr = expr;

        VarNode* var_item = malloc(sizeof(VarNode));
        var_item->base.base.base.base.tipo = NODE_VAR;
        var_item->base.base.base.base.lexeme = item_name;

        vec->item = var_item;
        vec->iterable = iterable;

        return (ExpressionNode*) vec;
    }

    // Explícito con lista: [ expr, expr, ... ]
    ExplicitVectorNode* vec = malloc(sizeof(ExplicitVectorNode));
    vec->base.base.base.tipo = NODE_EXPLICIT_VECTOR;

    vec->items = malloc(sizeof(ExpressionNode*) * 10);
    vec->item_counter = 0;

    ExpressionNode* item = parse_expr();
    ((ExpressionNode**)vec->items)[vec->item_counter++] = item;

    while (current_token.type == TOKEN_COMMA) {
        match(TOKEN_COMMA);
        ExpressionNode* next_item = parse_expr();
        ((ExpressionNode**)vec->items)[vec->item_counter++] = next_item;
    }

    match(TOKEN_RBRACKET);
    return (ExpressionNode*) vec;
}

ExpressionNode* parse_member(ExpressionNode* atom) {
    match(TOKEN_DOT);

    if (current_token.type != TOKEN_IDENTIFIER)
        syntax_error("Expected IDENTIFIER after '.'");

    char* member_name = strdup(current_token.lexema);
    match(TOKEN_IDENTIFIER);

    if (lookahead_token_is_call()) {
        // obj.method()
        match(TOKEN_LPAREN);

        ExpressionNode** args = malloc(sizeof(ExpressionNode*) * 10);
        int arg_count = 0;

        if (current_token.type != TOKEN_RPAREN) {
            do {
                ExpressionNode* arg = parse_expr();
                args[arg_count++] = arg;

                if (current_token.type == TOKEN_COMMA)
                    match(TOKEN_COMMA);
                else
                    break;
            } while (1);
        }
        args[arg_count] = NULL;

        match(TOKEN_RPAREN);

        CallMethodNode* node = malloc(sizeof(CallMethodNode));
        node->base.base.base.tipo = NODE_CALL_METHOD;
        node->inst = atom;
        node->method_name = member_name;
        node->method_args = args;
        node->method_args_counter = arg_count;

        return (ExpressionNode*) node;

    } else {
        // obj.attr
        MemberNode* node = malloc(sizeof(MemberNode));
        node->base.base.base.tipo = NODE_TYPE_ATTRIBUTE;
        node->object = atom;
        node->member = member_name;
        return (ExpressionNode*) node;
    }
}

ExpressionNode* parse_expr() {
   if (current_token.type == TOKEN_IF) {
      return parse_conditional();
   } else if (current_token.type == TOKEN_WHILE) {
      return parse_while_loop();
   } else if (current_token.type == TOKEN_FOR) {
      return parse_for_loop();
   } else if (current_token.type == TOKEN_LET) {
      return parse_let_expr();

   } else if (current_token.type == TOKEN_LBRACE) {
      return parse_expr_block();  
   }
   else if (current_token.type == TOKEN_PRINT) {
   return parse_print_expr();
}
else {
      return parse_or_expr();
   }
}
ExpressionNode* parse_print_expr() {
    match(TOKEN_PRINT);
    match(TOKEN_LPAREN);
    ExpressionNode* inner = parse_expr();
    match(TOKEN_RPAREN);

    PrintNode* node = malloc(sizeof(PrintNode));
    node->base.base.tipo = NODE_PRINT;
    node->value = inner;

    return (ExpressionNode*) node;
}


ExpressionNode* parse_atom() {
    ExpressionNode* result = NULL;

    if (current_token.type == TOKEN_LPAREN) {
        match(TOKEN_LPAREN);
        result = parse_expr();
        match(TOKEN_RPAREN);

    } if (current_token.type == TOKEN_NEW) {
        match(TOKEN_NEW);

        if (current_token.type != TOKEN_IDENTIFIER)
            syntax_error("Expected IDENTIFIER after 'new'");

        char* type_name = strdup(current_token.lexema);
        match(TOKEN_IDENTIFIER);

        match(TOKEN_LPAREN);

        // Soporta argumentos opcionales
        ExpressionNode** args = malloc(sizeof(ExpressionNode*) * 10);
        int arg_count = 0;

        if (current_token.type != TOKEN_RPAREN) {
            // parse arguments
            do {
                ExpressionNode* arg = parse_expr();
                args[arg_count++] = arg;

                if (current_token.type == TOKEN_COMMA)
                    match(TOKEN_COMMA);
                else
                    break;

            } while (1);
        }
        args[arg_count] = NULL;

        match(TOKEN_RPAREN);

        CallTypeConstructorNode* node = malloc(sizeof(CallTypeConstructorNode));
        node->base.tipo = NODE_CALL_TYPE_CONSTRUCTOR;
        node->type_name = type_name;
        node->arguments = args;
        node->arguments_counter = arg_count;

        result = (ExpressionNode*)node;
    }

    else if (current_token.type == TOKEN_TRUE || current_token.type == TOKEN_FALSE) {
        BooleanNode* node = malloc(sizeof(BooleanNode));
        node->base.base.base.base.tipo = NODE_BOOLEAN;
        node->base.base.base.base.row = current_token.line;
        node->base.base.base.base.column = current_token.column;
        node->base.lex = strdup(current_token.lexema);
        match(current_token.type);
        result = (ExpressionNode*) node;

    } else if (current_token.type == TOKEN_NUMBER) {
        NumberNode* node = malloc(sizeof(NumberNode));
        node->base.base.base.base.tipo = NODE_NUMBER;
        node->base.base.base.base.row = current_token.line;
        node->base.base.base.base.column = current_token.column;
        node->base.lex = strdup(current_token.lexema);
        match(TOKEN_NUMBER);
        result = (ExpressionNode*) node;

    } else if (current_token.type == TOKEN_STRING) {
        StringNode* node = malloc(sizeof(StringNode));
        node->base.base.base.base.tipo = NODE_STRING;
        node->base.base.base.base.row = current_token.line;
        node->base.base.base.base.column = current_token.column;
        node->base.lex = strdup(current_token.lexema);
        match(TOKEN_STRING);
        result = (ExpressionNode*) node;

    } else if (current_token.type == TOKEN_IDENTIFIER) {
   printf("[DEBUG] IDENTIFIER atom: '%s'\n", current_token.lexema);

   if (lookahead_token_is_call()) {
     printf("[DEBUG] Es llamada a función\n");
     result = parse_call_func();
   } else {
     VarNode* node = malloc(sizeof(VarNode));
     node->base.base.base.base.tipo = NODE_VAR;
     node->base.base.base.base.lexeme = strdup(current_token.lexema);
     printf("[DEBUG] VarNode creado con lexeme='%s'\n", node->base.base.base.base.lexeme);
     match(TOKEN_IDENTIFIER);
     result = (ExpressionNode*) node;
   }



    } else if (current_token.type == TOKEN_LBRACKET) {
        result = parse_vector();  // Explicit o Implicit

    } else {
        syntax_error("Invalid atom");
        return NULL;
    }

    // 📌 Postfix: indexación o miembro
    while (current_token.type == TOKEN_LBRACKET || current_token.type == TOKEN_DOT) {
        if (current_token.type == TOKEN_LBRACKET) {
            result = parse_index_object(result);
        } else if (current_token.type == TOKEN_DOT) {
            result = parse_member(result);
        }
    }

    return result;
}

DeclarationNode* parse_type() {
    match(TOKEN_TYPE);

    TypeDeclarationNode* type_node = malloc(sizeof(TypeDeclarationNode));
    type_node->base.base.tipo = NODE_TYPE_DECLARATION;
    type_node->base.base.row = current_token.line;
    type_node->base.base.column = current_token.column;

    // Nombre del tipo
    if (current_token.type != TOKEN_IDENTIFIER) syntax_error("Expected IDENTIFIER");
    type_node->name = strdup(current_token.lexema);
    type_node->base.base.lexeme = strdup(current_token.lexema);
    match(TOKEN_IDENTIFIER);

    // Opcional: Signature (Params)
    if (current_token.type == TOKEN_LPAREN) {
        match(TOKEN_LPAREN);

        type_node->params = malloc(sizeof(char*) * 10);
        type_node->param_count = 0;

        if (current_token.type != TOKEN_RPAREN) {
            parse_params(type_node);
        }

        match(TOKEN_RPAREN);
    } else {
        type_node->params = NULL;
        type_node->param_count = 0;
    }

    // Opcional: inherits IDENTIFIER (padre)
    if (current_token.type == TOKEN_INHERITS) {
        match(TOKEN_INHERITS);

        if (current_token.type != TOKEN_IDENTIFIER) syntax_error("Expected IDENTIFIER");
        type_node->parent = strdup(current_token.lexema);
        match(TOKEN_IDENTIFIER);

        // Opcional: argumentos del padre (parent_args)
        if (current_token.type == TOKEN_LPAREN) {
            match(TOKEN_LPAREN);

            type_node->parent_args = malloc(sizeof(ExpressionNode*) * 10);
            type_node->parent_args_count = 0;

            if (current_token.type != TOKEN_RPAREN) {
                parse_parent_args(type_node);
            }

            match(TOKEN_RPAREN);
        } else {
            type_node->parent_args = NULL;
            type_node->parent_args_count = 0;
        }
    } else {
        type_node->parent = strdup("Object");
        type_node->parent_args = NULL;
        type_node->parent_args_count = 0;
    }

    // Bloque { attributes methods }
    match(TOKEN_LBRACE);

    type_node->attributes = malloc(sizeof(TypeAttributeNode*) * 10);
    type_node->attribute_counter = 0;

    type_node->methods = malloc(sizeof(MethodDeclarationNode*) * 10);
    type_node->method_counter = 0;

    while (current_token.type != TOKEN_RBRACE) {
        if (current_token.type == TOKEN_IDENTIFIER) {
            // Atributo: IDENTIFIER ASSIGN Expr SEMICOLON
            TypeAttributeNode* attr = malloc(sizeof(TypeAttributeNode));
            attr->base.base.tipo = NODE_TYPE_ATTRIBUTE;
            attr->base.base.row = current_token.line;
            attr->base.base.column = current_token.column;

            attr->name = strdup(current_token.lexema);
            match(TOKEN_IDENTIFIER);

            match(TOKEN_ASSIGN);

            attr->value = parse_expr();

            match(TOKEN_SEMICOLON);

            ((TypeAttributeNode**)type_node->attributes)[type_node->attribute_counter++] = attr;

        } else if (current_token.type == TOKEN_FUNCTION) {
            // Método: FUNCTION ...
            MethodDeclarationNode* method = parse_method();
            ((MethodDeclarationNode**)type_node->methods)[type_node->method_counter++] = method;
        } else {
            syntax_error("Invalid type member");
        }
    }

    match(TOKEN_RBRACE);

    return (DeclarationNode*) type_node;
}
void parse_params(TypeDeclarationNode* type_node) {
    if (current_token.type != TOKEN_IDENTIFIER) syntax_error("Expected IDENTIFIER");

    ((char**)type_node->params)[type_node->param_count++] = strdup(current_token.lexema);
    match(TOKEN_IDENTIFIER);

    while (current_token.type == TOKEN_COMMA) {
        match(TOKEN_COMMA);
        if (current_token.type != TOKEN_IDENTIFIER) syntax_error("Expected IDENTIFIER");
        ((char**)type_node->params)[type_node->param_count++] = strdup(current_token.lexema);
        match(TOKEN_IDENTIFIER);
    }
}
void parse_parent_args(TypeDeclarationNode* type_node) {
    ExpressionNode* arg = parse_expr();
    ((ExpressionNode**)type_node->parent_args)[type_node->parent_args_count++] = arg;

    while (current_token.type == TOKEN_COMMA) {
        match(TOKEN_COMMA);
        ExpressionNode* next_arg = parse_expr();
        ((ExpressionNode**)type_node->parent_args)[type_node->parent_args_count++] = next_arg;
    }
}
MethodDeclarationNode* parse_method() {
    match(TOKEN_FUNCTION);

    MethodDeclarationNode* method = malloc(sizeof(MethodDeclarationNode));
    method->base.base.tipo = NODE_METHOD_DECLARATION;
    method->base.base.row = current_token.line;
    method->base.base.column = current_token.column;

    if (current_token.type != TOKEN_IDENTIFIER) syntax_error("Expected IDENTIFIER");
    method->name = strdup(current_token.lexema);
    match(TOKEN_IDENTIFIER);

    match(TOKEN_LPAREN);

    method->params = malloc(sizeof(char*) * 10);
    method->param_counter = 0;

    if (current_token.type != TOKEN_RPAREN) {
        parse_method_params(method);
    }

    match(TOKEN_RPAREN);

    method->body = parse_expr_block();  // o lo que uses como cuerpo

    return method;
}

void parse_method_params(MethodDeclarationNode* method) {
    if (current_token.type != TOKEN_IDENTIFIER) syntax_error("Expected IDENTIFIER");

    ((char**)method->params)[method->param_counter++] = strdup(current_token.lexema);
    match(TOKEN_IDENTIFIER);

    while (current_token.type == TOKEN_COMMA) {
        match(TOKEN_COMMA);
        if (current_token.type != TOKEN_IDENTIFIER) syntax_error("Expected IDENTIFIER");
        ((char**)method->params)[method->param_counter++] = strdup(current_token.lexema);
        match(TOKEN_IDENTIFIER);
    }
}
