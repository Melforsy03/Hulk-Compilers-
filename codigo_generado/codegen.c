#include "codegen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

FILE* salida_llvm = NULL;

static int contador_temporales = 0;
static int contador_etiquetas = 0;
// Añadir variable solo si aún no existe
int tmp_counter = 0;
char last_tmp[32];
#define MAX_STRING_CONSTS 100
StringConst constantes_string[MAX_STRING_CONSTS];
int num_strings = 0;
Variable variables_usadas[MAX_VARIABLES];  // ← aquí defines
int num_variables = 0;  
VarType obtener_tipo_variable(const char* nombre) {
    for (int i = 0; i < num_variables; i++) {
        if (strcmp(variables_usadas[i].nombre, nombre) == 0) {
            return variables_usadas[i].tipo;
        }
    }
    // Por defecto int si no se encuentra
    return VAR_TYPE_INT;
}

int nuevo_temp() {
    return contador_temporales++;
}
const char* obtener_nombre_variable(VarNode* var) {
    // Asumiendo que VarNode hereda de LiteralNode que tiene el campo 'lex'
    return ((LiteralNode*)var)->lex;
}
int nuevo_label() {
    return contador_etiquetas++;
}

void reset_temporales() {
    contador_temporales = 0;
}

// Función principal de generación de código
int generar_codigo(ExpressionNode* expr) {
    if (!expr) return -1;

    // Accedemos al tipo a través de la estructura base
    NodeType tipo = ((Node*)expr)->tipo;

    switch (tipo) {
        // Literales
        case NODE_NUMBER: {
            NumberNode* num = (NumberNode*)expr;
            int temp = nuevo_temp();
            // Usamos el campo 'lex' de LiteralNode
            fprintf(salida_llvm, "  %%%d = add i32 0, %s\n", temp, ((LiteralNode*)num)->lex);
            return temp;
        }
        case NODE_PROGRAM: {
            ProgramNode* prog = (ProgramNode*)expr;
            if (prog->expression) {
                return generar_codigo(prog->expression);
            }
            return -1;
        }

        case NODE_LET: {
            LetInNode* let_in = (LetInNode*)expr;
            VarDeclarationNode** declarations = (VarDeclarationNode**)let_in->variables;
            
            for (int i = 0; declarations[i] != NULL; i++) {
                VarDeclarationNode* decl = declarations[i];
                
                // Determinar el tipo de la variable (int o string)
                VarType var_tipo = VAR_TYPE_INT; // Por defecto entero
                if (decl->value && ((Node*)decl->value)->tipo == NODE_STRING) {
                    var_tipo = VAR_TYPE_STRING;
                }
                
                // Registrar la variable en la tabla de símbolos
                variables_usadas[num_variables].nombre = strdup(decl->name);
                variables_usadas[num_variables].tipo = var_tipo;
                variables_usadas[num_variables].inicializada = 1;
                num_variables++;
                
                // Generar código según el tipo
                if (var_tipo == VAR_TYPE_STRING) {
                    fprintf(salida_llvm, "  %%%s = alloca i8*\n", decl->name); // Puntero a string
                    if (decl->value) {
                        int temp = generar_codigo(decl->value); // Genera el string
                        fprintf(salida_llvm, "  store i8* %%%d, i8** %%%s\n", temp, decl->name);
                    } else {
                        fprintf(salida_llvm, "  store i8* null, i8** %%%s\n", decl->name); // Inicializar a NULL si no hay valor
                    }
                } else {
                    fprintf(salida_llvm, "  %%%s = alloca i32\n", decl->name); // Entero
                    if (decl->value) {
                        int temp = generar_codigo(decl->value);
                        fprintf(salida_llvm, "  store i32 %%%d, i32* %%%s\n", temp, decl->name);
                    } else {
                        fprintf(salida_llvm, "  store i32 0, i32* %%%s\n", decl->name); // Inicializar a 0 si no hay valor
                    }
                }
            }
            
            // Generar el cuerpo del 'in'
            return generar_codigo(let_in->body);
        }
        case NODE_LET_IN: {
                LetInNode* let_in = (LetInNode*)expr;
                VarDeclarationNode** declarations = (VarDeclarationNode**)let_in->variables;

                for (int i = 0; declarations && declarations[i] != NULL; i++) {
                    VarDeclarationNode* decl = declarations[i];

                    // Determinar el tipo de la variable (int o string)
                    VarType var_tipo = VAR_TYPE_INT; // Por defecto
                    if (decl->value && ((Node*)decl->value)->tipo == NODE_STRING) {
                        var_tipo = VAR_TYPE_STRING;
                    }

                    // Registrar la variable en la tabla de símbolos
                    variables_usadas[num_variables].nombre = strdup(decl->name);
                    variables_usadas[num_variables].tipo = var_tipo;
                    variables_usadas[num_variables].inicializada = 1;
                    num_variables++;

                    // Generar código de la variable
                    if (var_tipo == VAR_TYPE_STRING) {
                        fprintf(salida_llvm, "  %%%s = alloca i8*\n", decl->name);
                        if (decl->value) {
                            int temp = generar_codigo(decl->value);
                            fprintf(salida_llvm, "  store i8* %%%d, i8** %%%s\n", temp, decl->name);
                        } else {
                            fprintf(salida_llvm, "  store i8* null, i8** %%%s\n", decl->name);
                        }
                    } else {
                        fprintf(salida_llvm, "  %%%s = alloca i32\n", decl->name);
                        if (decl->value) {
                            int temp = generar_codigo(decl->value);
                            fprintf(salida_llvm, "  store i32 %%%d, i32* %%%s\n", temp, decl->name);
                        } else {
                            fprintf(salida_llvm, "  store i32 0, i32* %%%s\n", decl->name);
                        }
                    }
                }

                // Generar el cuerpo del let-in
                return generar_codigo(let_in->body);
            }

        case NODE_VAR: {
            VarNode* var = (VarNode*)expr;
            const char* var_name = obtener_nombre_variable(var);
            VarType tipo = obtener_tipo_variable(var_name);
            
            int temp = nuevo_temp();
            if (tipo == VAR_TYPE_STRING) {
                fprintf(salida_llvm, "  %%%d = load i8*, i8** %%%s\n", temp, var_name);
            } else {
                fprintf(salida_llvm, "  %%%d = load i32, i32* %%%s\n", temp, var_name);
            }
            return temp;
        }
        
        case NODE_BOOLEAN: {
            BooleanNode* bool_node = (BooleanNode*)expr;
            int temp = nuevo_temp();
            // Usamos el campo 'lex' de LiteralNode
            fprintf(salida_llvm, "  %%%d = add i1 0, %s\n", temp, 
                  (strcmp(((LiteralNode*)bool_node)->lex, "true") == 0) ? "1" : "0");
            return temp;
        }
        case NODE_STRING: {
            StringNode* str = (StringNode*)expr;
            int temp = nuevo_temp();  // temporario para el puntero duplicado
            char* str_val = ((LiteralNode*)str)->lex;
            int len = strlen(str_val);
        
            // Registrar el string para impresión global si aún no está
            int id = registrar_string_global(str_val); // usa un sistema para evitar duplicados
        
            // Generar llamada a strdup con el string global
            fprintf(salida_llvm, "  %%%d = call i8* @strdup(i8* getelementptr inbounds ([%d x i8], [%d x i8]* @.str.%d, i32 0, i32 0))\n",
                   temp, len + 1, len + 1, id);
        
            return temp;
        }
        
        // Aritméticos
        case NODE_ADD: return generar_binario((BinaryNode*)expr, "add");
        case NODE_SUB: return generar_binario((BinaryNode*)expr, "sub");
        case NODE_MUL: return generar_binario((BinaryNode*)expr, "mul");
        case NODE_DIV: return generar_binario((BinaryNode*)expr, "sdiv");
        case NODE_MOD: return generar_binario((BinaryNode*)expr, "srem");
        case NODE_POW: {
            BinaryNode* bin = (BinaryNode*)expr;
            int izq = get_valor(bin->left);
            int der = get_valor(bin->right);
            int temp = nuevo_temp();
            fprintf(salida_llvm, "  %%%d = call i32 @llvm.pow.i32(i32 %%%d, i32 %%%d)\n", temp, izq, der);
            return temp;
        }

        // Comparaciones
        case NODE_EQ: return generar_comparacion((BinaryNode*)expr, "eq");
        case NODE_NEQ: return generar_comparacion((BinaryNode*)expr, "ne");
        case NODE_LT: return generar_comparacion((BinaryNode*)expr, "slt");
        case NODE_LTE: return generar_comparacion((BinaryNode*)expr, "sle");
        case NODE_GT: return generar_comparacion((BinaryNode*)expr, "sgt");
        case NODE_GTE: return generar_comparacion((BinaryNode*)expr, "sge");

        // Lógicos
        case NODE_AND: {
            BinaryNode* bin = (BinaryNode*)expr;
            int izq = get_valor(bin->left);
            int der = get_valor(bin->right);
            int temp = nuevo_temp();
            fprintf(salida_llvm, "  %%%d = and i1 %%%d, %%%d\n", temp, izq, der);
            return temp;
        }
        case NODE_OR: {
            BinaryNode* bin = (BinaryNode*)expr;
            int izq = get_valor(bin->left);
            int der = get_valor(bin->right);
            int temp = nuevo_temp();
            fprintf(salida_llvm, "  %%%d = or i1 %%%d, %%%d\n", temp, izq, der);
            return temp;
        }
        case NODE_NOT: {
            UnaryNode* un = (UnaryNode*)expr;
            int val = get_valor(un->operand);
            int temp = nuevo_temp();
            fprintf(salida_llvm, "  %%%d = xor i1 %%%d, 1\n", temp, val);
            return temp;
        }

        // Control de flujo
        case NODE_IF: {
                ConditionalNode* cond = (ConditionalNode*)expr;

                // ✅ Corrige: usar solo la primera condición
                ExpressionNode** conditions = (ExpressionNode**)cond->conditions;
                int cond_val = generar_codigo(conditions[0]);

                int label_then = nuevo_label();
                int label_else = nuevo_label();
                int label_end = nuevo_label();

                fprintf(salida_llvm, "  br i1 %%%d, label %%L%d, label %%L%d\n", cond_val, label_then, label_else);

                // THEN
                fprintf(salida_llvm, "L%d:\n", label_then);
                ExpressionNode** thens = (ExpressionNode**)cond->expressions;
                for (int i = 0; thens && thens[i]; i++) {
                    generar_codigo(thens[i]);
                }
                fprintf(salida_llvm, "  br label %%L%d\n", label_end);

                // ELSE
                fprintf(salida_llvm, "L%d:\n", label_else);
                if (cond->default_expre)
                    generar_codigo(cond->default_expre);
                fprintf(salida_llvm, "  br label %%L%d\n", label_end);

                // END
                fprintf(salida_llvm, "L%d:\n", label_end);
                return -1;
            }

        case NODE_WHILE: {
                WhileNode* wh = (WhileNode*)expr;
                int label_start = nuevo_label();
                int label_body = nuevo_label();
                int label_end = nuevo_label();

                fprintf(salida_llvm, "  br label %%L%d\n", label_start);
                fprintf(salida_llvm, "L%d:\n", label_start);

                int cond = get_valor(wh->condition);
                fprintf(salida_llvm, "  br i1 %%%d, label %%L%d, label %%L%d\n", cond, label_body, label_end);

                fprintf(salida_llvm, "L%d:\n", label_body);
                
               if (((Node*)wh->body)->tipo == NODE_BLOCK) {
                    ExpressionBlockNode* block = (ExpressionBlockNode*)wh->body;
                    ExpressionNode** exprs = (ExpressionNode**)block->expressions;
                    for (int i = 0; exprs && exprs[i]; i++) {
                        generar_codigo(exprs[i]);
                    }
                } else {
                    generar_codigo(wh->body);
                }


                fprintf(salida_llvm, "  br label %%L%d\n", label_start);
                fprintf(salida_llvm, "L%d:\n", label_end);
                return -1;
            }
            case NODE_FOR: {
                ForNode* fr = (ForNode*)expr;

                // Supone que iterable = llamada a range(a, b)
                CallFuncNode* rango = (CallFuncNode*)fr->iterable;
                ExpressionNode** args = rango->arguments;

                int start = generar_codigo(args[0]);
                int end   = generar_codigo(args[1]);

                VarNode* var_item = (VarNode*)fr->item;
                const char* nombre = obtener_nombre_variable(var_item);

                int label_start = nuevo_label();
                int label_body  = nuevo_label();
                int label_end   = nuevo_label();

                fprintf(salida_llvm, "  %%%s = alloca i32\n", nombre);
                fprintf(salida_llvm, "  store i32 %%%d, i32* %%%s\n", start, nombre);
                fprintf(salida_llvm, "  br label %%L%d\n", label_start);

                fprintf(salida_llvm, "L%d:\n", label_start);
                int current = nuevo_temp();
                fprintf(salida_llvm, "  %%%d = load i32, i32* %%%s\n", current, nombre);

                int cond = nuevo_temp();
                fprintf(salida_llvm, "  %%%d = icmp slt i32 %%%d, %%%d\n", cond, current, end);
                fprintf(salida_llvm, "  br i1 %%%d, label %%L%d, label %%L%d\n", cond, label_body, label_end);

                fprintf(salida_llvm, "L%d:\n", label_body);
                generar_codigo(fr->body);

                int temp = nuevo_temp();
                fprintf(salida_llvm, "  %%%d = load i32, i32* %%%s\n", temp, nombre);
                int temp_inc = nuevo_temp();
                fprintf(salida_llvm, "  %%%d = add i32 %%%d, 1\n", temp_inc, temp);
                fprintf(salida_llvm, "  store i32 %%%d, i32* %%%s\n", temp_inc, nombre);
                fprintf(salida_llvm, "  br label %%L%d\n", label_start);

                fprintf(salida_llvm, "L%d:\n", label_end);
                return -1;
            }

           case NODE_FUNCTION_DEF: {
                FunctionDeclarationNode* func = (FunctionDeclarationNode*)expr;
                fprintf(salida_llvm, "define i32 @%s(", func->name);

                VarDeclarationNode** params = (VarDeclarationNode**)func->params;
                for (int i = 0; params && params[i]; i++) {
                    if (i > 0) fprintf(salida_llvm, ", ");
                    fprintf(salida_llvm, "i32 %%arg%d", i);
                }
                fprintf(salida_llvm, ") {\nentry:\n");

                // Almacenar parámetros en variables locales
                for (int i = 0; params && params[i]; i++) {
                    fprintf(salida_llvm, "  %%%s = alloca i32\n", params[i]->name);
                    fprintf(salida_llvm, "  store i32 %%arg%d, i32* %%%s\n", i, params[i]->name);
                }

                // Generar cuerpo de la función
                int retorno = generar_codigo(func->body);
                if (retorno != -1) {
                    fprintf(salida_llvm, "  ret i32 %%%d\n", retorno);
                } else {
                    fprintf(salida_llvm, "  ret i32 0\n");  // Valor por defecto si no hay return
                }

                fprintf(salida_llvm, "}\n\n");
                return -1;  // Las definiciones de función no devuelven un valor
            }
            case NODE_ASSIGN: {
                BinaryNode* bin = (BinaryNode*)expr;
                int right_val = get_valor(bin->right);
                const char* var_name = obtener_nombre_variable((VarNode*)bin->left);
                
                fprintf(salida_llvm, "  store i32 %%%d, i32* %%%s\n", right_val, var_name);
                return right_val;
            }
            case NODE_TYPE_DEF: {
            TypeDeclarationNode* type = (TypeDeclarationNode*)expr;
            fprintf(salida_llvm, "%%%s = type { ", type->name);

            TypeAttributeNode** attrs = (TypeAttributeNode**)type->attributes;
            for (int i = 0; attrs && attrs[i]; i++) {
                if (i > 0) fprintf(salida_llvm, ", ");
                VarType t;
                if (attrs[i]->type != NULL && strcmp(attrs[i]->type, "string") == 0) {
                    t = VAR_TYPE_STRING;
                } else {
                    t = VAR_TYPE_INT;
                }

                fprintf(salida_llvm, "%s", t == VAR_TYPE_STRING ? "i8*" : "i32");


            }

            fprintf(salida_llvm, " }\n");

            // (Opcional) Aquí podrías guardar información del tipo para uso futuro
            return -1;
        }

         case NODE_CALL_FUNC: {
                CallFuncNode* call = (CallFuncNode*)expr;
                ExpressionNode** args = (ExpressionNode**)call->arguments;

                if (strcmp(call->name, "print") == 0) {
                    ExpressionNode* arg = args[0];
                    int valor = generar_codigo(arg);
                    VarType tipo = VAR_TYPE_INT;
                    NodeType nodo_tipo = ((Node*)arg)->tipo;

                    if (nodo_tipo == NODE_STRING || nodo_tipo == NODE_CONCAT) {
                        tipo = VAR_TYPE_STRING;
                    } else if (nodo_tipo == NODE_VAR) {
                        const char* nombre = obtener_nombre_variable((VarNode*)arg);
                        tipo = obtener_tipo_variable(nombre);
                    }

                    if (tipo == VAR_TYPE_STRING) {
                        fprintf(salida_llvm, "  call void @print_str(i8* %%%d)\n", valor);
                    } else {
                        fprintf(salida_llvm, "  call void @print_int(i32 %%%d)\n", valor);
                    }
                    return -1;
                }

                int arg_values[10];  // Asumimos como máximo 10 args por simplicidad
            int arg_count = 0;

            // Primero: evaluar argumentos y generar instrucciones por separado
            for (int i = 0; args && args[i]; i++) {
                arg_values[arg_count++] = generar_codigo(args[i]);
            }

            // Segundo: emitir llamada con args ya evaluados
            int temp = nuevo_temp();
            fprintf(salida_llvm, "  %%%d = call i32 @%s(", temp, call->name);
            for (int i = 0; i < arg_count; i++) {
                if (i > 0) fprintf(salida_llvm, ", ");
                fprintf(salida_llvm, "i32 %%%d", arg_values[i]);
            }
            fprintf(salida_llvm, ")\n");
            return temp;
                    }

        case NODE_CONCAT: {
            BinaryNode* bin = (BinaryNode*)expr;
            int left = generar_codigo(bin->left);
            int right = generar_codigo(bin->right);

            VarType tipo_izq = VAR_TYPE_STRING;
            VarType tipo_der = VAR_TYPE_STRING;

            if (((Node*)bin->left)->tipo == NODE_VAR)
                tipo_izq = obtener_tipo_variable(obtener_nombre_variable((VarNode*)bin->left));
            if (((Node*)bin->right)->tipo == NODE_VAR)
                tipo_der = obtener_tipo_variable(obtener_nombre_variable((VarNode*)bin->right));

            if (((Node*)bin->left)->tipo == NODE_NUMBER)
                tipo_izq = VAR_TYPE_INT;
            if (((Node*)bin->right)->tipo == NODE_NUMBER)
                tipo_der = VAR_TYPE_INT;

            if (tipo_izq == VAR_TYPE_INT) {
                int conv = nuevo_temp();
                fprintf(salida_llvm, "  %%%d = call i8* @int_to_string(i32 %%%d)\n", conv, left);
                left = conv;
            }
            if (tipo_der == VAR_TYPE_INT) {
                int conv = nuevo_temp();
                fprintf(salida_llvm, "  %%%d = call i8* @int_to_string(i32 %%%d)\n", conv, right);
                right = conv;
            }

            int temp = nuevo_temp();
            fprintf(salida_llvm, "  %%%d = call i8* @strcat2(i8* %%%d, i8* %%%d)\n", temp, left, right);
            return temp;
        }

        case NODE_CALL_METHOD: {
            CallMethodNode* call = (CallMethodNode*)expr;
            VarNode* inst = calloc(1, sizeof(VarNode));
            inst->base.base.base.base.tipo = NODE_VAR;
            inst->base.lex = strdup(call->inst_name);
            int obj = get_valor((ExpressionNode*)inst);

            int temp = nuevo_temp();
            fprintf(salida_llvm, "  %%%d = call i32 @%s_%s(i32 %%%d", temp, call->inst_name, call->method_name, obj);
            // Implementación básica de argumentos
            if (call->method_args) {
                // Asumimos un solo argumento
                int arg = get_valor(call->method_args);
                fprintf(salida_llvm, ", i32 %%%d", arg);
            }
            fprintf(salida_llvm, ")\n");
            return temp;
        }

        case NODE_PRINT: {
                UnaryNode* un = (UnaryNode*)expr;
                ExpressionNode* arg = (ExpressionNode*)un->operand;

                NodeType tipo_arg = ((Node*)arg)->tipo;
                int valor = generar_codigo(arg);

                VarType tipo = VAR_TYPE_INT;

                if (tipo_arg == NODE_STRING || tipo_arg == NODE_CONCAT) {
                    tipo = VAR_TYPE_STRING;
                } else if (tipo_arg == NODE_VAR) {
                    const char* nombre = obtener_nombre_variable((VarNode*)arg);
                    tipo = obtener_tipo_variable(nombre);
                }

                if (tipo == VAR_TYPE_STRING) {
                    fprintf(salida_llvm, "  call void @print_str(i8* %%%d)\n", valor);
                } else {
                    fprintf(salida_llvm, "  call void @print_int(i32 %%%d)\n", valor);
                }

                return -1;
            }

        case NODE_RETURN: {
            ReturnNode* ret = (ReturnNode*)expr;
            int val = get_valor(ret->expr); // genera la expresión
            fprintf(salida_llvm, "  ret i32 %%%d\n", val);
            return -1; // nada se guarda, se sale
        }
        
        case NODE_BLOCK: {
            ExpressionBlockNode* block = (ExpressionBlockNode*)expr;
            ExpressionNode** exprs = (ExpressionNode**)block->expressions;
            int ultimo = -1;
            for (int i = 0; exprs && exprs[i]; i++) {
                ultimo = generar_codigo(exprs[i]);
            }
            return ultimo;
        }



        default:
            fprintf(salida_llvm, "; [TODO] Generación no implementada para tipo %d\n", tipo);
            return -1;
    }
}

const char* nuevo_tmp() {
    sprintf(last_tmp, "%%t%d", tmp_counter++);
    return last_tmp;
}

const char* tmp_actual() {
    return last_tmp;
}
// Registrar una nueva variable
void registrar_variables(ProgramNode* program) {
    if (!program || !program->declarations) return;

    VarDeclarationNode** decls = (VarDeclarationNode**)program->declarations;
    for (int i = 0; decls[i] != NULL; ++i) {
        // Aquí podrías registrar el nombre si lo deseas
        variables_usadas[num_variables].nombre = strdup(decls[i]->name);
        variables_usadas[num_variables].tipo = VAR_TYPE_INT; // o detectar por tipo real
        variables_usadas[num_variables].inicializada = 0;
        num_variables++;
    }
}

// Verificar si una variable existe
int variable_existe(const char* nombre) {
    for (int i = 0; i < num_variables; i++) {
        if (strcmp(variables_usadas[i].nombre, nombre) == 0) {
            return 1;
        }
    }
    return 0;
}
// Función auxiliar para obtener el valor de un nodo
int get_valor(ExpressionNode* node) {
    if (!node) return -1;
    return generar_codigo(node);
}

// Función auxiliar para generar código de expresiones binarias
int generar_binario(BinaryNode* bin, const char* opcode) {
    int izq = get_valor(bin->left);
    int der = get_valor(bin->right);
    int temp = nuevo_temp();
    fprintf(salida_llvm, "  %%%d = %s i32 %%%d, %%%d\n", temp, opcode, izq, der);
    return temp;
}

// Función auxiliar para generar comparaciones
int generar_comparacion(BinaryNode* bin, const char* cmp) {
    int izq = get_valor(bin->left);
    int der = get_valor(bin->right);
    int temp = nuevo_temp();
    fprintf(salida_llvm, "  %%%d = icmp %s i32 %%%d, %%%d\n", temp, cmp, izq, der);
    return temp;
}

// Genera declaración de variables
void generar_declaraciones_variables() {
    for (int i = 0; i < num_variables; i++) {
        // Asegurarse que el nombre no esté vacío
        if (variables_usadas[i].nombre && strlen(variables_usadas[i].nombre) > 0) {
            fprintf(salida_llvm, "  %%var_%s = alloca i32\n", variables_usadas[i].nombre);
            fprintf(salida_llvm, "  store i32 0, i32* %%var_%s\n", variables_usadas[i].nombre);
        }
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
                if (decls[i]->base.tipo == NODE_FUNCTION_DEF) {
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

void declare_extern_functions() {
    fprintf(salida_llvm, "declare i8* @strdup(i8*)\n");
    fprintf(salida_llvm, "declare i32 @llvm.pow.i32(i32, i32)\n");
    fprintf(salida_llvm, "declare void @print_int(i32)\n");
    fprintf(salida_llvm, "declare void @print_str(i8*)\n\n");
    fprintf(salida_llvm, "declare i8* @int_to_string(i32)\n");  
    fprintf(salida_llvm, "declare i8* @strcat2(i8*, i8*)\n\n");
}
void generar_programa(ProgramNode* program) {
    if (!program) {
        fprintf(stderr, "[ERROR] ProgramNode nulo\n");
        return;
    }

    fprintf(salida_llvm, "; Generado automáticamente por el compilador\n\n");
    declare_extern_functions();
    generar_constantes_globales(program);

    // 1. Primero genera todas las funciones (declaraciones)
    if (program->declarations) {
        DeclarationNode** decls = (DeclarationNode**)program->declarations;
        for (int i = 0; decls[i]; i++) {
            if (decls[i]->base.tipo == NODE_FUNCTION_DEF) {
                generar_codigo((ExpressionNode*)decls[i]);
            }
        }
    }

    // 2. Luego genera el main
    fprintf(salida_llvm, "define i32 @main() {\n");
    fprintf(salida_llvm, "entry:\n");

    registrar_variables(program);
    generar_declaraciones_variables();

    int last_temp = generar_codigo(program->expression);

    if (last_temp != -1) {
        fprintf(salida_llvm, "  ret i32 %%%d\n", last_temp);
    } else {
        fprintf(salida_llvm, "  ret i32 0\n");
    }

    fprintf(salida_llvm, "}\n");
}
void asegurar_declaracion(const char* var_name) {
    fprintf(salida_llvm, "  %%var_%s = alloca i32\n", var_name);
    fprintf(salida_llvm, "  store i32 0, i32* %%var_%s\n", var_name);
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

        case NODE_PRINT:
       
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


        case NODE_ADD: case NODE_SUB: case NODE_MUL:
        case NODE_DIV: case NODE_EQ: case NODE_LT:
        case NODE_GT: case NODE_AND: case NODE_OR:
        case NODE_POW: case NODE_MOD:
        case NODE_NEQ: case NODE_LTE: case NODE_GTE: {
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

        case NODE_BLOCK: {
            ExpressionBlockNode* block = (ExpressionBlockNode*)expr;
            ExpressionNode** exprs = (ExpressionNode**)block->expressions;
            for (int i = 0; exprs && exprs[i]; i++)
                recorrer_ast_para_strings(exprs[i]);
            break;
        }
        case NODE_IF: {
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
int registrar_string_global(const char* texto) {
    for (int i = 0; i < num_strings; i++) {
        if (strcmp(constantes_string[i].valor, texto) == 0)
            return constantes_string[i].id;  // Ya registrado
    }
    int nuevo_id = num_strings;
    constantes_string[nuevo_id].valor = strdup(texto);
    constantes_string[nuevo_id].id = nuevo_id;
    num_strings++;
    return nuevo_id;
}
void generar_funciones(ExpressionNode* expr) {
    if (!expr) return;
    NodeType tipo = ((Node*)expr)->tipo;

    switch (tipo) {
        case NODE_FUNCTION_DEF:
            generar_codigo(expr);
            break;

        case NODE_BLOCK: {
            ExpressionBlockNode* block = (ExpressionBlockNode*)expr;
            ExpressionNode** exprs = (ExpressionNode**)block->expressions;
            for (int i = 0; exprs && exprs[i]; i++) {
                generar_funciones(exprs[i]);
            }
            break;
        }

        case NODE_LET:
        case NODE_LET_IN: {
            LetInNode* let = (LetInNode*)expr;
            generar_funciones(let->body);
            break;
        }

        case NODE_IF: {
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
