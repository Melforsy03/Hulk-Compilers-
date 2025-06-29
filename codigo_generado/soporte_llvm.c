#include "generacion_codigo.h"
#include <string.h>   // ← para strdup, strcmp, strlen

extern FILE* salida_llvm ;
StringConst constantes_string[MAX_STRING_CONSTS];
int num_strings = 0;

void declare_extern_functions() {
    fprintf(salida_llvm, "declare i8* @strdup(i8*)\n");
    fprintf(salida_llvm, "declare i32 @int_pow(i32, i32)\n");
    fprintf(salida_llvm, "declare void @print_int(i32)\n");
    fprintf(salida_llvm, "declare void @print_str(i8*)\n");
    fprintf(salida_llvm, "declare i8* @int_to_string(i32)\n");
    fprintf(salida_llvm, "declare i8* @strcat2(i8*, i8*)\n\n");
    fprintf(salida_llvm, "declare i32 @strcmp(i8*, i8*)\n");
    fprintf(salida_llvm, "declare i8* @float_to_string(float)\n");
    fprintf(salida_llvm, "declare i8* @bool_to_string(i1)\n");
    fprintf(salida_llvm, "declare float @my_sin(float)\n");
    fprintf(salida_llvm, "declare float @my_cos(float)\n");
    fprintf(salida_llvm, "declare float @my_tan(float)\n");
    fprintf(salida_llvm, "declare float @my_cot(float)\n");
    fprintf(salida_llvm, "declare void @print_float(float)\n");
}
void generar_funciones(ExpressionNode* expr) {
    if (!expr) return;
    NodeType tipo = ((Node*)expr)->tipo;

    switch (tipo) {
        case NODE_FUNCTION_DECLARATION:
            generar_codigo(expr);
            break;

        case NODE_EXPRESSION_BLOCK: {
            ExpressionBlockNode* block = (ExpressionBlockNode*)expr;
            ExpressionNode** exprs = (ExpressionNode**)block->expressions;
            for (int i = 0; exprs && exprs[i]; i++) {
                generar_funciones(exprs[i]);
            }
            break;
        }

       
        case NODE_LET_IN: {
            LetInNode* let = (LetInNode*)expr;
            generar_funciones(let->body);
            break;
        }

        case NODE_CONDITIONAL: {
        ConditionalNode* cond = (ConditionalNode*)expr;
        ExpressionNode** conds = (ExpressionNode**)cond->conditions;
        ExpressionNode** exprs = (ExpressionNode**)cond->expressions;
        for (int i = 0; conds && conds[i]; i++) {
            generar_funciones(conds[i]);
        }
        for (int i = 0; exprs && exprs[i]; i++) {
            generar_funciones(exprs[i]);
        }
        if (cond->default_expre) {
            generar_funciones((ExpressionNode*)cond->default_expre);
        }
        break;
    }

        case NODE_WHILE: {
            WhileNode* wh = (WhileNode*)expr;
            generar_funciones(wh->condition);
            generar_funciones(wh->body);
            break;
        }

        default:
            break;
    }
}
int registrar_string_global(const char* texto) {
    for (int i = 0; i < num_strings; i++) {
        if (strcmp(constantes_string[i].valor, texto) == 0)
            return constantes_string[i].id;
    }
    int nuevo_id = num_strings;
    constantes_string[nuevo_id].valor = strdup(texto);
    constantes_string[nuevo_id].id = nuevo_id;
    num_strings++;
    return nuevo_id;
}
void recorrer_ast_para_strings(ExpressionNode* expr) {
    if (!expr) return;

    NodeType tipo = ((Node*)expr)->tipo;

    switch (tipo) {
        case NODE_STRING: {
            StringNode* str = (StringNode*)expr;
            registrar_string_global(str->base.lex);
            break;
        }
     
        case NODE_LET_IN: {
           
            LetInNode* let = (LetInNode*)expr;
            VarDeclarationNode** decls = (VarDeclarationNode**)let->variables;
            for (int i = 0; decls && decls[i]; i++) {
                recorrer_ast_para_strings(decls[i]->value);
               
            }
           
            recorrer_ast_para_strings(let->body);
            
            break;
        }

        
        case NODE_PRINT: {
            PrintNode* p = (PrintNode*)expr;
            recorrer_ast_para_strings(p->value);
            break;
        }
        case NODE_NOT: {
            UnaryNode* un = (UnaryNode*)expr;
            recorrer_ast_para_strings(un->operand);
            break;
        }

        case NODE_RETURN: {
            ReturnNode* ret = (ReturnNode*)expr;
            recorrer_ast_para_strings(ret->expr);
            break;
        }

        case NODE_PLUS:
        case NODE_MINUS: case NODE_MULT:
        case NODE_DIV: case NODE_EQUAL: case NODE_LESS:
        case NODE_GREATER: case NODE_AND: case NODE_OR:
        case NODE_POW: case NODE_MOD:
        case NODE_NOT_EQUAL: case NODE_LESS_EQUAL: case NODE_GREATER_EQUAL: {
  
            BinaryNode* bin = (BinaryNode*)expr;
            recorrer_ast_para_strings((ExpressionNode*)bin->left);
            recorrer_ast_para_strings((ExpressionNode*)bin->right);
            break;
        }
        case NODE_CALL_FUNC: {
            CallFuncNode* call = (CallFuncNode*)expr;
            ExpressionNode** args = (ExpressionNode**)call->arguments;
            for (int i = 0; args && args[i]; i++) {
                recorrer_ast_para_strings(args[i]);
            }
            break;
        }

        case NODE_EXPRESSION_BLOCK: {
            ExpressionBlockNode* block = (ExpressionBlockNode*)expr;
            ExpressionNode** exprs = (ExpressionNode**)block->expressions;
            for (int i = 0; exprs && exprs[i]; i++)
                recorrer_ast_para_strings(exprs[i]);
            break;
        }
        case NODE_CONDITIONAL: {
            ConditionalNode* cond = (ConditionalNode*)expr;
            ExpressionNode** conditions = (ExpressionNode**)cond->conditions;
            ExpressionNode** expressions = (ExpressionNode**)cond->expressions;
            for (int i = 0; conditions && conditions[i]; i++)
                recorrer_ast_para_strings(conditions[i]);
            for (int i = 0; expressions && expressions[i]; i++)
                recorrer_ast_para_strings(expressions[i]);
            if (cond->default_expre)
                recorrer_ast_para_strings(cond->default_expre);
            break;
        }
        case NODE_CONCAT: {
            BinaryNode* bin = (BinaryNode*)expr;
            recorrer_ast_para_strings((ExpressionNode*)bin->left);
            recorrer_ast_para_strings((ExpressionNode*)bin->right);
            break;
        }

        case NODE_WHILE: {
            WhileNode* wh = (WhileNode*)expr;
            recorrer_ast_para_strings((ExpressionNode*)wh->condition);
            recorrer_ast_para_strings((ExpressionNode*)wh->body);
            break;
        }
        default: break;
    }
}
void generar_constantes_globales(ProgramNode* program) {
    // Recolectar strings en el cuerpo del programa
        if (program->expression)
           
            recorrer_ast_para_strings((ExpressionNode*)program->expression);
   
        // Recolectar strings en las funciones también
        if (program->declarations) {
            DeclarationNode** decls = (DeclarationNode**)program->declarations;
            for (int i = 0; decls[i]; i++) {
                if (decls[i]->base.tipo == NODE_FUNCTION_DECLARATION) {
                    FunctionDeclarationNode* f = (FunctionDeclarationNode*)decls[i];
                    if (f->body) {
                        recorrer_ast_para_strings(f->body);
                    }
                }
            }
        }

    for (int i = 0; i < num_strings; i++) {
        char* str_val = constantes_string[i].valor;
        int len = strlen(str_val);
        fprintf(salida_llvm, "@.str.%d = private unnamed_addr constant [%d x i8] c\"%s\\00\"\n",
               i, len + 1, str_val);
    }
}
void asegurar_declaracion(const char* var_name) {
   fprintf(salida_llvm, "  %%var_%s = alloca i32\n", var_name);
   fprintf(salida_llvm, "  store i32 0, i32* %%var_%s\n", var_name);

}
