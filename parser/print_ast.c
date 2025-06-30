#include <stdio.h>
#include "../ast_nodes/ast_nodes.h"

void print_expression(ExpressionNode* expr, int indent);
void print_declaration(DeclarationNode* decl, int indent);

void print_indent(int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
}

void print_program(ProgramNode* prog) {
    printf("ProgramNode:\n");

    printf("  Declarations:\n");
    DeclarationNode** decls = (DeclarationNode**) prog->declarations;
     
    for (int i = 0; decls && decls[i] != NULL; i++) {
    print_declaration(decls[i], 2);
    }

    if (prog->expression) {
        printf("  Expression:\n");
        print_expression(prog->expression, 2);
    } else {
        printf("  Expression: NULL\n");
    }
}

void print_declaration(DeclarationNode* decl, int indent) {
    if (!decl) {
        print_indent(indent);
        printf("NULL DeclarationNode\n");
        return;
    }

    print_indent(indent);
    printf("DeclarationNode type: %d\n", decl->base.tipo);

    if (decl->base.tipo == NODE_FUNCTION_DECLARATION) {
        FunctionDeclarationNode* f = (FunctionDeclarationNode*) decl;
        if (f->name) {
            print_indent(indent + 1);
            printf("Function name: %s\n", f->name);
        } else {
            print_indent(indent + 1);
            printf("Function name: NULL\n");
        }

    } else if (decl->base.tipo == NODE_TYPE_DECLARATION) {
        TypeDeclarationNode* t = (TypeDeclarationNode*) decl;
        print_indent(indent + 1);
        printf("Type name: %s\n", t->name ? t->name : "NULL");
        print_indent(indent + 1);
        printf("Parent: %s\n", t->parent ? t->parent : "NULL");

        if (t->attributes) {
            print_indent(indent + 1);
            printf("Attributes:\n");
            TypeAttributeNode** attrs = (TypeAttributeNode**) t->attributes;
            for (int i = 0; i < t->attribute_counter; i++) {
                print_indent(indent + 2);
                printf("Attr name: %s\n", attrs[i]->name ? attrs[i]->name : "NULL");
            }
        }

        if (t->methods) {
            print_indent(indent + 1);
            printf("Methods:\n");
            MethodDeclarationNode** methods = (MethodDeclarationNode**) t->methods;
            for (int i = 0; i < t->method_counter; i++) {
                print_indent(indent + 2);
                printf("Method name: %s\n", methods[i]->name ? methods[i]->name : "NULL");
            }
        }

    } else {
        print_indent(indent + 1);
        printf("Unknown or invalid DeclarationNode type!\n");
    }
}

void print_expression(ExpressionNode* expr, int indent) {
    if (!expr) return;
    print_indent(indent);
    printf("ExpressionNode type: %d\n", expr->base.tipo);

    switch (expr->base.tipo) {
        case NODE_CONDITIONAL: {
            ConditionalNode* cond = (ConditionalNode*) expr;
            ExpressionNode** conds = (ExpressionNode**) cond->conditions;
            ExpressionNode** exprs = (ExpressionNode**) cond->expressions;
            for (int i = 0; i < cond->condition_counter; i++) {
                print_indent(indent + 1);
                printf("Condition:\n");
                print_expression(conds[i], indent + 2);
                print_indent(indent + 1);
                printf("Then:\n");
                print_expression(exprs[i], indent + 2);
            }
            if (cond->default_expre) {
                print_indent(indent + 1);
                printf("Else:\n");
                print_expression(cond->default_expre, indent + 2);
            }
            break;
        }

        case NODE_WHILE: {
            WhileNode* w = (WhileNode*) expr;
            print_indent(indent + 1);
            printf("While Condition:\n");
            print_expression(w->condition, indent + 2);
            print_indent(indent + 1);
            printf("While Body:\n");
            print_expression(w->body, indent + 2);
            break;
        }

        case NODE_FOR: {
            ForNode* f = (ForNode*) expr;
            print_indent(indent + 1);
            printf("For Item: %s\n", (char*) f->item);
            print_indent(indent + 1);
            printf("Iterable:\n");
            print_expression(f->iterable, indent + 2);
            print_indent(indent + 1);
            printf("Body:\n");
            print_expression(f->body, indent + 2);
            break;
        }

        case NODE_LET_IN: {
            
            LetInNode* let = (LetInNode*) expr;
            print_indent(indent + 1);
            printf("Let variables:\n");
            VarDeclarationNode** vars = (VarDeclarationNode**) let->variables;
            for (int i = 0; i < let->variable_counter; i++) {
                print_indent(indent + 2);
                printf("Var name: %s, type: %s\n", vars[i]->name,
                       vars[i]->type ? vars[i]->type : "NULL");
                print_indent(indent + 2);
                printf("Value:\n");
                print_expression((ExpressionNode*) vars[i]->value, indent + 3);
            }
            print_indent(indent + 1);
            printf("Body:\n");
            print_expression(let->body, indent + 2);
            break;
        }

        case NODE_OR: 
        case NODE_AND:
        case NODE_PLUS:
        case NODE_MINUS:
        case NODE_POW:
        case NODE_DIV: {
            BinaryNode* b = (BinaryNode*) expr;
            print_indent(indent + 1);
            printf("Left:\n");
            print_expression((ExpressionNode*) b->left, indent + 2);
            print_indent(indent + 1);
            printf("Right:\n");
            print_expression((ExpressionNode*) b->right, indent + 2);
            break;
        }

        case NODE_CALL_FUNC: {
            CallFuncNode* call = (CallFuncNode*) expr;
            print_indent(indent + 1);
            printf("Function call: %s\n", call->name);
            if (call->arguments) {
                ExpressionNode** args = (ExpressionNode**) call->arguments;
                for (int i = 0; i < call->arguments_counter; i++) {
                    print_indent(indent + 2);
                    printf("Arg %d:\n", i);
                    print_expression(args[i], indent + 3);
                }
            }
            break;
        }

        case NODE_TYPE_ATTRIBUTE: {
            MemberNode* mem = (MemberNode*) expr;
            print_indent(indent + 1);
            printf("Member of:\n");
            print_expression((ExpressionNode*) mem->object, indent + 2);
            print_indent(indent + 1);
            printf("Member name: %s\n", mem->member);
            break;
        }
        case NODE_EXPRESSION_BLOCK: {
            ExpressionBlockNode* block = (ExpressionBlockNode*)expr;
            print_indent(indent + 1);
            printf("Expression Block:\n");

            ExpressionNode** exprs = (ExpressionNode**)block->expressions;
            for (int i = 0; exprs && exprs[i]; i++) {
                print_expression(exprs[i], indent + 2);
            }
            break;
        }
        case NODE_INDEX_OBJECT: {
            IndexObjectNode* idx = (IndexObjectNode*) expr;
            print_indent(indent + 1);
            printf("Object:\n");
            print_expression((ExpressionNode*) idx->object, indent + 2);
            print_indent(indent + 1);
            printf("Index:\n");
            print_expression((ExpressionNode*) idx->pos, indent + 2);
            break;
        }

        case NODE_EXPLICIT_VECTOR: {
            ExplicitVectorNode* vec = (ExplicitVectorNode*) expr;
            print_indent(indent + 1);
            printf("Explicit vector items:\n");
            ExpressionNode** items = (ExpressionNode**) vec->items;
            for (int i = 0; i < vec->item_counter; i++) {
                print_expression(items[i], indent + 2);
            }
            break;
        }

        case NODE_VAR: {
            VarNode* v = (VarNode*) expr;
            print_indent(indent + 1);
            printf("Var: %s\n", v->base.base.base.base.lexeme);
            break;
        }
        case NODE_PRINT: {
            PrintNode* p = (PrintNode*) expr;
            print_indent(indent + 1);
            printf("Print:\n");
            print_expression(p->value, indent + 2);
            break;
        }

        case NODE_NUMBER: {
            NumberNode* n = (NumberNode*) expr;
            print_indent(indent + 1);
            printf("Number: %s\n", n->base.lex);
            break;
        }

        case NODE_STRING: {
            StringNode* s = (StringNode*) expr;
            print_indent(indent + 1);
            printf("String: %s\n", s->base.lex);
            break;
        }

        case NODE_BOOLEAN: {
            BooleanNode* b = (BooleanNode*) expr;
            print_indent(indent + 1);
            printf("Boolean: %s\n", b->base.lex);
            break;
        }

        default:
            print_indent(indent + 1);
            printf("Tipo de nodo sin imprimir.\n");
    }
}
