#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void indent(CodeGenContext* ctx) {
    for (int i = 0; i < ctx->indent; ++i)
        fprintf(ctx->out, "  ");
}

static char* codegen_expr(CodeGenContext* ctx, ASTNode* node) {
  switch (node->type) {
    case AST_NUMBER: {
      char* buffer = malloc(32);
      snprintf(buffer, 32, "%s", node->value);
      return buffer;
    }
    case AST_IDENTIFIER: {
      char* buffer = malloc(32);
      snprintf(buffer, 32, "%%%s_val", node->value);
      return buffer;
    }
    case AST_BINOP: {
      char* left = codegen_expr(ctx, node->children[0]);
      char* right = codegen_expr(ctx, node->children[1]);
      int temp = ctx->temp_count++;
      const char* op = strcmp(node->value, "+") == 0 ? "add" :
                       strcmp(node->value, "-") == 0 ? "sub" :
                       strcmp(node->value, "*") == 0 ? "mul" :
                       strcmp(node->value, "/") == 0 ? "sdiv" : NULL;
      indent(ctx);
      fprintf(ctx->out, "%%%d = %s i32 %s, %s\n", temp, op, left, right);
      free(left);
      free(right);
      char* buffer = malloc(32);
      snprintf(buffer, 32, "%%%d", temp);
      return buffer;
    }
    case AST_ASSIGN: {
        const char* name = node->children[0]->value;
        ASTNode* expr_node = node->children[1];
        char* expr = codegen_expr(ctx, expr_node);

        indent(ctx);
        fprintf(ctx->out, "store i32 %s, i32* %%%s\n", expr, name);
        indent(ctx);
        fprintf(ctx->out, "%%%s_val = load i32, i32* %%%s\n", name, name);
        free(expr);
        break;
        }
        case AST_UNARYOP: {
            const char* op = node->value;
            char* expr = codegen_expr(ctx, node->children[0]);
            int temp = ctx->temp_count++;

            if (strcmp(op, "-") == 0) {
                indent(ctx);
                fprintf(ctx->out, "%%%d = sub i32 0, %s\n", temp, expr);
            } else {
                fprintf(stderr, "Operador unario no soportado: %s\n", op);
                exit(1);
            }

            char* buffer = malloc(32);
            snprintf(buffer, 32, "%%%d", temp);
            free(expr);
            return buffer;
            }
           case AST_BOOL:
            {
                char* buffer = malloc(32);
                if (strcmp(node->value, "true") == 0) {
                snprintf(buffer, 32, "1");
                } else {
                snprintf(buffer, 32, "0");
                }
                return buffer;
            }

        
            case AST_WHILE: {
                int label_id = ctx->temp_count++;

                indent(ctx);
                fprintf(ctx->out, "br label %%loop_cond%d\n", label_id);

                indent(ctx);
                fprintf(ctx->out, "loop_cond%d:\n", label_id);

                char* cond = codegen_expr(ctx, node->children[0]);
                indent(ctx);
                fprintf(ctx->out, "%%cond%d = icmp ne i32 %s, 0\n", label_id, cond);

                indent(ctx);
                fprintf(ctx->out, "br i1 %%cond%d, label %%loop_body%d, label %%loop_end%d\n",
                        label_id, label_id, label_id);

                indent(ctx);
                fprintf(ctx->out, "loop_body%d:\n", label_id);
                ctx->indent++;
                codegen_stmt(ctx, node->children[1]); // cuerpo while
                indent(ctx);
                fprintf(ctx->out, "br label %%loop_cond%d\n", label_id);
                ctx->indent--;

                indent(ctx);
                fprintf(ctx->out, "loop_end%d:\n", label_id);
                free(cond);
                break;
            }

    default:
      fprintf(stderr, "Error: tipo de nodo de expresión no soportado: %d\n", node->type);
      exit(1);
  }
}

static void codegen_stmt(CodeGenContext* ctx, ASTNode* node) {
    switch (node->type) {
        case AST_PROGRAM:
        case AST_STATEMENT_LIST:
        case AST_BLOCK:
        case AST_VAR_DECL_LIST:
        case AST_LET:
            for (int i = 0; i < node->num_children; ++i) {
                codegen_stmt(ctx, node->children[i]);
            }
            break;

        case AST_VAR_DECL: {
            const char* name = node->children[0]->value;
            ASTNode* expr_node = NULL;

            if (node->num_children == 3 && node->children[1]->type == AST_ASSIGN_OP) {
                // Ignora AssignOp
                expr_node = node->children[2];
            } else {
                expr_node = node->children[1];
            }

            char* expr = codegen_expr(ctx, expr_node);

            indent(ctx);
            fprintf(ctx->out, "%%%s = alloca i32\n", name);
            indent(ctx);
            fprintf(ctx->out, "store i32 %s, i32* %%%s\n", expr, name);
            indent(ctx);
            fprintf(ctx->out, "%%%s_val = load i32, i32* %%%s\n", name, name);
            break;
        }
        case AST_IF: {
            char* cond = codegen_expr(ctx, node->children[0]);
            int label_id = ctx->temp_count++;

            indent(ctx);
            fprintf(ctx->out, "%%cond%d = icmp ne i32 %s, 0\n", label_id, cond);

            indent(ctx);
            fprintf(ctx->out, "br i1 %%cond%d, label %%then%d, label %%endif%d\n",
                    label_id, label_id, label_id);

            // Then
            indent(ctx);
            fprintf(ctx->out, "then%d:\n", label_id);
            ctx->indent++;
            codegen_stmt(ctx, node->children[1]); // cuerpo if
            indent(ctx);
            fprintf(ctx->out, "br label %%endif%d\n", label_id);
            ctx->indent--;

            // Endif
            indent(ctx);
            fprintf(ctx->out, "endif%d:\n", label_id);
            free(cond);
            break;
            }
        case AST_PRINT: {
            char* val = codegen_expr(ctx, node->children[0]);
            indent(ctx);
            fprintf(ctx->out, "call i32 (i8*, ...) @printf(i8* getelementptr "
                              "([4 x i8], [4 x i8]* @print.str, i32 0, i32 0), i32 %s)\n", val);
            break;
        }

        default:
            fprintf(stderr, "Error: tipo de nodo de sentencia no soportado: %d\n", node->type);
            exit(1);
    }
}

void generate_code(CodeGenContext* ctx, ASTNode* root) {
    ctx->temp_count = 1;
    ctx->indent = 0;

    fprintf(ctx->out, "declare i32 @printf(i8*, ...)\n");
    fprintf(ctx->out, "@print.str = constant [4 x i8] c\"%%d\\0A\\00\"\n\n");

    fprintf(ctx->out, "define i32 @main() {\n");
    ctx->indent++;

    codegen_stmt(ctx, root);

    indent(ctx);
    fprintf(ctx->out, "ret i32 0\n");

    ctx->indent--;
    fprintf(ctx->out, "}\n");
}
