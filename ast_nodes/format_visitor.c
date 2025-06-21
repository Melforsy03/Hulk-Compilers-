#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast_nodes.h"

char* visit(Node* node, int tabs);
char* visit_ProgramNode(ProgramNode* node, int tabs);
char* visit_MethodSignatureNode(MethodSignatureNode* node, int tabs);
char* visit_FunctionDeclarationNode(FunctionDeclarationNode* node, int tabs);
char* visit_MethodDeclarationNode(MethodDeclarationNode* node, int tabs);
char* visit_TypeAttributeNode(TypeAttributeNode* node, int tabs);
char* visit_TypeDeclarationNode(TypeDeclarationNode* node, int tabs);
char* visit_ProtocolDeclarationNode(ProtocolDeclarationNode* node, int tabs);
char* visit_VarDeclarationNode(VarDeclarationNode* node, int tabs);
char* visit_ConditionalNode(ConditionalNode* node, int tabs);
char* visit_LetInNode(LetInNode* node, int tabs);
char* visit_WhileNode(WhileNode* node, int tabs);
char* visit_ForNode(ForNode* node, int tabs);
char* visit_DestrNode(DestrNode* node, int tabs);
char* visit_BinaryNode(BinaryNode* node, int tabs, const char* class_name);
char* visit_UnaryNode(UnaryNode* node, int tabs, const char* class_name);
char* visit_LiteralNode(LiteralNode* node, int tabs, const char* class_name);
char* visit_ExpressionBlockNode(ExpressionBlockNode* node, int tabs);
char* visit_CallFuncNode(CallFuncNode* node, int tabs);
char* visit_TypeInstantiationNode(TypeInstantiationNode* node, int tabs);
char* visit_ExplicitVectorNode(ExplicitVectorNode* node, int tabs);
char* visit_ImplicitVectorNode(ImplicitVectorNode* node, int tabs);
char* visit_IndexObjectNode(IndexObjectNode* node, int tabs);
char* visit_CallMethodNode(CallMethodNode* node, int tabs);
char* visit_CallTypeAttributeNode(CallTypeAttributeNode* node, int tabs);
char* visit_CastTypeNode(CastTypeNode* node, int tabs);
char* visit_CheckTypeNode(CheckTypeNode* node, int tabs);

// Funciones auxiliares
char* create_indentation(int tabs) {
    char* indent = malloc(tabs + 1);
    for (int i = 0; i < tabs; i++) {
        indent[i] = '\t';
    }
    indent[tabs] = '\0';
    return indent;
}

char* join_strings(const char** strings, int count, const char* separator) {
    if (count == 0) return strdup("");
    
    size_t total_length = 0;
    for (int i = 0; i < count; i++) {
        total_length += strlen(strings[i]);
    }
    total_length += strlen(separator) * (count - 1) + 1;
    
    char* result = malloc(total_length);
    result[0] = '\0';
    
    for (int i = 0; i < count; i++) {
        strcat(result, strings[i]);
        if (i < count - 1) {
            strcat(result, separator);
        }
    }
    
    return result;
}

// Función principal de visita
char* visit(Node* node, int tabs) {
    if (node == NULL) return strdup("");
    
    switch (node->tipo) {
        case NODE_PROGRAM:
            return visit_ProgramNode((ProgramNode*)node, tabs);
        case NODE_FUNCTION_DECLARATION:
            return visit_FunctionDeclarationNode((FunctionDeclarationNode*)node, tabs);
        case NODE_TYPE_DECLARATION:
            return visit_TypeDeclarationNode((TypeDeclarationNode*)node, tabs);
        case NODE_LET_IN:
            return visit_LetInNode((LetInNode*)node, tabs);
        case NODE_CONDITIONAL:
            return visit_ConditionalNode((ConditionalNode*)node, tabs);
        case NODE_WHILE:
            return visit_WhileNode((WhileNode*)node, tabs);
        case NODE_FOR:
            return visit_ForNode((ForNode*)node, tabs);
        case NODE_RETURN:
            return visit_UnaryNode((UnaryNode*)node, tabs, "ReturnNode");
        case NODE_PLUS:
            return visit_BinaryNode((BinaryNode*)node, tabs, "PlusNode");
        case NODE_MINUS:
            return visit_BinaryNode((BinaryNode*)node, tabs, "MinusNode");
        case NODE_MULT:
            return visit_BinaryNode((BinaryNode*)node, tabs, "MultNode");
        case NODE_DIV:
            return visit_BinaryNode((BinaryNode*)node, tabs, "DivNode");
        case NODE_POW:
            return visit_BinaryNode((BinaryNode*)node, tabs, "PowNode");
        case NODE_MOD:
            return visit_BinaryNode((BinaryNode*)node, tabs, "ModNode");
        case NODE_EQUAL:
            return visit_BinaryNode((BinaryNode*)node, tabs, "EqualNode");
        case NODE_NOT_EQUAL:
            return visit_BinaryNode((BinaryNode*)node, tabs, "NotEqualNode");
        case NODE_LESS:
            return visit_BinaryNode((BinaryNode*)node, tabs, "LessNode");
        case NODE_LESS_EQUAL:
            return visit_BinaryNode((BinaryNode*)node, tabs, "LessEqualNode");
        case NODE_GREATER:
            return visit_BinaryNode((BinaryNode*)node, tabs, "GreaterNode");
        case NODE_GREATER_EQUAL:
            return visit_BinaryNode((BinaryNode*)node, tabs, "GreaterEqualNode");
        case NODE_AND:
            return visit_BinaryNode((BinaryNode*)node, tabs, "AndNode");
        case NODE_OR:
            return visit_BinaryNode((BinaryNode*)node, tabs, "OrNode");
        case NODE_NOT:
            return visit_UnaryNode((UnaryNode*)node, tabs, "NotNode");
        case NODE_VAR:
            return visit_LiteralNode((LiteralNode*)node, tabs, "VarNode");
        case NODE_NUMBER:
            return visit_LiteralNode((LiteralNode*)node, tabs, "NumberNode");
        case NODE_STRING:
            return visit_LiteralNode((LiteralNode*)node, tabs, "StringNode");
        case NODE_BOOLEAN:
            return visit_LiteralNode((LiteralNode*)node, tabs, "BooleanNode");
        case NODE_EXPRESSION_BLOCK:
            return visit_ExpressionBlockNode((ExpressionBlockNode*)node, tabs);
        case NODE_CALL_FUNC:
            return visit_CallFuncNode((CallFuncNode*)node, tabs);
        case NODE_TYPE_INSTANTIATION:
            return visit_TypeInstantiationNode((TypeInstantiationNode*)node, tabs);
        case NODE_INDEX_OBJECT:
            return visit_IndexObjectNode((IndexObjectNode*)node, tabs);
        case NODE_CALL_METHOD:
            return visit_CallMethodNode((CallMethodNode*)node, tabs);
        case NODE_CONCAT:
            return visit_BinaryNode((BinaryNode*)node, tabs, "ConcatNode");
        case NODE_PRINT:
            return visit_UnaryNode((UnaryNode*)node, tabs, "PrintNode");
        case NODE_ASSING:
            return visit_BinaryNode((BinaryNode*)node, tabs, "AssignNode");
        case NODE_COLON_ASSING:
            return visit_BinaryNode((BinaryNode*)node, tabs, "ColonAssignNode");
        case NODE_SELF:
            return strdup("\\__SelfNode");
        case NODE_BASE:
            return strdup("\\__BaseNode");
        case NODE_INHERITS:
            return visit_BinaryNode((BinaryNode*)node, tabs, "InheritsNode");
        default:
            return strdup("Unknown node type");
    }
}

// Implementaciones específicas para cada tipo de nodo

char* visit_ProgramNode(ProgramNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    char* ans = malloc(strlen(indent) + 100);
    sprintf(ans, "%s\\__ProgramNode [<decl>, ...,<decl>] <expr>", indent);
    free(indent);
    
    // Asumimos que node->declarations es un array de Node* terminado en NULL
    char** decl_strings = NULL;
    int decl_count = 0;
    if (node->declarations != NULL) {
        Node** decls = (Node**)node->declarations;
        while (*decls != NULL) {
            decl_count++;
            decls++;
        }
        
        decl_strings = malloc(decl_count * sizeof(char*));
        decls = (Node**)node->declarations;
        for (int i = 0; i < decl_count; i++) {
            decl_strings[i] = visit(decls[i], tabs + 1);
        }
    }
    
    char* declarations = join_strings((const char**)decl_strings, decl_count, "\n");
    char* expression = visit(node->expression, tabs + 1);
    
    char* result = malloc(strlen(ans) + strlen(declarations) + strlen(expression) + 3);
    sprintf(result, "%s\n%s\n%s", ans, declarations, expression);
    
    free(ans);
    for (int i = 0; i < decl_count; i++) {
        free(decl_strings[i]);
    }
    free(decl_strings);
    free(declarations);
    free(expression);
    
    return result;
}

char* visit_MethodSignatureNode(MethodSignatureNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    
    // Procesar parámetros (asumimos que es una lista de ParameterNode)
    char* params_str = "[params]";
    if (node->params != NULL) {
        params_str = "param1:type1, param2:type2";
    }
    
    char* ans = malloc(strlen(indent) + strlen(node->name) + strlen(params_str) + 
                (node->returnType ? strlen(node->returnType) : 4) + 50);
    sprintf(ans, "%s\\__MethodSignatureNode: %s(%s) : %s", 
            indent, node->name, params_str, 
            node->returnType ? node->returnType : "None");
    free(indent);
    
    return ans;
}

char* visit_FunctionDeclarationNode(FunctionDeclarationNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    
    // Procesar parámetros
    char* params_str = "[params]";
    if (node->params != NULL) {
        params_str = "param1:type1, param2:type2";
    }
    
    char* ans = malloc(strlen(indent) + strlen(node->name) + strlen(params_str) + 
                (node->returnType ? strlen(node->returnType) : 4) + 100);
    sprintf(ans, "%s\\__FunctionDeclarationNode: %s(%s): %s -> <expr>", 
            indent, node->name, params_str, 
            node->returnType ? node->returnType : "None");
    free(indent);
    
    char* body = visit(node->body, tabs + 1);
    char* result = malloc(strlen(ans) + strlen(body) + 2);
    sprintf(result, "%s\n%s", ans, body);
    
    free(ans);
    free(body);
    
    return result;
}

char* visit_MethodDeclarationNode(MethodDeclarationNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    
    // Similar a FunctionDeclarationNode
    char* params_str = "[params]";
    if (node->params != NULL) {
        params_str = "param1:type1, param2:type2";
    }
    
    char* ans = malloc(strlen(indent) + strlen(node->name) + strlen(params_str) + 
                (node->returnType ? strlen(node->returnType) : 4) + 100);
    sprintf(ans, "%s\\__MethodDeclarationNode: %s(%s): %s -> <expr>", 
            indent, node->name, params_str, 
            node->returnType ? node->returnType : "None");
    free(indent);
    
    char* body = visit(node->body, tabs + 1);
    char* result = malloc(strlen(ans) + strlen(body) + 2);
    sprintf(result, "%s\n%s", ans, body);
    
    free(ans);
    free(body);
    
    return result;
}

char* visit_TypeAttributeNode(TypeAttributeNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    char* ans = malloc(strlen(indent) + strlen(node->name) + 
                (node->type ? strlen(node->type) : 4) + 50);
    sprintf(ans, "%s\\__TypeAttributeNode: %s : %s = <expr>", 
            indent, node->name, node->type ? node->type : "None");
    free(indent);
    
    char* expr = visit(node->value, tabs + 1);
    char* result = malloc(strlen(ans) + strlen(expr) + 2);
    sprintf(result, "%s\n%s", ans, expr);
    
    free(ans);
    free(expr);
    
    return result;
}

char* visit_TypeDeclarationNode(TypeDeclarationNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    
    // Procesar parámetros
    char* params_str = "[params]";
    if (node->params != NULL) {
        params_str = "param1:type1, param2:type2";
    }
    
    char* ans = malloc(strlen(indent) + strlen(node->name) + strlen(params_str) + 
                strlen(node->parent) + 150);
    sprintf(ans, "%s\\__TypeDeclarationNode: %s(%s) inherits %s (<expr>,...,<expr>) -> <body>", 
            indent, node->name, params_str, node->parent);
    free(indent);
    
    // Procesar parent_args (asumimos que es una lista de ExpressionNode*)
    char** parent_arg_strings = NULL;
    int parent_arg_count = 0;
    if (node->parent_args != NULL) {
        ExpressionNode** args = (ExpressionNode**)node->parent_args;
        while (*args != NULL) {
            parent_arg_count++;
            args++;
        }
        
        parent_arg_strings = malloc(parent_arg_count * sizeof(char*));
        args = (ExpressionNode**)node->parent_args;
        for (int i = 0; i < parent_arg_count; i++) {
            parent_arg_strings[i] = visit((Node*)args[i], tabs + 1);
        }
    }
    char* parent_args = join_strings((const char**)parent_arg_strings, parent_arg_count, "\n");
    
    // Procesar attributes (asumimos que es una lista de TypeAttributeNode*)
    char** attr_strings = NULL;
    int attr_count = 0;
    if (node->attributes != NULL) {
        TypeAttributeNode** attrs = (TypeAttributeNode**)node->attributes;
        while (*attrs != NULL) {
            attr_count++;
            attrs++;
        }
        
        attr_strings = malloc(attr_count * sizeof(char*));
        attrs = (TypeAttributeNode**)node->attributes;
        for (int i = 0; i < attr_count; i++) {
            attr_strings[i] = visit((Node*)attrs[i], tabs + 1);
        }
    }
    char* attributes = join_strings((const char**)attr_strings, attr_count, "\n");
    
    // Procesar methods (asumimos que es una lista de MethodDeclarationNode*)
    char** method_strings = NULL;
    int method_count = 0;
    if (node->methods != NULL) {
        MethodDeclarationNode** methods = (MethodDeclarationNode**)node->methods;
        while (*methods != NULL) {
            method_count++;
            methods++;
        }
        
        method_strings = malloc(method_count * sizeof(char*));
        methods = (MethodDeclarationNode**)node->methods;
        for (int i = 0; i < method_count; i++) {
            method_strings[i] = visit((Node*)methods[i], tabs + 1);
        }
    }
    char* methods = join_strings((const char**)method_strings, method_count, "\n");
    
    // Construir resultado final
    size_t result_len = strlen(ans) + strlen(parent_args) + strlen(attributes) + 
                       strlen(methods) + 4;
    char* result = malloc(result_len);
    sprintf(result, "%s\n%s\n%s\n%s", ans, parent_args, attributes, methods);
    
    // Liberar memoria
    free(ans);
    for (int i = 0; i < parent_arg_count; i++) free(parent_arg_strings[i]);
    free(parent_arg_strings);
    free(parent_args);
    for (int i = 0; i < attr_count; i++) free(attr_strings[i]);
    free(attr_strings);
    free(attributes);
    for (int i = 0; i < method_count; i++) free(method_strings[i]);
    free(method_strings);
    free(methods);
    
    return result;
}

char* visit_ProtocolDeclarationNode(ProtocolDeclarationNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    char* ans = malloc(strlen(indent) + strlen(node->name) + 
                (node->parent ? strlen(node->parent) : 4) + 50);
    sprintf(ans, "%s\\__ProtocolDeclarationNode: %s extends %s -> body", 
            indent, node->name, node->parent ? node->parent : "None");
    free(indent);
    
    // Procesar methods_signature (asumimos lista de MethodSignatureNode*)
    char** method_strings = NULL;
    int method_count = 0;
    if (node->methods_signature != NULL) {
        MethodSignatureNode** methods = (MethodSignatureNode**)node->methods_signature;
        while (*methods != NULL) {
            method_count++;
            methods++;
        }
        
        method_strings = malloc(method_count * sizeof(char*));
        methods = (MethodSignatureNode**)node->methods_signature;
        for (int i = 0; i < method_count; i++) {
            method_strings[i] = visit((Node*)methods[i], tabs + 1);
        }
    }
    char* methods = join_strings((const char**)method_strings, method_count, "\n");
    
    char* result = malloc(strlen(ans) + strlen(methods) + 2);
    sprintf(result, "%s\n%s", ans, methods);
    
    free(ans);
    for (int i = 0; i < method_count; i++) free(method_strings[i]);
    free(method_strings);
    free(methods);
    
    return result;
}

char* visit_VarDeclarationNode(VarDeclarationNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    char* ans = malloc(strlen(indent) + strlen(node->name) + 
                (node->type ? strlen(node->type) : 4) + 50);
    sprintf(ans, "%s\\__VarDeclarationNode: %s : %s = <expr>", 
            indent, node->name, node->type ? node->type : "None");
    free(indent);
    
    char* expr = visit(node->value, tabs + 1);
    char* result = malloc(strlen(ans) + strlen(expr) + 2);
    sprintf(result, "%s\n%s", ans, expr);
    
    free(ans);
    free(expr);
    
    return result;
}

char* visit_ConditionalNode(ConditionalNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    char* ans = malloc(strlen(indent) + 100);
    sprintf(ans, "%s\\__ConditionalNode: [<expr>,...,<expr>] [<expr>,...,<expr>] <expr>", indent);
    free(indent);
    
    // Procesar condiciones y expresiones (asumimos listas paralelas)
    char** condition_expr_strings = NULL;
    int condition_count = 0;
    if (node->conditions != NULL && node->expressions != NULL) {
        ExpressionNode** conds = (ExpressionNode**)node->conditions;
        ExpressionNode** exprs = (ExpressionNode**)node->expressions;
        while (*conds != NULL && *exprs != NULL) {
            condition_count++;
            conds++;
            exprs++;
        }
        
        condition_expr_strings = malloc(condition_count * 2 * sizeof(char*));
        conds = (ExpressionNode**)node->conditions;
        exprs = (ExpressionNode**)node->expressions;
        for (int i = 0; i < condition_count; i++) {
            char* cond_indent = create_indentation(tabs + 1);
            condition_expr_strings[i*2] = malloc(strlen(cond_indent) + 20);
            sprintf(condition_expr_strings[i*2], "%sCondition:", cond_indent);
            free(cond_indent);
            
            condition_expr_strings[i*2+1] = visit((Node*)conds[i], tabs + 2);
            
            char* expr_indent = create_indentation(tabs + 1);
            condition_expr_strings[i*2+2] = malloc(strlen(expr_indent) + 20);
            sprintf(condition_expr_strings[i*2+2], "%sThen:", expr_indent);
            free(expr_indent);
            
            condition_expr_strings[i*2+3] = visit((Node*)exprs[i], tabs + 2);
        }
    }
    char* conditions_exprs = join_strings((const char**)condition_expr_strings, condition_count * 4, "\n");
    
    // Procesar default
    char* default_indent = create_indentation(tabs + 1);
    char* default_header = malloc(strlen(default_indent) + 20);
    sprintf(default_header, "%sDefault:", default_indent);
    free(default_indent);
    
    char* default_expr = visit(node->default_expre, tabs + 2);
    
    char* default_str = malloc(strlen(default_header) + strlen(default_expr) + 2);
    sprintf(default_str, "%s\n%s", default_header, default_expr);
    
    // Construir resultado final
    size_t result_len = strlen(ans) + strlen(conditions_exprs) + strlen(default_str) + 3;
    char* result = malloc(result_len);
    sprintf(result, "%s\n%s\n%s", ans, conditions_exprs, default_str);
    
    // Liberar memoria
    free(ans);
    for (int i = 0; i < condition_count * 4; i++) free(condition_expr_strings[i]);
    free(condition_expr_strings);
    free(conditions_exprs);
    free(default_header);
    free(default_expr);
    free(default_str);
    
    return result;
}

char* visit_LetInNode(LetInNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    char* ans = malloc(strlen(indent) + 100);
    sprintf(ans, "%s\\__LetInNode: let [<var_decl>...<var_decl>] in <expr>", indent);
    free(indent);
    
    // Procesar variables (asumimos lista de VarDeclarationNode*)
    char** var_strings = NULL;
    int var_count = 0;
    if (node->variables != NULL) {
        VarDeclarationNode** vars = (VarDeclarationNode**)node->variables;
        while (*vars != NULL) {
            var_count++;
            vars++;
        }
        
        var_strings = malloc(var_count * sizeof(char*));
        vars = (VarDeclarationNode**)node->variables;
        for (int i = 0; i < var_count; i++) {
            var_strings[i] = visit((Node*)vars[i], tabs + 1);
        }
    }
    char* variables = join_strings((const char**)var_strings, var_count, "\n");
    
    // Procesar body
    char* body = visit(node->body, tabs + 1);
    
    // Construir resultado final
    size_t result_len = strlen(ans) + strlen(variables) + strlen(body) + 3;
    char* result = malloc(result_len);
    sprintf(result, "%s\n%s\n%s", ans, variables, body);
    
    // Liberar memoria
    free(ans);
    for (int i = 0; i < var_count; i++) free(var_strings[i]);
    free(var_strings);
    free(variables);
    free(body);
    
    return result;
}

char* visit_WhileNode(WhileNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    char* ans = malloc(strlen(indent) + 100);
    sprintf(ans, "%s\\__WhileNode: while <expr> -> <expr>", indent);
    free(indent);
    
    char* condition = visit(node->condition, tabs + 1);
    char* body = visit(node->body, tabs + 1);
    
    char* result = malloc(strlen(ans) + strlen(condition) + strlen(body) + 3);
    sprintf(result, "%s\n%s\n%s", ans, condition, body);
    
    free(ans);
    free(condition);
    free(body);
    
    return result;
}

char* visit_ForNode(ForNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    char* ans = malloc(strlen(indent) + 100);
    sprintf(ans, "%s\\__ForNode: for <var> in <expr> -> <expr>", indent);
    free(indent);
    
    char* item = visit(node->item, tabs + 1);
    char* iterable = visit(node->iterable, tabs + 1);
    char* body = visit(node->body, tabs + 1);
    
    char* result = malloc(strlen(ans) + strlen(item) + strlen(iterable) + strlen(body) + 4);
    sprintf(result, "%s\n%s\n%s\n%s", ans, item, iterable, body);
    
    free(ans);
    free(item);
    free(iterable);
    free(body);
    
    return result;
}

char* visit_DestrNode(DestrNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    char* ans = malloc(strlen(indent) + 100);
    sprintf(ans, "%s\\__DestrNode: <var> := <expr>", indent);
    free(indent);
    
    char* var = visit(node->var, tabs + 1);
    char* expr = visit(node->expr, tabs + 1);
    
    char* result = malloc(strlen(ans) + strlen(var) + strlen(expr) + 3);
    sprintf(result, "%s\n%s\n%s", ans, var, expr);
    
    free(ans);
    free(var);
    free(expr);
    
    return result;
}

char* visit_BinaryNode(BinaryNode* node, int tabs, const char* class_name) {
    char* indent = create_indentation(tabs);
    char* ans = malloc(strlen(indent) + strlen(class_name) + 20);
    sprintf(ans, "%s\\__<expr> %s <expr>", indent, class_name);
    free(indent);
    
    char* left = visit(node->left, tabs + 1);
    char* right = visit(node->right, tabs + 1);
    
    char* result = malloc(strlen(ans) + strlen(left) + strlen(right) + 3);
    sprintf(result, "%s\n%s\n%s", ans, left, right);
    
    free(ans);
    free(left);
    free(right);
    
    return result;
}

char* visit_UnaryNode(UnaryNode* node, int tabs, const char* class_name) {
    char* indent = create_indentation(tabs);
    char* ans = malloc(strlen(indent) + strlen(class_name) + 20);
    sprintf(ans, "%s\\__%s <expr>", indent, class_name);
    free(indent);
    
    char* operand = visit(node->operand, tabs + 1);
    char* result = malloc(strlen(ans) + strlen(operand) + 2);
    sprintf(result, "%s\n%s", ans, operand);
    
    free(ans);
    free(operand);
    
    return result;
}

char* visit_LiteralNode(LiteralNode* node, int tabs, const char* class_name) {
    char* indent = create_indentation(tabs);
    char* ans = malloc(strlen(indent) + strlen(class_name) + strlen(node->lex) + 10);
    sprintf(ans, "%s\\__%s : %s", indent, class_name, node->lex);
    free(indent);
    return ans;
}

char* visit_ExpressionBlockNode(ExpressionBlockNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    char* ans = malloc(strlen(indent) + 100);
    sprintf(ans, "%s\\__ExpressionBlockNode {{<expr>;...;<expr>;}}", indent);
    free(indent);
    
    // Procesar expresiones (asumimos lista de ExpressionNode*)
    char** expr_strings = NULL;
    int expr_count = 0;
    if (node->expressions != NULL) {
        ExpressionNode** exprs = (ExpressionNode**)node->expressions;
        while (*exprs != NULL) {
            expr_count++;
            exprs++;
        }
        
        expr_strings = malloc(expr_count * sizeof(char*));
        exprs = (ExpressionNode**)node->expressions;
        for (int i = 0; i < expr_count; i++) {
            expr_strings[i] = visit((Node*)exprs[i], tabs + 1);
        }
    }
    char* expressions = join_strings((const char**)expr_strings, expr_count, "\n");
    
    char* result = malloc(strlen(ans) + strlen(expressions) + 2);
    sprintf(result, "%s\n%s", ans, expressions);
    
    free(ans);
    for (int i = 0; i < expr_count; i++) free(expr_strings[i]);
    free(expr_strings);
    free(expressions);
    
    return result;
}

char* visit_CallFuncNode(CallFuncNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    char* ans = malloc(strlen(indent) + strlen(node->name) + 100);
    sprintf(ans, "%s\\__CallFuncNode: %s(<expr>, ..., <expr>)", indent, node->name);
    free(indent);
    
    // Procesar argumentos (asumimos lista de ExpressionNode*)
    char** arg_strings = NULL;
    int arg_count = 0;
    if (node->arguments != NULL) {
        ExpressionNode** args = (ExpressionNode**)node->arguments;
        while (*args != NULL) {
            arg_count++;
            args++;
        }
        
        arg_strings = malloc(arg_count * sizeof(char*));
        args = (ExpressionNode**)node->arguments;
        for (int i = 0; i < arg_count; i++) {
            arg_strings[i] = visit((Node*)args[i], tabs + 1);
        }
    }
    char* arguments = join_strings((const char**)arg_strings, arg_count, "\n");
    
    char* result = malloc(strlen(ans) + strlen(arguments) + 2);
    sprintf(result, "%s\n%s", ans, arguments);
    
    free(ans);
    for (int i = 0; i < arg_count; i++) free(arg_strings[i]);
    free(arg_strings);
    free(arguments);
    
    return result;
}

char* visit_TypeInstantiationNode(TypeInstantiationNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    char* ans = malloc(strlen(indent) + strlen(node->name) + 100);
    sprintf(ans, "%s\\__TypeInstantiationNode: %s(<expr>, ..., <expr>)", indent, node->name);
    free(indent);
    
    // Procesar argumentos (asumimos lista de ExpressionNode*)
    char** arg_strings = NULL;
    int arg_count = 0;
    if (node->arguments != NULL) {
        ExpressionNode** args = (ExpressionNode**)node->arguments;
        while (*args != NULL) {
            arg_count++;
            args++;
        }
        
        arg_strings = malloc(arg_count * sizeof(char*));
        args = (ExpressionNode**)node->arguments;
        for (int i = 0; i < arg_count; i++) {
            arg_strings[i] = visit((Node*)args[i], tabs + 1);
        }
    }
    char* arguments = join_strings((const char**)arg_strings, arg_count, "\n");
    
    char* result = malloc(strlen(ans) + strlen(arguments) + 2);
    sprintf(result, "%s\n%s", ans, arguments);
    
    free(ans);
    for (int i = 0; i < arg_count; i++) free(arg_strings[i]);
    free(arg_strings);
    free(arguments);
    
    return result;
}

char* visit_ExplicitVectorNode(ExplicitVectorNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    char* ans = malloc(strlen(indent) + 100);
    sprintf(ans, "%s\\__ExplicitVectorNode: [<expr>,...,<expr>]", indent);
    free(indent);
    
    // Procesar items (asumimos lista de ExpressionNode*)
    char** item_strings = NULL;
    int item_count = 0;
    if (node->items != NULL) {
        ExpressionNode** items = (ExpressionNode**)node->items;
        while (*items != NULL) {
            item_count++;
            items++;
        }
        
        item_strings = malloc(item_count * sizeof(char*));
        items = (ExpressionNode**)node->items;
        for (int i = 0; i < item_count; i++) {
            item_strings[i] = visit((Node*)items[i], tabs + 1);
        }
    }
    char* items = join_strings((const char**)item_strings, item_count, "\n");
    
    char* result = malloc(strlen(ans) + strlen(items) + 2);
    sprintf(result, "%s\n%s", ans, items);
    
    free(ans);
    for (int i = 0; i < item_count; i++) free(item_strings[i]);
    free(item_strings);
    free(items);
    
    return result;
}

char* visit_ImplicitVectorNode(ImplicitVectorNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    char* ans = malloc(strlen(indent) + 100);
    sprintf(ans, "%s\\__ImplicitVectorNode: [<expr> || <var> in <expr>]", indent);
    free(indent);
    
    char* expr = visit(node->expr, tabs + 1);
    char* item = visit(node->item, tabs + 1);
    char* iterable = visit(node->iterable, tabs + 1);
    
    char* result = malloc(strlen(ans) + strlen(expr) + strlen(item) + strlen(iterable) + 4);
    sprintf(result, "%s\n%s\n%s\n%s", ans, expr, item, iterable);
    
    free(ans);
    free(expr);
    free(item);
    free(iterable);
    
    return result;
}

char* visit_IndexObjectNode(IndexObjectNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    char* ans = malloc(strlen(indent) + 100);
    sprintf(ans, "%s\\__IndexObjectNode: <atom> [<expr>]", indent);
    free(indent);
    
    char* obj = visit(node->object, tabs + 1);
    char* pos = visit(node->pos, tabs + 1);
    
    char* result = malloc(strlen(ans) + strlen(obj) + strlen(pos) + 3);
    sprintf(result, "%s\n%s\n%s", ans, obj, pos);
    
    free(ans);
    free(obj);
    free(pos);
    
    return result;
}

char* visit_CallMethodNode(CallMethodNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    char* ans = malloc(strlen(indent) + strlen(node->method_name) + 100);
    sprintf(ans, "%s\\__CallMethodNode: <atom>.%s(<expr>, ..., <expr>)", 
            indent, node->method_name);
    free(indent);
    
    // Procesar instancia (asumimos que es un ExpressionNode*)
    char* inst_name = visit((Node*)node->inst_name, tabs + 1);
    
    // Procesar argumentos (asumimos lista de ExpressionNode*)
    char** arg_strings = NULL;
    int arg_count = 0;
    if (node->method_args != NULL) {
        ExpressionNode** args = (ExpressionNode**)node->method_args;
        while (*args != NULL) {
            arg_count++;
            args++;
        }
        
        arg_strings = malloc(arg_count * sizeof(char*));
        args = (ExpressionNode**)node->method_args;
        for (int i = 0; i < arg_count; i++) {
            arg_strings[i] = visit((Node*)args[i], tabs + 1);
        }
    }
    char* arguments = join_strings((const char**)arg_strings, arg_count, "\n");
    
    // Construir resultado final
    size_t result_len = strlen(ans) + strlen(inst_name) + strlen(arguments) + 3;
    char* result = malloc(result_len);
    sprintf(result, "%s\n%s\n%s", ans, inst_name, arguments);
    
    // Liberar memoria
    free(ans);
    free(inst_name);
    for (int i = 0; i < arg_count; i++) free(arg_strings[i]);
    free(arg_strings);
    free(arguments);
    
    return result;
}

char* visit_CallTypeAttributeNode(CallTypeAttributeNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    char* ans = malloc(strlen(indent) + strlen(node->attribute) + 100);
    sprintf(ans, "%s\\__CallTypeAttributeNode: <atom>.%s", 
            indent, node->attribute);
    free(indent);
    
    // Procesar instancia (asumimos que es un ExpressionNode*)
    char* inst_name = visit((Node*)node->inst_name, tabs + 1);
    
    char* result = malloc(strlen(ans) + strlen(inst_name) + 2);
    sprintf(result, "%s\n%s", ans, inst_name);
    
    free(ans);
    free(inst_name);
    
    return result;
}

char* visit_CastTypeNode(CastTypeNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    char* ans = malloc(strlen(indent) + strlen(node->type_cast) + 100);
    sprintf(ans, "%s\\__CastTypeNode: <atom> as %s", 
            indent, node->type_cast);
    free(indent);
    
    // Procesar instancia (asumimos que es un ExpressionNode*)
    char* inst_name = visit((Node*)node->inst_name, tabs + 1);
    
    char* result = malloc(strlen(ans) + strlen(inst_name) + 2);
    sprintf(result, "%s\n%s", ans, inst_name);
    
    free(ans);
    free(inst_name);
    
    return result;
}

char* visit_CheckTypeNode(CheckTypeNode* node, int tabs) {
    char* indent = create_indentation(tabs);
    char* ans = malloc(strlen(indent) + 100);
    sprintf(ans, "%s\\__<expr> CheckTypeNode <expr>", indent);
    free(indent);
    
    char* left = visit(node->left, tabs + 1);
    char* right = visit(node->right, tabs + 1);
    
    char* result = malloc(strlen(ans) + strlen(left) + strlen(right) + 3);
    sprintf(result, "%s\n%s\n%s", ans, left, right);
    
    free(ans);
    free(left);
    free(right);
    
    return result;
}

// Función principal para imprimir el AST
void print_ast(Node* root) {
    char* formatted = visit(root, 0);
    printf("%s\n", formatted);
    free(formatted);
}

// Función para liberar el AST
void free_ast(Node* node) {
    if (node == NULL) return;
    
    switch (node->tipo) {
        case NODE_PROGRAM: {
            ProgramNode* n = (ProgramNode*)node;
            // Liberar declaraciones
            if (n->declarations != NULL) {
                Node** decls = (Node**)n->declarations;
                while (*decls != NULL) {
                    free_ast(*decls);
                    decls++;
                }
                free(n->declarations);
            }
            // Liberar expresión
            free_ast(n->expression);
            break;
        }
        case NODE_FUNCTION_DECLARATION: {
            FunctionDeclarationNode* n = (FunctionDeclarationNode*)node;
            free(n->name);
            // Liberar parámetros
            if (n->params != NULL) {
                // Implementar liberación de parámetros según tu estructura
                free(n->params);
            }
            if (n->returnType) free(n->returnType);
            free_ast(n->body);
            break;
        }
        // Implementar casos para otros tipos de nodos...
        
        default:
            // Para nodos simples que no tienen hijos o strings dinámicos
            break;
    }
    
    free(node);
}