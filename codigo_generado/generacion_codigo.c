#include "generacion_codigo.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
EntradaFuncion funciones[100];  // ← Aquí SÍ las defines
int total_funciones = 0;
int retorno_emitido = 0;
char contexto_funcion[64] = "main";

char* nombre_llvm(const char* base) {
    static char buffer[128];
    snprintf(buffer, sizeof(buffer), "var_%s_%s", contexto_funcion, base);
    return buffer;
}

void generar_programa(ProgramNode* program) {
    if (!program) {
        fprintf(stderr, "[ERROR] ProgramNode nulo\n");
        return;
    }

    fprintf(salida_llvm, "; Generado automáticamente por el compilador\n\n");

    // Declarar funciones externas (como print, strdup, etc.)
    declare_extern_functions();

    // Registrar strings globales que aparecerán en el .ll
    generar_constantes_globales(program);

    // 1. Generar todas las funciones declaradas (antes del main)
    if (program->declarations) {
        DeclarationNode** decls = (DeclarationNode**)program->declarations;
        for (int i = 0; decls[i]; i++) {
            if (decls[i]->base.tipo == NODE_FUNCTION_DECLARATION) {
                generar_codigo((ExpressionNode*)decls[i]);
            }
        }
    }

    // 2. Generar función principal (main)
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

int generar_codigo(ExpressionNode* expr) {
    if (!expr) return -1;
    expr = optimizar_constantes(expr);  // Aplicar optimización

    NodeType tipo = ((Node*)expr)->tipo;

    switch (tipo) {

        // ----- Literales -----
        case NODE_NUMBER: {
            LiteralNode* lit = (LiteralNode*)expr;

            if (strchr(lit->lex, '.')) {
                int temp = nuevo_temp();
                fprintf(salida_llvm, "  %%%d = fadd float 0.0, %s\n", temp, lit->lex);
                return temp;
            } else {
                int temp = nuevo_temp();
                fprintf(salida_llvm, "  %%%d = add i32 0, %s\n", temp, lit->lex);
                return temp;
            }
        }

        case NODE_BOOLEAN: {
            BooleanNode* bool_node = (BooleanNode*)expr;
            int temp = nuevo_temp();
            fprintf(salida_llvm, "  %%%d = add i1 0, %s\n", temp,
                    (strcmp(((LiteralNode*)bool_node)->lex, "true") == 0) ? "1" : "0");
            return temp;
        }

        case NODE_STRING: {
            StringNode* str = (StringNode*)expr;
            int temp = nuevo_temp();
            char* str_val = ((LiteralNode*)str)->lex;
            int len = strlen(str_val);

            int id = registrar_string_global(str_val);
            fprintf(salida_llvm,
                    "  %%%d = call i8* @strdup(i8* getelementptr inbounds ([%d x i8], [%d x i8]* @.str.%d, i32 0, i32 0))\n",
                    temp, len + 1, len + 1, id);

            return temp;
        }

        case NODE_PROGRAM: {
            ProgramNode* prog = (ProgramNode*)expr;
            if (prog->expression) {
                return generar_codigo(prog->expression);
            }
            return -1;
        }

        // ----- Variables y asignaciones -----
        case NODE_VAR: {
            VarNode* var = (VarNode*)expr;
            const char* var_name = obtener_nombre_variable(var);
            VarType tipo = obtener_tipo_variable(var_name);

            int temp = nuevo_temp();
            if (tipo == TIPO_STRING)
                fprintf(salida_llvm, "  %%%d = load i8*, i8** %%%s\n", temp, nombre_llvm(var_name));
            else
                fprintf(salida_llvm, "  %%%d = load i32, i32* %%%s\n", temp, nombre_llvm(var_name));

                        return temp;
                    }


        case NODE_ASSING: {
            BinaryNode* bin = (BinaryNode*)expr;
            int right_val = generar_codigo(bin->right);
            const char* var_name = obtener_nombre_variable((VarNode*)bin->left);

            VarType tipo = obtener_tipo_variable(var_name);
            const char* tipo_llvm = (tipo == VAR_TYPE_STRING) ? "i8*" : "i32";

            fprintf(salida_llvm, "  store %s %%%d, %s* %%%s\n", tipo_llvm, right_val, tipo_llvm, nombre_llvm(var_name));

            return right_val;
        }

        // ----- Let y Let-In -----
        
            case NODE_LET_IN: {
                LetInNode* let_in = (LetInNode*)expr;
                VarDeclarationNode** declarations = (VarDeclarationNode**)let_in->variables;

                for (int i = 0; declarations && declarations[i]; i++) {
                    VarDeclarationNode* decl = declarations[i];
                    const char* nombre = decl->name;
                    ExpressionNode* valor = decl->value;

                    // Inferir tipo real
                    LLVMType tipo = tipo_expr(valor);
                    const char* tipo_llvm = (tipo == TIPO_STRING) ? "i8*" : "i32";

                    // Registrar en entorno de variables
                    variables_usadas[num_variables].nombre = strdup(nombre);
                    variables_usadas[num_variables].tipo = (tipo == TIPO_STRING) ? VAR_TYPE_STRING : VAR_TYPE_INT;
                    variables_usadas[num_variables].inicializada = 1;
                    num_variables++;

                    // Emitir alloca
                    fprintf(salida_llvm, "  %%%s = alloca %s\n", nombre_llvm(nombre), tipo_llvm);


                    if (valor) {
                        int temp = generar_codigo(valor);

                        // Si viene de comparación y es i1, convertir a i32
                        if (tipo == TIPO_INT && es_comparacion(((Node*)valor)->tipo)) {
                            int conv = nuevo_temp();
                            fprintf(salida_llvm, "  %%%d = zext i1 %%%d to i32\n", conv, temp);
                            temp = conv;
                        }

                        fprintf(salida_llvm, "  store %s %%%d, %s* %%%s\n", tipo_llvm, temp, tipo_llvm, nombre_llvm(nombre));

                    } else {
                        fprintf(salida_llvm, "  store %s %s, %s* %%var_%s\n",
                            tipo_llvm,
                            (tipo == TIPO_STRING) ? "null" : "0",
                            tipo_llvm, nombre_llvm (nombre));
                    }
                }

                return generar_codigo((ExpressionNode*)let_in->body);
            }

        // ----- Operaciones aritméticas -----
        case NODE_PLUS: return generar_binario((BinaryNode*)expr, "add");
        case NODE_MINUS: return generar_binario((BinaryNode*)expr, "sub");
        case NODE_MULT: return generar_binario((BinaryNode*)expr, "mul");
        case NODE_DIV: return generar_binario((BinaryNode*)expr, "sdiv");
        case NODE_MOD: return generar_binario((BinaryNode*)expr, "srem");

        case NODE_POW: {
            BinaryNode* bin = (BinaryNode*)expr;
            int izq = generar_codigo(bin->left);
            int der = generar_codigo(bin->right);
            int temp = nuevo_temp();
            fprintf(salida_llvm, "  %%%d = call i32 @int_pow(i32 %%%d, i32 %%%d)\n", temp, izq, der);
            return temp;
        }
        // ----- Comparaciones -----
        case NODE_EQUAL:  return generar_comparacion((BinaryNode*)expr, "eq");
        case NODE_NOT_EQUAL: return generar_comparacion((BinaryNode*)expr, "ne");
        case NODE_LESS:  return generar_comparacion((BinaryNode*)expr, "slt");
        case NODE_LESS_EQUAL: return generar_comparacion((BinaryNode*)expr, "sle");
        case NODE_GREATER:  return generar_comparacion((BinaryNode*)expr, "sgt");
        case NODE_GREATER_EQUAL: return generar_comparacion((BinaryNode*)expr, "sge");

        // ----- Lógicos -----
        case NODE_AND: {
            BinaryNode* bin = (BinaryNode*)expr;
            int izq = generar_codigo(bin->left);
            int der = generar_codigo(bin->right);
            int temp = nuevo_temp();
            fprintf(salida_llvm, "  %%%d = and i1 %%%d, %%%d\n", temp, izq, der);
            return temp;
        }

        case NODE_OR: {
            BinaryNode* bin = (BinaryNode*)expr;
            int izq = generar_codigo(bin->left);
            int der = generar_codigo(bin->right);
            int temp = nuevo_temp();
            fprintf(salida_llvm, "  %%%d = or i1 %%%d, %%%d\n", temp, izq, der);
            return temp;
        }

        case NODE_NOT: {
            UnaryNode* un = (UnaryNode*)expr;
            int val = generar_codigo(un->operand);
            int temp = nuevo_temp();
            fprintf(salida_llvm, "  %%%d = xor i1 %%%d, 1\n", temp, val);
            return temp;
        }

            case NODE_CONDITIONAL: {
                ConditionalNode* cond = (ConditionalNode*)expr;
                ExpressionNode** conditions = (ExpressionNode**)cond->conditions;
                ExpressionNode** thens = (ExpressionNode**)cond->expressions;
                ExpressionNode* else_expr = (ExpressionNode*)cond->default_expre;

                // 1. Inferir tipo del if (usando el tipo del 'then' como referencia)
                LLVMType tipo_if = tipo_expr(thens[0]);
                const char* tipo_llvm = (tipo_if == TIPO_STRING) ? "i8*" : "i32";

                // 2. Generar condición
                int cond_val = generar_codigo(conditions[0]);
                NodeType cond_tipo = ((Node*)conditions[0])->tipo;
                int cond_final = cond_val;

                // Convertir a i1 si es necesario (para enteros no booleanos)
                if (!es_comparacion(cond_tipo) && cond_tipo != NODE_BOOLEAN) {
                    cond_final = nuevo_temp();
                    fprintf(salida_llvm, "  %%%d = icmp ne i32 %%%d, 0\n", cond_final, cond_val);
                }

                // 3. Etiquetas para control de flujo
                int label_then = nuevo_label();
                int label_else = nuevo_label();
                int label_end = nuevo_label();

                fprintf(salida_llvm, "  br i1 %%%d, label %%L%d, label %%L%d\n", 
                    cond_final, label_then, label_else);

                // 4. Bloque THEN
                fprintf(salida_llvm, "L%d:\n", label_then);
                int val_then = generar_codigo(thens[0]);
                int then_has_return = contiene_return(thens[0]);
                if (!then_has_return) {
                    fprintf(salida_llvm, "  br label %%L%d\n", label_end);
                }

                // 5. Bloque ELSE
                fprintf(salida_llvm, "L%d:\n", label_else);
                int val_else = else_expr ? generar_codigo(else_expr) : -1;
                int else_has_return = else_expr ? contiene_return(else_expr) : 0;
                if (!else_has_return) {
                    fprintf(salida_llvm, "  br label %%L%d\n", label_end);
                }

                // 6. Bloque END y PHI solo si ambas ramas NO retornan
                int resultado = -1;
                if (!then_has_return && !else_has_return) {
                    fprintf(salida_llvm, "L%d:\n", label_end);

                    if (val_then >= 0 && val_else >= 0) {
                        resultado = nuevo_temp();
                        fprintf(salida_llvm, "  %%%d = phi %s [%%%d, %%L%d], [%%%d, %%L%d]\n",
                                resultado, tipo_llvm,
                                val_then, label_then,
                                val_else, label_else);
                    }
                }

                return resultado;
            }


            // ----- While -----
                    case NODE_WHILE: {
                        WhileNode* wh = (WhileNode*)expr;
                        int label_start = nuevo_label();
                        int label_body  = nuevo_label();
                        int label_end   = nuevo_label();

                        // Salto inicial al chequeo de condición
                        fprintf(salida_llvm, "  br label %%L%d\n", label_start);

                        // START: evaluar condición
                        fprintf(salida_llvm, "L%d:\n", label_start);
                        int cond_val = generar_codigo(wh->condition);
                        NodeType cond_tipo = ((Node*)wh->condition)->tipo;
                        int cond_final = cond_val;

                        // Convertir a i1 si es necesario
                        if (!es_comparacion(cond_tipo) && cond_tipo != NODE_BOOLEAN) {
                            cond_final = nuevo_temp();
                            fprintf(salida_llvm, "  %%%d = icmp ne i32 %%%d, 0\n", cond_final, cond_val);
                        }

                        fprintf(salida_llvm, "  br i1 %%%d, label %%L%d, label %%L%d\n", cond_final, label_body, label_end);

                        // BODY
                        fprintf(salida_llvm, "L%d:\n", label_body);
                        generar_codigo(wh->body);
                        if (!contiene_return((ExpressionNode*)wh->body)) {
                            fprintf(salida_llvm, "  br label %%L%d\n", label_start);
                        }

                        // END
                        fprintf(salida_llvm, "L%d:\n", label_end);
                        return -1;
                    }


                        // ----- For -----
                case NODE_FOR: {
                    ForNode* fr = (ForNode*)expr;
                    CallFuncNode* rango = (CallFuncNode*)fr->iterable;
                    ExpressionNode** args = (ExpressionNode**)rango->arguments;

                    int start = generar_codigo(args[0]);
                    int end   = generar_codigo(args[1]);

                    VarNode* var_item = (VarNode*)fr->item;
                    const char* nombre = obtener_nombre_variable(var_item);

                    // Registrar variable en entorno
                    variables_usadas[num_variables].nombre = strdup(nombre);
                    variables_usadas[num_variables].tipo = VAR_TYPE_INT;
                    variables_usadas[num_variables].inicializada = 1;
                    num_variables++;

                    int label_start = nuevo_label();
                    int label_body  = nuevo_label();
                    int label_end   = nuevo_label();

                    // alloca + asignación inicial
                    fprintf(salida_llvm, "  %%var_%s = alloca i32\n", nombre);
                    fprintf(salida_llvm, "  store i32 %%%d, i32* %%var_%s\n", start,nombre_llvm( nombre));
                    fprintf(salida_llvm, "  br label %%L%d\n", label_start);

                    // Comienzo del bucle
                    fprintf(salida_llvm, "L%d:\n", label_start);
                    int current = nuevo_temp();
                    fprintf(salida_llvm, "  %%%d = load i32, i32* %%var_%s\n", current, nombre_llvm(nombre));

                    int cond = nuevo_temp();
                    fprintf(salida_llvm, "  %%%d = icmp slt i32 %%%d, %%%d\n", cond, current, end);
                    fprintf(salida_llvm, "  br i1 %%%d, label %%L%d, label %%L%d\n", cond, label_body, label_end);

                    // Cuerpo del bucle
                    fprintf(salida_llvm, "L%d:\n", label_body);
                    generar_codigo(fr->body);

                    if (!contiene_return((ExpressionNode*)fr->body)) {
                        int tmp = nuevo_temp();
                        fprintf(salida_llvm, "  %%%d = load i32, i32* %%var_%s\n", tmp, nombre_llvm(nombre));

                        int inc = nuevo_temp();
                        fprintf(salida_llvm, "  %%%d = add i32 %%%d, 1\n", inc, tmp);
                        fprintf(salida_llvm, "  store i32 %%%d, i32* %%var_%s\n", inc, nombre);
                        fprintf(salida_llvm, "  br label %%L%d\n", label_start);
                    }

                    // Fin del bucle
                    fprintf(salida_llvm, "L%d:\n", label_end);
                    return -1;
                }


        // ----- Definición de función -----
            case NODE_FUNCTION_DECLARATION: {

                FunctionDeclarationNode* func = (FunctionDeclarationNode*)expr;
                char contexto_anterior[64];
                strcpy(contexto_anterior, contexto_funcion);
                strcpy(contexto_funcion, func->name);
                // Guardar estado anterior del flag
                int retorno_prev = retorno_emitido;
                retorno_emitido = 0;

                // Inferir tipo de retorno
                LLVMType tipo = tipo_expr(func->body);
                const char* tipo_llvm = (tipo == TIPO_STRING) ? "i8*" :
                                        (tipo == TIPO_INT) ? "i32" : "void";

                // Parámetros
                VarDeclarationNode** parametros = (VarDeclarationNode**)func->params;
                int cantidad = 0;
                for (; parametros && parametros[cantidad]; cantidad++);

                fprintf(salida_llvm, "define %s @%s(", tipo_llvm, func->name);
                for (int i = 0; i < cantidad; i++) {
                    LLVMType tipo_param = tipo_expr(parametros[i]->value);
                    const char* tipo_param_llvm = (tipo_param == TIPO_STRING) ? "i8*" : "i32";
                    fprintf(salida_llvm, "%s %%arg%d", tipo_param_llvm, i);
                    if (i < cantidad - 1) fprintf(salida_llvm, ", ");
                }
                fprintf(salida_llvm, ") {\nentry:\n");

                for (int i = 0; i < cantidad; i++) {
                    const char* nombre = parametros[i]->name;
                    LLVMType tipo_param = tipo_expr(parametros[i]->value);
                    const char* tipo_param_llvm = (tipo_param == TIPO_STRING) ? "i8*" : "i32";

                    fprintf(salida_llvm, "  %%%s = alloca %s\n", nombre_llvm(nombre), tipo_param_llvm);
                    fprintf(salida_llvm, "  store %s %%arg%d, %s* %%%s\n", tipo_param_llvm, i, tipo_param_llvm, nombre_llvm(nombre));


                    // Registrar variable local
                    variables_usadas[num_variables].nombre = strdup(nombre);
                    variables_usadas[num_variables].tipo = (tipo_param == TIPO_STRING) ? VAR_TYPE_STRING : VAR_TYPE_INT;
                    variables_usadas[num_variables].inicializada = 1;
                    num_variables++;
                }

                // Registrar la función
                if (total_funciones < MAX_FUNCIONES) {
                    EntradaFuncion* f = &funciones[total_funciones++];
                    f->nombre = strdup(func->name);
                    f->tipo_retorno = tipo;
                    f->cantidad_param = cantidad;
                    for (int i = 0; i < cantidad; i++) {
                        f->tipos_param[i] = tipo_expr(parametros[i]->value);
                    }
                }

                generar_codigo(func->body);

                // Retorno por defecto si el usuario no lo especificó
                if (!retorno_emitido) {
                    if (strcmp(tipo_llvm, "i8*") == 0)
                        fprintf(salida_llvm, "  ret i8* null\n");
                    else if (strcmp(tipo_llvm, "i32") == 0)
                        fprintf(salida_llvm, "  ret i32 0\n");
                    else
                        fprintf(salida_llvm, "  ret void\n");
                }

                fprintf(salida_llvm, "}\n");

                // Restaurar flag anterior
                retorno_emitido = retorno_prev;
                strcpy(contexto_funcion, contexto_anterior);

                return -1;
                }

            // ----- Llamada a función -----
              case NODE_CALL_FUNC: {
                    CallFuncNode* call = (CallFuncNode*)expr;
                    ExpressionNode** args = (ExpressionNode**)call->arguments;

                    // Print integrado (caso especial)
                    if (strcmp(call->name, "print") == 0) {
                        ExpressionNode* arg = args[0];
                        int valor = generar_codigo(arg);

                        LLVMType tipo = tipo_expr(arg);  // 🔥 usa esto, no VAR_TYPE

                        if (tipo == TIPO_STRING) {
                            fprintf(salida_llvm, "  call void @print_str(i8* %%%d)\n", valor);
                        } else if (tipo == TIPO_INT) {
                            NodeType tipo_nodo = ((Node*)arg)->tipo;
                            if (es_comparacion(tipo_nodo)) {
                                int conv = nuevo_temp();
                                fprintf(salida_llvm, "  %%%d = zext i1 %%%d to i32\n", conv, valor);
                                valor = conv;
                            }
                            fprintf(salida_llvm, "  call void @print_int(i32 %%%d)\n", valor);
                        } else {
                            fprintf(stderr, "[ERROR] print: tipo no compatible para impresión\n");
                        }

                        return -1;
                    }


                    // Verificar si la función fue registrada
                    LLVMType tipo_ret = tipo_funcion(call->name);
                    if (tipo_ret == TIPO_DESCONOCIDO) {
                        fprintf(stderr, "[ERROR] Función '%s' no declarada antes de su uso.\n", call->name);
                        exit(1);
                    }

                    int arg_values[10];
                    LLVMType arg_types[10];
                    int arg_count = 0;
                    for (int i = 0; args && args[i]; i++) {
                        arg_values[arg_count] = generar_codigo(args[i]);
                        arg_types[arg_count] = tipo_expr(args[i]);
                        arg_count++;
                    }

                    const char* llvm_ret_type = (tipo_ret == TIPO_STRING) ? "i8*" :
                                                (tipo_ret == TIPO_INT) ? "i32" : "void";

                    if (strcmp(llvm_ret_type, "void") == 0) {
                        fprintf(salida_llvm, "  call void @%s(", call->name);
                        for (int i = 0; i < arg_count; i++) {
                            const char* tipo_arg = (arg_types[i] == TIPO_STRING) ? "i8*" : "i32";
                            if (i > 0) fprintf(salida_llvm, ", ");
                            fprintf(salida_llvm, "%s %%%d", tipo_arg, arg_values[i]);
                        }
                        fprintf(salida_llvm, ")\n");
                        return -1;
                    } else {
                        int temp = nuevo_temp();
                        fprintf(salida_llvm, "  %%%d = call %s @%s(", temp, llvm_ret_type, call->name);
                        for (int i = 0; i < arg_count; i++) {
                            LLVMType t = tipo_param_funcion(call->name, i);
                            const char* tipo_arg = (t == TIPO_STRING) ? "i8*" : "i32";

                            if (i > 0) fprintf(salida_llvm, ", ");
                            fprintf(salida_llvm, "%s %%%d", tipo_arg, arg_values[i]);
                        }
                        fprintf(salida_llvm, ")\n");
                        return temp;
                    }
                }
                case NODE_TYPE_DECLARATION: {
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
                    return -1;
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
                } else if (tipo == VAR_TYPE_FLOAT) {
                    int tmpstr = nuevo_temp();
                    fprintf(salida_llvm, "  %%%d = call i8* @float_to_string(float %%%d)\n", tmpstr, valor);
                    fprintf(salida_llvm, "  call void @print_str(i8* %%%d)\n", tmpstr);
                } else if (tipo == VAR_TYPE_BOOL) {
                    int tmpstr = nuevo_temp();
                    fprintf(salida_llvm, "  %%%d = call i8* @bool_to_string(i1 %%%d)\n", tmpstr, valor);
                    fprintf(salida_llvm, "  call void @print_str(i8* %%%d)\n", tmpstr);
                } else {
                    fprintf(salida_llvm, "  call void @print_int(i32 %%%d)\n", valor);
                }


                return -1;
            }
        
        // ----- Concatenación -----
            case NODE_CONCAT: {
                BinaryNode* bin = (BinaryNode*)expr;
                int left = generar_codigo(bin->left);
                int right = generar_codigo(bin->right);

                VarType tipo_izq = VAR_TYPE_STRING;
                VarType tipo_der = VAR_TYPE_STRING;

                // Detectar si alguno de los lados es entero
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

        // ----- Llamada a método -----
        case NODE_CALL_METHOD: {
            CallMethodNode* call = (CallMethodNode*)expr;

            // Crear nodo temporal para la instancia
            VarNode* inst = calloc(1, sizeof(VarNode));
            inst->base.base.base.base.tipo = NODE_VAR;
            inst->base.lex = strdup(call->inst_name);

            int obj = generar_codigo((ExpressionNode*)inst);
            int temp = nuevo_temp();

            fprintf(salida_llvm, "  %%%d = call i32 @%s_%s(i32 %%%d", temp, call->inst_name, call->method_name, obj);

            if (call->method_args) {
                int arg = generar_codigo(call->method_args);
                fprintf(salida_llvm, ", i32 %%%d", arg);
            }

            fprintf(salida_llvm, ")\n");
            return temp;
        }


         // ----- Return -----
        case NODE_RETURN: {
                ReturnNode* ret = (ReturnNode*)expr;

                if (!ret->expr) {
                    // return sin expresión → válido solo para funciones void
                    fprintf(salida_llvm, "  ret void\n");
                    retorno_emitido = 1;
                    return -1;
                }

                int val = generar_codigo(ret->expr);
                NodeType tipo_expre = ((Node*)ret->expr)->tipo;

                // Inferir tipo real
                LLVMType tipo = tipo_expr(ret->expr);

                if (tipo == TIPO_STRING) {
                    fprintf(salida_llvm, "  ret i8* %%%d\n", val);
                } else if (tipo == TIPO_INT || tipo == TIPO_DESCONOCIDO) {
                    // Si es una comparación, convertir i1 a i32
                    if (es_comparacion(tipo_expre)) {
                        int conv = nuevo_temp();
                        fprintf(salida_llvm, "  %%%d = zext i1 %%%d to i32\n", conv, val);
                        val = conv;
                    }
                    fprintf(salida_llvm, "  ret i32 %%%d\n", val);
                } else if (tipo == TIPO_VOID) {
                    fprintf(salida_llvm, "  ret void\n");
                } else {
                    fprintf(stderr, "[ERROR] Tipo de retorno no compatible\n");
                    fprintf(salida_llvm, "  ret i32 0\n");
                }

                retorno_emitido = 1;
                return -1;
            }

        // ----- Bloque -----
        case NODE_BLOCK: {
            ExpressionBlockNode* block = (ExpressionBlockNode*)expr;
            ExpressionNode** exprs = (ExpressionNode**)block->expressions;
            int ultimo = -1;

            for (int i = 0; exprs && exprs[i]; i++) {
                ultimo = generar_codigo(exprs[i]);
            }

            return ultimo;
        }

        // ----- Nodo desconocido -----
        default:
            fprintf(salida_llvm, "; [TODO] Generación no implementada para tipo %d\n", tipo);
            return -1;
    }
}
