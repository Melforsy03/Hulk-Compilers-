#include "codegen.h"
#include <stdio.h>
#include <string.h>

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
            printf("  %%%d = add i32 0, %s\n", temp, ((LiteralNode*)num)->lex);
            return temp;
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
                    printf("  %%%s = alloca i8*\n", decl->name); // Puntero a string
                    if (decl->value) {
                        int temp = generar_codigo(decl->value); // Genera el string
                        printf("  store i8* %%%d, i8** %%%s\n", temp, decl->name);
                    } else {
                        printf("  store i8* null, i8** %%%s\n", decl->name); // Inicializar a NULL si no hay valor
                    }
                } else {
                    printf("  %%%s = alloca i32\n", decl->name); // Entero
                    if (decl->value) {
                        int temp = generar_codigo(decl->value);
                        printf("  store i32 %%%d, i32* %%%s\n", temp, decl->name);
                    } else {
                        printf("  store i32 0, i32* %%%s\n", decl->name); // Inicializar a 0 si no hay valor
                    }
                }
            }
            
            // Generar el cuerpo del 'in'
            return generar_codigo(let_in->body);
        }
        case NODE_VAR: {
            VarNode* var = (VarNode*)expr;
            const char* var_name = obtener_nombre_variable(var);
            VarType tipo = obtener_tipo_variable(var_name);
            
            int temp = nuevo_temp();
            if (tipo == VAR_TYPE_STRING) {
                printf("  %%%d = load i8*, i8** %%%s\n", temp, var_name);
            } else {
                printf("  %%%d = load i32, i32* %%%s\n", temp, var_name);
            }
            return temp;
        }
        
        case NODE_BOOLEAN: {
            BooleanNode* bool_node = (BooleanNode*)expr;
            int temp = nuevo_temp();
            // Usamos el campo 'lex' de LiteralNode
            printf("  %%%d = add i1 0, %s\n", temp, 
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
            printf("  %%%d = call i8* @strdup(i8* getelementptr inbounds ([%d x i8], [%d x i8]* @.str.%d, i32 0, i32 0))\n",
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
            printf("  %%%d = call i32 @llvm.pow.i32(i32 %%%d, i32 %%%d)\n", temp, izq, der);
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
            printf("  %%%d = and i1 %%%d, %%%d\n", temp, izq, der);
            return temp;
        }
        case NODE_OR: {
            BinaryNode* bin = (BinaryNode*)expr;
            int izq = get_valor(bin->left);
            int der = get_valor(bin->right);
            int temp = nuevo_temp();
            printf("  %%%d = or i1 %%%d, %%%d\n", temp, izq, der);
            return temp;
        }
        case NODE_NOT: {
            UnaryNode* un = (UnaryNode*)expr;
            int val = get_valor(un->operand);
            int temp = nuevo_temp();
            printf("  %%%d = xor i1 %%%d, 1\n", temp, val);
            return temp;
        }

        // Control de flujo
        case NODE_IF: {
            ConditionalNode* cond = (ConditionalNode*)expr;
            int label_then = nuevo_label();
            int label_else = nuevo_label();
            int label_end = nuevo_label();

            int cond_val = get_valor(cond->conditions);
            printf("  br i1 %%%d, label %%L%d, label %%L%d\n", cond_val, label_then, label_else);

            printf("L%d:\n", label_then);
            generar_codigo(cond->expressions);
            printf("  br label %%L%d\n", label_end);

            printf("L%d:\n", label_else);
            if (cond->default_expre)
                generar_codigo(cond->default_expre);
            printf("  br label %%L%d\n", label_end);

            printf("L%d:\n", label_end);
            return -1;
        }
        case NODE_WHILE: {
            WhileNode* wh = (WhileNode*)expr;
            int label_start = nuevo_label();
            int label_body = nuevo_label();
            int label_end = nuevo_label();

            printf("  br label %%L%d\n", label_start);
            printf("L%d:\n", label_start);

            int cond = get_valor(wh->condition);
            printf("  br i1 %%%d, label %%L%d, label %%L%d\n", cond, label_body, label_end);

            printf("L%d:\n", label_body);
            generar_codigo(wh->body);
            printf("  br label %%L%d\n", label_start);

            printf("L%d:\n", label_end);
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

                printf("  %%%s = alloca i32\n", nombre);
                printf("  store i32 %%%d, i32* %%%s\n", start, nombre);
                printf("  br label %%L%d\n", label_start);

                printf("L%d:\n", label_start);
                int current = nuevo_temp();
                printf("  %%%d = load i32, i32* %%%s\n", current, nombre);

                int cond = nuevo_temp();
                printf("  %%%d = icmp slt i32 %%%d, %%%d\n", cond, current, end);
                printf("  br i1 %%%d, label %%L%d, label %%L%d\n", cond, label_body, label_end);

                printf("L%d:\n", label_body);
                generar_codigo(fr->body);

                int temp = nuevo_temp();
                printf("  %%%d = load i32, i32* %%%s\n", temp, nombre);
                int temp_inc = nuevo_temp();
                printf("  %%%d = add i32 %%%d, 1\n", temp_inc, temp);
                printf("  store i32 %%%d, i32* %%%s\n", temp_inc, nombre);
                printf("  br label %%L%d\n", label_start);

                printf("L%d:\n", label_end);
                return -1;
            }


            case NODE_FUNCTION_DEF: {
                FunctionDeclarationNode* func = (FunctionDeclarationNode*)expr;
                printf("define i32 @%s(", func->name);

                VarDeclarationNode** params = (VarDeclarationNode**)func->params;
                for (int i = 0; params && params[i]; i++) {
                    if (i > 0) printf(", ");
                    printf("i32 %%arg%d", i);
                }
                printf(") {\nentry:\n");

                for (int i = 0; params && params[i]; i++) {
                    printf("  %%%s = alloca i32\n", params[i]->name);
                    printf("  store i32 %%arg%d, i32* %%%s\n", i, params[i]->name);
                }

                int retorno = generar_codigo(func->body);
                if (retorno == -1) {
                    printf("  ret i32 0\n");
                }

                printf("}\n\n");
                return -1;
            }

            case NODE_TYPE_DEF: {
            TypeDeclarationNode* type = (TypeDeclarationNode*)expr;
            printf("%%%s = type { ", type->name);

            TypeAttributeNode** attrs = (TypeAttributeNode**)type->attributes;
            for (int i = 0; attrs && attrs[i]; i++) {
                if (i > 0) printf(", ");
                VarType t = attrs[i]->type;
                if (t == VAR_TYPE_INT) {
                    printf("i32");
                } else if (t == VAR_TYPE_STRING) {
                    printf("i8*");
                } else {
                    printf("i32"); // tipo por defecto si falta
                }
            }

            printf(" }\n");

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
                        printf("  call void @print_str(i8* %%%d)\n", valor);
                    } else {
                        printf("  call void @print_int(i32 %%%d)\n", valor);
                    }
                    return -1;
                }

                int temp = nuevo_temp();
                printf("  %%%d = call i32 @%s(", temp, call->name);
                for (int i = 0; args && args[i]; i++) {
                    if (i > 0) printf(", ");
                    int val = generar_codigo(args[i]);
                    printf("i32 %%%d", val);
                }
                printf(")\n");
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
                printf("  %%%d = call i8* @int_to_string(i32 %%%d)\n", conv, left);
                left = conv;
            }
            if (tipo_der == VAR_TYPE_INT) {
                int conv = nuevo_temp();
                printf("  %%%d = call i8* @int_to_string(i32 %%%d)\n", conv, right);
                right = conv;
            }

            int temp = nuevo_temp();
            printf("  %%%d = call i8* @strcat2(i8* %%%d, i8* %%%d)\n", temp, left, right);
            return temp;
        }

        case NODE_CALL_METHOD: {
            CallMethodNode* call = (CallMethodNode*)expr;
            int obj = get_valor(call->inst_name);
            int temp = nuevo_temp();
            printf("  %%%d = call i32 @%s_%s(i32 %%%d", temp, call->inst_name, call->method_name, obj);
            // Implementación básica de argumentos
            if (call->method_args) {
                // Asumimos un solo argumento
                int arg = get_valor(call->method_args);
                printf(", i32 %%%d", arg);
            }
            printf(")\n");
            return temp;
        }

        case NODE_PRINT: {
            UnaryNode* un = (UnaryNode*)expr;
            ExpressionNode* arg = (ExpressionNode*)un->operand;
            
            // Obtener el tipo del argumento (int o string)
            NodeType tipo_arg = ((Node*)arg)->tipo;
            int valor = generar_codigo(arg);
            
            // Llamar a print_int o print_str según el tipo
            if (tipo_arg == NODE_STRING) {
                printf("  call void @print_str(i8* %%%d)\n", valor);
            } else {
                printf("  call void @print_int(i32 %%%d)\n", valor);
            }
            return -1;
        }
        case NODE_RETURN: {
            ReturnNode* ret = (ReturnNode*)expr;
            int val = get_valor(ret->expr); // genera la expresión
            printf("  ret i32 %%%d\n", val);
            return -1; // nada se guarda, se sale
        }
        
        case NODE_BLOCK: {
            ExpressionBlockNode* block = (ExpressionBlockNode*)expr;
            // Implementación básica de bloques
            if (block->expressions) {
                // Asumimos lista simple de expresiones
                generar_codigo(block->expressions);
            }
            return -1;
        }

        default:
            printf("; [TODO] Generación no implementada para tipo %d\n", tipo);
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
    VarDeclarationNode** decls = (VarDeclarationNode**)program->declarations;
    for (int i = 0; decls && decls[i]; ++i) {
        registrar_variables(decls[i]->name);
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
    printf("  %%%d = %s i32 %%%d, %%%d\n", temp, opcode, izq, der);
    return temp;
}

// Función auxiliar para generar comparaciones
int generar_comparacion(BinaryNode* bin, const char* cmp) {
    int izq = get_valor(bin->left);
    int der = get_valor(bin->right);
    int temp = nuevo_temp();
    printf("  %%%d = icmp %s i32 %%%d, %%%d\n", temp, cmp, izq, der);
    return temp;
}

// Genera declaración de variables
void generar_declaraciones_variables() {
    for (int i = 0; i < num_variables; i++) {
        // Asegurarse que el nombre no esté vacío
        if (variables_usadas[i].nombre && strlen(variables_usadas[i].nombre) > 0) {
            printf("  %%var_%s = alloca i32\n", variables_usadas[i].nombre);
            printf("  store i32 0, i32* %%var_%s\n", variables_usadas[i].nombre);
        }
    }
}
void generar_constantes_globales(ProgramNode* program) {
    recorrer_ast_para_strings((ExpressionNode*)program->expression);
    for (int i = 0; i < num_strings; i++) {
        char* str_val = constantes_string[i].valor;
        int len = strlen(str_val);
        printf("@.str.%d = private unnamed_addr constant [%d x i8] c\"%s\\00\"\n",
               i, len + 1, str_val);
    }
}

void declare_extern_functions() {
    printf("declare i8* @strdup(i8*)\n");
    printf("declare i32 @llvm.pow.i32(i32, i32)\n");
    printf("declare void @print_int(i32)\n");
    printf("declare void @print_str(i8*)\n\n");
    printf("declare i8* @int_to_string(i32)\n");  
    printf("declare i8* @strcat2(i8*, i8*)\n\n");
}
void generar_programa(ProgramNode* program) {
    printf("; Generado automáticamente por el compilador\n\n");
    declare_extern_functions();
    generar_constantes_globales(program);
    printf("define i32 @main() {\n");
    printf("entry:\n");
    
    // 1. Registrar todas las variables del programa
    registrar_variables(program);
    
    // 2. Generar declaraciones
    generar_declaraciones_variables();
    // 3. Generar código principal
    int last_temp = generar_codigo(program->expression);
    
    // 4. Retornar
    if (last_temp != -1) {
        printf("  ret i32 %%%d\n", last_temp);
    } else {
        printf("  ret i32 0\n");
    }
    printf("}\n");
}
void asegurar_declaracion(const char* var_name) {
    printf("  %%var_%s = alloca i32\n", var_name);
    printf("  store i32 0, i32* %%var_%s\n", var_name);
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
        case NODE_LET: {
            LetInNode* let = (LetInNode*)expr;
            VarDeclarationNode** decls = (VarDeclarationNode**)let->variables;
            for (int i = 0; decls && decls[i]; i++) {
                recorrer_ast_para_strings((ExpressionNode*)decls[i]->value);
            }
            recorrer_ast_para_strings((ExpressionNode*)let->body);
            break;
        }
        case NODE_PRINT:
        case NODE_NOT:
        case NODE_RETURN: {
            UnaryNode* un = (UnaryNode*)expr;
            recorrer_ast_para_strings((ExpressionNode*)un->operand);
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
            recorrer_ast_para_strings((ExpressionNode*)call->arguments);
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
