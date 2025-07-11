#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

static void indent(CodeGenContext* ctx) {
  for (int i = 0; i < ctx->indent; ++i)
    fprintf(ctx->out, "  ");
}
void parse_type_members(ASTNode* node, const char* type_name, TypeTable* type_table) {
  if (!node) return;

  if (node->type == AST_BASES) {
    // Agrega bases a la entrada del tipo
    TypeEntry* t = lookup_type(type_table, type_name);
    if (!t) {
      fprintf(stderr, "Error: tipo '%s' no encontrado al agregar bases\n", type_name);
      return;
    }

    for (int i = 0; i < node->num_children; ++i) {
      const char* base_name = node->children[i]->value;

      if (t->num_bases >= 8) {
        fprintf(stderr, "Error: demasiadas bases para tipo '%s'\n", type_name);
        break;
      }

      strcpy(t->bases[t->num_bases++], base_name);
      printf("✅ %s hereda de %s\n", type_name, base_name);
    }
  } 


  else if (node->type == AST_STATEMENT_LIST) {
    for (int i = 0; i < node->num_children; ++i) {
      parse_type_members(node->children[i], type_name, type_table);
    }
  }

  else if (node->type == AST_ASSIGN || node->type == AST_VAR_DECL) {
    const char* attr_name = node->children[0]->value;
    const char* attr_type = "i32"; // o infiere del RHS
    add_member_to_type(type_table, type_name, attr_name, attr_type, NULL);
    printf("✅ Guardado atributo %s::%s\n", type_name, attr_name);
  }

  else if (node->type == AST_FUNCTION_DECL) {
    const char* func_name = node->children[0]->value;
    const char* return_type = "i32";
    ASTNode* body = NULL;

    if (node->num_children >= 3) {
      body = node->children[2];
    }

    add_member_to_type(type_table, type_name, func_name, return_type, body);
    printf("✅ Guardado método %s::%s\n", type_name, func_name);
  }

  else {
    // Recurse
    for (int i = 0; i < node->num_children; ++i) {
      parse_type_members(node->children[i], type_name, type_table);
    }
  }
}

int get_primary_value_member_index(TypeTable* table, const char* type_name) {
  TypeEntry* t = lookup_type(table, type_name);
  if (!t) {
    fprintf(stderr, "Tipo '%s' no encontrado\n", type_name);
    return -1;
  }

  int index = 0;
  Member* m = t->members;
  while (m) {
    if (strcmp(m->name, "value") == 0) return index;
    index++;
    m = m->next;
  }

  for (int i = 0; i < t->num_bases; ++i) {
    int base_idx = get_primary_value_member_index(table, t->bases[i]);
    if (base_idx >= 0) return index + base_idx;
  }
  return -1; // No tiene 'value'
}

static void register_function_return(CodeGenContext* ctx, const char* name, ASTNode* body) {
    int has_return = 0;
    for (int i = 0; i < body->num_children; ++i) {
        if (body->children[i]->type == AST_RETURN) {
            has_return = 1;
            break;
        }
    }

    strcpy(ctx->return_table.func_names[ctx->return_table.count], name);
    strcpy(ctx->return_table.return_types[ctx->return_table.count], has_return ? "i32" : "void");
    ctx->return_table.count++;
}

char* emit_virtual_call(CodeGenContext* ctx, Symbol* s_obj, const char* method_name) {
    // 1) Obtener puntero vptr
    int tmp0 = ctx->temp_count++;
    indent(ctx);
    fprintf(ctx->out, "%%%d = getelementptr %%%s, %%%s* %%%d, i32 0, i32 0\n",
          tmp0, s_obj->type, s_obj->type, s_obj->last_temp_id);

    int tmp1 = ctx->temp_count++;
    indent(ctx);
    fprintf(ctx->out, "%%%d = load i8**, i8*** %%%d\n", tmp1, tmp0);

    // 2) Índice del método
    int idx = get_method_index(ctx->type_table, s_obj->dynamic_type[0]? s_obj->dynamic_type : s_obj->type, method_name);
    int tmp2 = ctx->temp_count++;
    indent(ctx);
    fprintf(ctx->out,
        "%%%d = getelementptr [%d x i32 (%%%s*)*], [%d x i32 (%%%s*)*]* @%s_vtable, i32 0, i32 %d\n",
        tmp2, get_method_count(ctx->type_table, s_obj->type), s_obj->type,
        get_method_count(ctx->type_table, s_obj->type), s_obj->type,
        s_obj->type, idx);

    int tmp3 = ctx->temp_count++;
    indent(ctx);
    fprintf(ctx->out, "%%%d = load i32 (%%%s*)*, i32 (%%%s*)** %%%d\n",
          tmp3, s_obj->type, s_obj->type, tmp2);

    // 3) Llamada al método dinámico
    int tmp4 = ctx->temp_count++;
    indent(ctx);
    fprintf(ctx->out, "%%%d = call i32 %%%d(%%%s* %%%d)\n",
          tmp4, tmp3, s_obj->type, s_obj->last_temp_id);

    char* buffer = malloc(32);
    snprintf(buffer, 32, "%%%d", tmp4);
    return buffer;
}

int get_method_count(TypeTable* table, const char* type_name) {
    TypeEntry* t = lookup_type(table, type_name);
    return t ? t->method_count : 0;
}

static const char* get_function_return_type(CodeGenContext* ctx, const char* name) {
    for (int i = 0; i < ctx->return_table.count; ++i) {
        if (strcmp(ctx->return_table.func_names[i], name) == 0) {
            return ctx->return_table.return_types[i];
        }
    }
    return "i32"; // por defecto
}
static void register_all_function_returns(CodeGenContext* ctx, ASTNode* node) {
    if (node->type == AST_FUNCTION_DECL) {
        const char* name = node->children[0]->value;
        ASTNode* body = node->children[2];
        register_function_return(ctx, name, body);
    }

    for (int i = 0; i < node->num_children; ++i) {
        register_all_function_returns(ctx, node->children[i]);
    }
}
static char* codegen_expr(CodeGenContext* ctx, ASTNode* node) {
  switch (node->type) {
    case AST_RETURN: {
      char* val = codegen_expr(ctx, node->children[0]);
      indent(ctx);
      fprintf(ctx->out, "ret i32 %s\n", val);
      free(val);
  
      return strdup("0");
    }
    
    case AST_FUNCTION_CALL: {
      ASTNode* callee = node->children[0];
      ASTNode* args = node->children[1];
    
      if (callee->type == AST_IDENTIFIER) {
        const char* func_name = callee->value;
        Symbol* s = lookup(ctx->sym_table, func_name);
        if (!s) {
          fprintf(stderr, "Error: función global '%s' no declarada\n", func_name);
          exit(1);
        }
      
        // Guarda argumentos reales en last_call_args
        for (int i = 0; i < args->num_children; ++i) {
          ASTNode* arg = args->children[i];
          ctx->last_call_args[i] = (arg->type == AST_IDENTIFIER) ? lookup(ctx->sym_table, arg->value) : NULL;
        }
      
        const char* ret_type = s->type;
      
        char arg_str[256] = "";
        for (int i = 0; i < args->num_children; ++i) {
          char* arg_val = codegen_expr(ctx, args->children[i]);
          strcat(arg_str, arg_val);
          if (i < args->num_children - 1) strcat(arg_str, ", ");
          free(arg_val);
        }
      
        int temp = -1;
        if (strcmp(ret_type, "void") != 0) {
          temp = ctx->temp_count++;
          indent(ctx);
          fprintf(ctx->out, "%%%d = call i32 @%s(%s)\n", temp, func_name, arg_str);
        } else {
          indent(ctx);
          fprintf(ctx->out, "call void @%s(%s)\n", func_name, arg_str);
        }
      
        for (int i = 0; i < 8; ++i) ctx->last_call_args[i] = NULL;
      
        char* buffer = malloc(32);
        snprintf(buffer, 32, (temp >= 0) ? "%%%d" : "0", temp);
        return buffer;
      }
    
      // Método virtual
      else if (callee->type == AST_MEMBER) {
        ASTNode* obj_node = callee->children[0];
        ASTNode* method_node = callee->children[1];
      
        const char* obj_name = obj_node->value;
        const char* method_name = method_node->value;
      
        Symbol* s_obj = lookup(ctx->sym_table, obj_name);
        if (!s_obj) {
          fprintf(stderr, "Error: objeto '%s' no declarado\n", obj_name);
          exit(1);
        }
      
        return emit_virtual_call(ctx, s_obj, method_name);
      }
    
      else {
        fprintf(stderr, "Error: FunctionCall callee no soportado.\n");
        exit(1);
      }
    }

    case AST_NEW_EXPR: {
      const char* type_name = node->children[0]->value;  // Circle, Rectangle, etc.
      int temp = ctx->temp_count++;
      indent(ctx);
      fprintf(ctx->out, "%%%d = alloca %%%s\n", temp, type_name);
    
      char* buffer = malloc(32);
      snprintf(buffer, 32, "%%%d", temp);
      return buffer;  // Quien lo use (VAR_DECL) debe registrar dynamic_type!
    }


    case AST_MEMBER: {
      char* base = NULL;
      
      const char* var_name = node->children[0]->value;
      
      // Busca el símbolo
      Symbol* s = lookup(ctx->sym_table, var_name);
      if (!s) {
        fprintf(stderr, "Error: variable '%s' no declarada\n", var_name);
        exit(1);
      }
    
      // ✅ Si es 'this', usa literal %this
      if (strcmp(var_name, "this") == 0) {
        base = strdup("%this");
      } else {
        base = malloc(32);
        snprintf(base, 32, "%%%d", s->last_temp_id);
      }
    
      const char* struct_type = s->type;
      const char* member_name = node->children[1]->value;
    
      int index = get_member_index(ctx->type_table, struct_type, member_name);
      if (index == -1) {
        fprintf(stderr, "Error: miembro '%s' no existe en '%s'\n", member_name, struct_type);
        free(base);
        exit(1);
      }
    
      int gep = ctx->temp_count++;
      indent(ctx);
      fprintf(ctx->out,
        "%%%d = getelementptr %%%s, %%%s* %s, i32 0, i32 %d\n",
        gep, struct_type, struct_type, base, index);
      
      int load = ctx->temp_count++;
      indent(ctx);
      fprintf(ctx->out, "%%%d = load i32, i32* %%%d\n", load, gep);
      
      char* buffer = malloc(32);
      snprintf(buffer, 32, "%%%d", load);
      
      free(base);
      return buffer;
    }

    case AST_STRING: {
    // Crea un identificador único para el string literal
    static int string_count = 0;
    int id = string_count++;
    size_t len = strlen(node->value);

    // Declara el string literal en global
    fprintf(ctx->out, "@.str%d = private constant [%zu x i8] c\"%s\\00\"\n",
            id, len + 1, node->value);

    // Devuelve el nombre para usarlo en printf
    char* buffer = malloc(32);
    snprintf(buffer, 32, "@.str%d", id);
    return buffer;
    }


    case AST_NUMBER: {
      char* buffer = malloc(32);
      snprintf(buffer, 32, "%s", node->value);
      return buffer;
    }

    case AST_BOOL: {
      char* buffer = malloc(32);
      snprintf(buffer, 32, strcmp(node->value, "true") == 0 ? "1" : "0");
      return buffer;
    }

    case AST_IDENTIFIER: {
      Symbol* s = lookup(ctx->sym_table, node->value);
      if (!s) {
        fprintf(stderr, "Error: variable '%s' no declarada\n", node->value);
        exit(1);
      }
    
      // ⚠️ Si es struct, devuelve como puntero directamente
      if (lookup_type(ctx->type_table, s->type)) {
        char* buffer = malloc(32);
        snprintf(buffer, 32, "%%%d", s->last_temp_id);  // puntero ya alocado con alloca
        return buffer;
      }
    
      // Caso normal: escalar i32
      int temp = ctx->temp_count++;
      indent(ctx);
      fprintf(ctx->out, "%%%d = load i32, i32* %%%s\n", temp, node->value);
      char* buffer = malloc(32);
      snprintf(buffer, 32, "%%%d", temp);
      return buffer;
    }


    case AST_UNARYOP: {
      const char* op = node->value;
      char* expr = codegen_expr(ctx, node->children[0]);
      int temp = ctx->temp_count++;

      if (strcmp(op, "-") == 0) {
        indent(ctx);
        fprintf(ctx->out, "%%%d = sub i32 0, %s\n", temp, expr);
      } else if (strcmp(op, "+") == 0) {
        indent(ctx);
        fprintf(ctx->out, "%%%d = add i32 0, %s\n", temp, expr);
      } else if (strcmp(op, "!") == 0) {
        indent(ctx);
        fprintf(ctx->out, "%%%d = icmp eq i32 %s, 0\n", temp, expr);
        int temp2 = ctx->temp_count++;
        indent(ctx);
        fprintf(ctx->out, "%%%d = zext i1 %%%d to i32\n", temp2, temp);
        temp = temp2;
      } else {
        fprintf(stderr, "Error: operador unario no soportado: %s\n", op);
        exit(1);
      }

      char* buffer = malloc(32);
      snprintf(buffer, 32, "%%%d", temp);
      free(expr);
      return buffer;
    }
    case AST_INDEX: {
        char* base = codegen_expr(ctx, node->children[0]); // Identifier (v)
        char* idx = codegen_expr(ctx, node->children[1]);

        // Busca símbolo para tamaño
        Symbol* s = lookup(ctx->sym_table, node->children[0]->value);
        int size = 1; // default fallback
        if (s && strncmp(s->type, "Vector[", 7) == 0) {
            sscanf(s->type + 7, "%d", &size);
        }

        int gep_id = ctx->temp_count++;
        indent(ctx);
        fprintf(ctx->out,
            "%%%d = getelementptr [%d x i32], [%d x i32]* %s, i32 0, i32 %s\n",
            gep_id, size, size, base, idx);

        int load_id = ctx->temp_count++;
        indent(ctx);
        fprintf(ctx->out, "%%%d = load i32, i32* %%%d\n", load_id, gep_id);

        char* buffer = malloc(32);
        snprintf(buffer, 32, "%%%d", load_id);
        free(base);
        free(idx);
        return buffer;
        }


    case AST_BINOP: {
      char* left = codegen_expr(ctx, node->children[0]);
      char* right = codegen_expr(ctx, node->children[1]);
      int temp = ctx->temp_count++;

      const char* op = NULL;
      const char* rel = NULL;

      if (strcmp(node->value, "+") == 0) op = "add";
      else if (strcmp(node->value, "-") == 0) op = "sub";
      else if (strcmp(node->value, "*") == 0) op = "mul";
      else if (strcmp(node->value, "/") == 0) op = "sdiv";
      else if (strcmp(node->value, "and") == 0) rel = "and";
      else if (strcmp(node->value, "or") == 0) rel = "or";
      else if (strcmp(node->value, "==") == 0) rel = "eq";
      else if (strcmp(node->value, "!=") == 0) rel = "ne";
      else if (strcmp(node->value, "<") == 0) rel = "slt";
      else if (strcmp(node->value, "<=") == 0) rel = "sle";
      else if (strcmp(node->value, ">") == 0) rel = "sgt";
      else if (strcmp(node->value, ">=") == 0) rel = "sge";

      if (op) {
        indent(ctx);
        fprintf(ctx->out, "%%%d = %s i32 %s, %s\n", temp, op, left, right);
      } else if (rel) {
        indent(ctx);
        fprintf(ctx->out, "%%%d = icmp %s i32 %s, %s\n", temp, rel, left, right);
        int temp2 = ctx->temp_count++;
        indent(ctx);
        fprintf(ctx->out, "%%%d = zext i1 %%%d to i32\n", temp2, temp);
        temp = temp2;
      } else if (strcmp(node->value, "and") == 0 || strcmp(node->value, "or") == 0) {
        fprintf(stderr, "Operador lógico no implementado todavía: %s\n", node->value);
        exit(1);
      } else {
        fprintf(stderr, "Error: operador binario no soportado: %s\n", node->value);
        exit(1);
      }

      char* buffer = malloc(32);
      snprintf(buffer, 32, "%%%d", temp);
      free(left);
      free(right);
      return buffer;
    }
 
    case AST_ASSIGN: {
      ASTNode* lhs = node->children[0];
      ASTNode* rhs = node->children[1];
      char* expr = codegen_expr(ctx, rhs);
    
      char* buffer = malloc(32);
    
      if (lhs->type == AST_IDENTIFIER) {
        const char* name = lhs->value;
        indent(ctx);
        fprintf(ctx->out, "store i32 %s, i32* %%%s\n", expr, name);
      
        // ✅ Usa temp_count++ para el load
        int temp = ctx->temp_count++;
        indent(ctx);
        fprintf(ctx->out, "%%%d = load i32, i32* %%%s\n", temp, name);
      
        // ✅ Actualiza el símbolo con el %n real
        Symbol* s = lookup(ctx->sym_table, name);
        if (s) {
          s->last_temp_id = temp;
        } else {
          fprintf(stderr, "Error: símbolo '%s' no declarado\n", name);
          exit(1);
        }
      
        snprintf(buffer, 32, "%%%d", temp);
        free(expr);
        return buffer;  // Devuelve el %n real
      }
      if (lhs->type == AST_MEMBER) {
        const char* var_name = lhs->children[0]->value;
        const char* member_name = lhs->children[1]->value;
      
        Symbol* s = lookup(ctx->sym_table, var_name);
        if (!s) {
          fprintf(stderr, "Error: variable '%s' no declarada\n", var_name);
          exit(1);
        }
      
        const char* struct_type = s->type;
        int index = get_member_index(ctx->type_table, struct_type, member_name);
        if (index == -1) {
          fprintf(stderr, "Error: miembro '%s' no encontrado en '%s'\n", member_name, struct_type);
          exit(1);
        }
      
        int gep = ctx->temp_count++;
        indent(ctx);
        fprintf(ctx->out,
          "%%%d = getelementptr %%%s, %%%s* %%%d, i32 0, i32 %d\n",
          gep, struct_type, struct_type, s->last_temp_id, index);  // 🔥 Usa %n real
        
        indent(ctx);
        fprintf(ctx->out, "store i32 %s, i32* %%%d\n", expr, gep);
        
        char* buffer = malloc(32);
        snprintf(buffer, 32, "%%%d", gep);
        free(expr);
        return buffer;
      }
    
      fprintf(stderr, "Error: LHS de asignación no soportado.\n");
      exit(1);
    }
    case AST_TYPE_SPEC:
      // El nombre del tipo en programación: usamos el identificador interno
      return strdup(node->children[0]->value);
      
    //default:
    //  fprintf(stderr, "Error: tipo de nodo de expresión no soportado: %d\n", node->type);
    //  exit(1);

    default: {
      // Imprime el tipo numérico, el nombre si existe, y cuántos hijos tiene
      fprintf(stderr,
          "Error: nodo de expresión no soportado:\n"
          "  type_id    = %d\n"
          "  type_str   = %s\n"
          "  value      = '%s'\n"
          "  #children  = %d\n",
          node->type,
          node_type_to_str(node->type),
          node->value ? node->value : "",
          node->num_children
      );
      exit(1);
    }
  }
}

static void codegen_stmt(CodeGenContext* ctx, ASTNode* node) {
  switch (node->type) {
   
    case AST_TYPE_SPEC:
    break;
    case AST_PROGRAM:
      
    
    case AST_BLOCK:
    case AST_VAR_DECL_LIST:
    case AST_LET:
      for (int i = 0; i < node->num_children; ++i) {
          if (node->children[i]->type != AST_FUNCTION_DECL) {
          codegen_stmt(ctx, node->children[i]);
          }
      }
    break;
    case AST_FUNCTION_DECL: {
      const char* func_name = node->children[0]->value;
    
      // ⚡ Detecta si es método dentro de un tipo
      const char* mangled_name = func_name;
      const char* ret_type = get_function_return_type(ctx, func_name);
    
      if (ctx->sym_table) {
        // Si estamos dentro de un type, usa nombre mangled
        static char buffer[128];
        snprintf(buffer, sizeof(buffer), "%s_%s", ctx->sym_table, func_name);
        mangled_name = buffer;
      }
    
      ASTNode* params = node->children[1];
      ASTNode* body = node->children[2];
    
      // ⚡ Si es método, primer parámetro es el puntero 'this'
      if (ctx->sym_table) {
        fprintf(ctx->out, "define i32 @%s(%%%s* %%this) {\n", mangled_name, ctx->sym_table);
      } else {
        fprintf(ctx->out, "define %s @%s() {\n", ret_type, mangled_name);
      }
    
      ctx->indent++;
      codegen_stmt(ctx, body);
      ctx->indent--;
    
      fprintf(ctx->out, "}\n");
      break;
    }

      case AST_FOR: {
        ASTNode* var = node->children[0];
        ASTNode* expr = node->children[1];
        ASTNode* body = node->children[2];

        int label_id = ctx->temp_count++;

        // Inicializar iterador
        indent(ctx);
        fprintf(ctx->out, "%%%s = alloca i32\n", var->value);
        indent(ctx);
        fprintf(ctx->out, "store i32 0, i32* %%%s\n", var->value);

        // ⚡ Después de store: genera load + actualiza last_temp_id
        Symbol* s = lookup(ctx->sym_table, var->value);
        if (!s) { fprintf(stderr, "Variable '%s' no declarada\n", var->value); exit(1); }

        s->last_temp_id = ctx->temp_count++;
        indent(ctx);
        fprintf(ctx->out, "%%%d = load i32, i32* %%%s\n", s->last_temp_id, var->value);

        indent(ctx);
        fprintf(ctx->out, "br label %%for_cond%d\n", label_id);

        indent(ctx);
        fprintf(ctx->out, "for_cond%d:\n", label_id);
        char* cond = codegen_expr(ctx, expr);

        indent(ctx);
        fprintf(ctx->out, "%%cond%d = icmp slt i32 %%%d, %s\n",
                label_id, s->last_temp_id, cond);
        
        indent(ctx);
        fprintf(ctx->out, "br i1 %%cond%d, label %%for_body%d, label %%for_end%d\n",
                label_id, label_id, label_id);
        
        indent(ctx);
        fprintf(ctx->out, "for_body%d:\n", label_id);
        ctx->indent++;
        codegen_stmt(ctx, body);
        
        indent(ctx);
        fprintf(ctx->out, "%%inc%d = add i32 %%%d, 1\n", label_id, s->last_temp_id);
        indent(ctx);
        fprintf(ctx->out, "store i32 %%inc%d, i32* %%%s\n", label_id, var->value);
        
        // ⚡ Load actualizado para next iteración
        s->last_temp_id = ctx->temp_count++;
        indent(ctx);
        fprintf(ctx->out, "%%%d = load i32, i32* %%%s\n", s->last_temp_id, var->value);
        
        indent(ctx);
        fprintf(ctx->out, "br label %%for_cond%d\n", label_id);
        ctx->indent--;
        
        indent(ctx);
        fprintf(ctx->out, "for_end%d:\n", label_id);
        
        free(cond);
        break;
      }


      case AST_RETURN: {
        char* val = codegen_expr(ctx, node->children[0]);
        indent(ctx);
        fprintf(ctx->out, "ret i32 %s\n", val);
        free(val);
        break;
        }
      case AST_VAR_DECL: {
        const char* name = node->children[0]->value;
        ASTNode* expr_node = NULL;

        if (node->num_children == 3 && node->children[1]->type == AST_ASSIGN_OP) {
            expr_node = node->children[2];  // ignora :=
        } else {
            expr_node = node->children[1];
        }
      
        char* expr = codegen_expr(ctx, expr_node);
      
        // 🔧 Crear símbolo
        Symbol* s = malloc(sizeof(Symbol));
        strcpy(s->name, name);
      
        if (expr_node->type == AST_NEW_EXPR) {
            const char* struct_type = expr_node->children[0]->value;
            strcpy(s->type, struct_type);
            strcpy(s->dynamic_type, struct_type);
        
            // ⚠️ Almacenar puntero a struct (Circle*, Rectangle*)
            indent(ctx);
            fprintf(ctx->out, "%%%s = alloca %%%s*\n", name, s->type);  // %circle = alloca %Circle*
            indent(ctx);
            fprintf(ctx->out, "store %%%s* %s, %%%s** %%%s\n", s->type, expr, s->type, name);
        
            int temp = ctx->temp_count++;
            indent(ctx);
            fprintf(ctx->out, "%%%d = load %%%s*, %%%s** %%%s\n", temp, s->type, s->type, name);
            s->last_temp_id = temp;
        } else {
            // valor escalar
            strcpy(s->type, "i32");
            s->dynamic_type[0] = '\0';
        
            indent(ctx);
            fprintf(ctx->out, "%%%s = alloca i32\n", name);
            indent(ctx);
            fprintf(ctx->out, "store i32 %s, i32* %%%s\n", expr, name);
        
            int temp = ctx->temp_count++;
            indent(ctx);
            fprintf(ctx->out, "%%%d = load i32, i32* %%%s\n", temp, name);
            s->last_temp_id = temp;
        }
      
        s->kind = SYMBOL_VARIABLE;
        s->next = ctx->sym_table->head;
        ctx->sym_table->head = s;
      
        free(expr);
        break;
      }

case AST_ASSIGN: {
  char* val = codegen_expr(ctx, node);
  free(val);  // Descartas porque ya hizo store
  break;
}



 case AST_IF: {
  char* cond = codegen_expr(ctx, node->children[0]);
  int label_id = ctx->temp_count++;

  indent(ctx);
  fprintf(ctx->out, "%%cond%d = icmp ne i32 %s, 0\n", label_id, cond);

  if (node->num_children > 2) {
    // if-else
    indent(ctx);
    fprintf(ctx->out, "br i1 %%cond%d, label %%then%d, label %%else%d\n",
            label_id, label_id, label_id);

    indent(ctx);
    fprintf(ctx->out, "then%d:\n", label_id);
    ctx->indent++;
    codegen_stmt(ctx, node->children[1]);
    indent(ctx);
    fprintf(ctx->out, "br label %%endif%d\n", label_id);
    ctx->indent--;

    indent(ctx);
    fprintf(ctx->out, "else%d:\n", label_id);
    ctx->indent++;
    codegen_stmt(ctx, node->children[2]);
    indent(ctx);
    fprintf(ctx->out, "br label %%endif%d\n", label_id);
    ctx->indent--;

    indent(ctx);
    fprintf(ctx->out, "endif%d:\n", label_id);
  } else {
    // if simple
    indent(ctx);
    fprintf(ctx->out, "br i1 %%cond%d, label %%then%d, label %%endif%d\n",
            label_id, label_id, label_id);

    indent(ctx);
    fprintf(ctx->out, "then%d:\n", label_id);
    ctx->indent++;
    codegen_stmt(ctx, node->children[1]);
    indent(ctx);
    fprintf(ctx->out, "br label %%endif%d\n", label_id);
    ctx->indent--;

    indent(ctx);
    fprintf(ctx->out, "endif%d:\n", label_id);
  }
  free(cond);
  break;
}
case AST_WHILE: {
  int label_id = ctx->temp_count++;

  indent(ctx); fprintf(ctx->out, "br label %%loop_cond%d\n", label_id);
  indent(ctx); fprintf(ctx->out, "loop_cond%d:\n", label_id);

  char* cond = codegen_expr(ctx, node->children[0]);
  indent(ctx); fprintf(ctx->out, "%%cond%d = icmp ne i32 %s, 0\n", label_id, cond);

  indent(ctx); fprintf(ctx->out, "br i1 %%cond%d, label %%loop_body%d, label %%loop_end%d\n",
                       label_id, label_id, label_id);

  indent(ctx); fprintf(ctx->out, "loop_body%d:\n", label_id);
  ctx->indent++;
  codegen_stmt(ctx, node->children[1]); // ⚡ allow_ret = 0: sin ret dentro del bucle
  indent(ctx); fprintf(ctx->out, "br label %%loop_cond%d\n", label_id);
  ctx->indent--;

  indent(ctx); fprintf(ctx->out, "loop_end%d:\n", label_id);
  free(cond);
  break;
}


case AST_PRINT: {
    char* val = codegen_expr(ctx, node->children[0]);

    int temp = ctx->temp_count++;
    indent(ctx);
    fprintf(ctx->out,
        "%%%d = call i32 (i8*, ...) @printf(i8* getelementptr "
        "([4 x i8], [4 x i8]* @print.str, i32 0, i32 0), i32 %s)\n",
        temp, val);

    free(val);
    break;
}


    case AST_TYPE_DECL:
  // NO debes emitir nada para TypeDecl aquí; ya se manejan en semantic.c
  break;
case AST_FUNCTION_CALL: {
  const char* func_name = node->children[0]->value;
  const char* ret_type = get_function_return_type(ctx, func_name);

  char* val = codegen_expr(ctx, node);
  if (strcmp(ret_type, "void") != 0) {
    free(val);  // descarta retorno
  }
  break;
}


    case AST_BINOP: {
        // Si aparece un BinOp suelto en el cuerpo de función => asume return implícito
        char* val = codegen_expr(ctx, node);
        indent(ctx);
        fprintf(ctx->out, "ret i32 %s\n", val);
        free(val);
        break;
        }
        case AST_STATEMENT_LIST: {
            for (int i = 0; i < node->num_children; ++i) {
              ASTNode* child = node->children[i];
              if (child->type == AST_FUNCTION_DECL) continue;

              int is_last = (i == node->num_children - 1);
              int needs_ret = (child->type == AST_BINOP ||
                              child->type == AST_FUNCTION_CALL ||
                              child->type == AST_IDENTIFIER ||
                              child->type == AST_ASSIGN);

             if (is_last && needs_ret) {
  if (child->type == AST_FUNCTION_CALL &&
      child->children[0]->type == AST_IDENTIFIER &&
      strcmp(child->children[0]->value, "print") == 0) {
    codegen_stmt(ctx, child);  // sin return
  } else {
    const char* func_name = (child->type == AST_FUNCTION_CALL)
                              ? child->children[0]->value : NULL;
    const char* ret_type = (func_name) ? get_function_return_type(ctx, func_name) : "i32";

    char* val = codegen_expr(ctx, child);
    if (strcmp(ret_type, "void") != 0) {
      indent(ctx);
      fprintf(ctx->out, "ret i32 %s\n", val);
    }
    free(val);
  }
}
else {
                codegen_stmt(ctx, child);
              }
            }
            break;
          }


    default:
      fprintf(stderr, "Error: tipo de nodo de sentencia no soportado: %d\n", node->type);
      exit(1);
  }
}

void codegen_type_methods(CodeGenContext* ctx, TypeTable* type_table) {
  TypeEntry* t = type_table->head;
  while (t) {
      Member* m = t->members;
      while (m) {
        if (m->body != NULL) {
          indent(ctx);
          fprintf(ctx->out, "define i32 @%s_%s(%%%s* %%this) {\n", t->name, m->name, t->name);
          ctx->indent++;
          // ✅ Crea un scope local basado en el global
          SymbolTable local_scope = *ctx->sym_table;
          // ✅ Inserta 'this' como símbolo especial
          Symbol* this_sym = malloc(sizeof(Symbol));
          strcpy(this_sym->name, "this");
          strcpy(this_sym->type, t->name);
          this_sym->last_temp_id = -1; // especial: se usa literal %this
          this_sym->next = local_scope.head;
          local_scope.head = this_sym;
          // ✅ Usa el scope local solo dentro del método
          SymbolTable* prev_scope = ctx->sym_table;
          ctx->sym_table = &local_scope;
          // ✅ Genera cuerpo del método
          if (m->body->type == AST_STATEMENT_LIST) {
              codegen_stmt(ctx, m->body);
          } else {
              char* result = codegen_expr(ctx, m->body);
              indent(ctx);
              fprintf(ctx->out, "ret i32 %s\n", result);
              free(result);
          }
          indent(ctx);
          fprintf(ctx->out, "ret i32 0\n"); // fallback
          ctx->indent--;
          indent(ctx);
          fprintf(ctx->out, "}\n\n");
          ctx->sym_table = prev_scope; // Restaura scope global
        }
        m = m->next;
      }
      t = t->next;
  }
}

void collect_stmts_and_funcs(ASTNode* node, FuncBuffer* buf, ASTNode** main_stmts) {
  if (!node) return;

  if (node->type == AST_FUNCTION_DECL) {
    // Agrega función al buffer
    if (buf->count >= buf->capacity) {
      buf->capacity *= 2;
      buf->funcs = realloc(buf->funcs, sizeof(ASTNode*) * buf->capacity);
    }
    buf->funcs[buf->count++] = node;
  }

  else if (node->type == AST_TYPE_DECL) {
    // ⚡ NO vuelvas a insertar tipos aquí
    // La tabla de tipos ya está resuelta por semantic.c
  }

  else if (node->type == AST_STATEMENT_LIST || node->type == AST_PROGRAM) {
    for (int i = 0; i < node->num_children; ++i) {
      collect_stmts_and_funcs(node->children[i], buf, main_stmts);
    }
  }

  else {
    // Otro statement: guárdalo como parte del main
    if (*main_stmts == NULL) {
      *main_stmts = create_ast_node(AST_STATEMENT_LIST, NULL);
    }
    add_ast_child(*main_stmts, node);
  }
}

void debug_print_types(TypeTable* table) {
  printf("=== Type Table ===\n");
  TypeEntry* t = table->head;
  while (t) {
    printf("Type: %s\n", t->name);
    printf("  Bases: ");
    for (int i = 0; i < t->num_bases; ++i) {
      printf("%s ", t->bases[i]);
    }
    printf("\n");
    printf("  Members:\n");
    Member* m = t->members;
    while (m) {
      printf("    - %s\n", m->name);
      m = m->next;
    }
    t = t->next;
  }
}

void debug_print_symbols(SymbolTable* table) {
  printf("=== Symbol Table ===\n");
  if (!table) {
    printf("(null table)\n");
    return;
  }

  Symbol* current = table->head;
  if (!current) {
    printf("(empty)\n");
    return;
  }

  while (current) {
    printf("Name: %s\n", current->name);
    printf("  Type: %s\n", current->type);
    printf("  Last temp ID: %d\n", current->last_temp_id);
    printf("\n");
    current = current->next;
  }
}
void generate_code(CodeGenContext* ctx, ASTNode* root) {
  ctx->temp_count = 1;
  ctx->indent = 0;
  ctx->return_table.count = 0;

  // LLVM preámbulo: printf y formato
  fprintf(ctx->out, "declare i32 @printf(i8*, ...)\n");
  fprintf(ctx->out, "@print.str = constant [4 x i8] c\"%%d\\0A\\00\"\n\n");

  // Emitir tipos (structs) con vptr incluido
  emit_structs(ctx, ctx->type_table);
  emit_vtables(ctx, ctx->type_table);

  //  1) Genera todos los métodos de tipos usando TU version robusta
  codegen_type_methods(ctx, ctx->type_table);

  //  2) Registra todos los tipos de retorno de funciones globales
  register_all_function_returns(ctx, root);

  //  4) Recolecta funciones y statements del programa principal
  FuncBuffer buf;
  buf.funcs = malloc(sizeof(ASTNode*) * 4);
  buf.count = 0;
  buf.capacity = 4;

  ASTNode* main_stmts = NULL;
  collect_stmts_and_funcs(root, &buf, &main_stmts);

  //  5) Genera funciones globales normales
  for (int i = 0; i < buf.count; ++i) {
    codegen_function_decl(ctx, buf.funcs[i]);
  }

  //  6) Genera el main program
  fprintf(ctx->out, "define i32 @main() {\n");
  ctx->indent++;

  if (main_stmts) {
    codegen_stmt(ctx, main_stmts);
  }

  indent(ctx);
  fprintf(ctx->out, "ret i32 0\n");
  ctx->indent--;
  fprintf(ctx->out, "}\n");

  free(buf.funcs);
}

void codegen_function_decl(CodeGenContext* ctx, ASTNode* node) {
  const char* func_name = node->children[0]->value;
  ASTNode* params = node->children[1];
  ASTNode* body = node->children[2];

  const char* ret_type = get_function_return_type(ctx, func_name);

  // 1) Inserta parámetros en la symbol table
  for (int i = 0; i < params->num_children; ++i) {
    ASTNode* param = params->children[i];
    const char* param_name = param->value;

    const char* param_type = "i32";
    for (int j = 0; j < param->num_children; ++j) {
      if (param->children[j]->type == AST_TYPE_SPEC) {
        param_type = param->children[j]->children[0]->value;
      }
    }

    Symbol* s = malloc(sizeof(Symbol));
    strcpy(s->name, param_name);
    strcpy(s->type, param_type);
    s->last_temp_id = -1;
    s->next = ctx->sym_table->head;
    ctx->sym_table->head = s;
  }

  // 2) Firma LLVM: usa %Shape* si corresponde
  fprintf(ctx->out, "define %s @%s(", ret_type, func_name);
  for (int i = 0; i < params->num_children; ++i) {
    ASTNode* param = params->children[i];
    const char* param_type = "i32";
    for (int j = 0; j < param->num_children; ++j) {
      if (param->children[j]->type == AST_TYPE_SPEC) {
        param_type = param->children[j]->children[0]->value;
      }
    }

    if (strcmp(param_type, "Shape") == 0 || lookup_type(ctx->type_table, param_type)) {
      fprintf(ctx->out, "%%%s* %%%s", param_type, param->value); 
    } else {
      fprintf(ctx->out, "i32");
    }

    if (i < params->num_children - 1) fprintf(ctx->out, ", ");
  }
  fprintf(ctx->out, ") {\n");

  ctx->indent++;
  ctx->temp_count = params->num_children + 1;

  // 3) Alloca y guarda cada parámetro
  for (int i = 0; i < params->num_children; ++i) {
    ASTNode* param = params->children[i];
    const char* param_name = param->value;

    const char* param_type = "i32";
    for (int j = 0; j < param->num_children; ++j) {
      if (param->children[j]->type == AST_TYPE_SPEC) {
        param_type = param->children[j]->children[0]->value;
      }
    }
    if (strcmp(param_type, "Shape") == 0 || lookup_type(ctx->type_table, param_type)) {
      indent(ctx);
      fprintf(ctx->out, "%%%s_ptr = alloca %%%s*\n", param_name, param_type);
    
      indent(ctx);
      fprintf(ctx->out, "store %%%s* %%%s, %%%s** %%%s_ptr\n",
              param_type, param_name, param_type, param_name);
      
      int temp_id = ctx->temp_count++;
      indent(ctx);
      fprintf(ctx->out, "%%%d = load %%%s*, %%%s** %%%s_ptr\n",
              temp_id, param_type, param_type, param_name);
      
      Symbol* s_param = lookup(ctx->sym_table, param_name);
      if (s_param) {
        s_param->last_temp_id = temp_id;
      
        // ⚡ Hereda dynamic_type:
        Symbol* s_arg = ctx->last_call_args[i];
        if (s_arg && strlen(s_arg->dynamic_type) > 0) {
          strcpy(s_param->dynamic_type, s_arg->dynamic_type);
          printf("🔗 %s hereda dynamic_type = %s\n", param_name, s_param->dynamic_type);
        }
      }
    }

    else {
          // 🟢 Escalar normal
          indent(ctx);
          fprintf(ctx->out, "%%%s = alloca i32\n", param_name);
          indent(ctx);
          fprintf(ctx->out, "store i32 %%%d, i32* %%%s\n", i, param_name);
    
          int temp = ctx->temp_count++;
          indent(ctx);
          fprintf(ctx->out, "%%%d = load i32, i32* %%%s\n", temp, param_name);
    
            // Guarda correctamente el nombre y el ID:
      Symbol* s = lookup(ctx->sym_table, param_name);
      if (s) {
        s->last_temp_id = temp;   // Usado en emit_virtual_call
        strcpy(s->name, param_name); // ⚡ Necesario para bitcast correcto
      }
    }
  }

  // 4) Cuerpo de la función
  codegen_stmt(ctx, body);

  if (strcmp(ret_type, "void") == 0) {
    indent(ctx);
    fprintf(ctx->out, "ret void\n");
  }

  ctx->indent--;
  fprintf(ctx->out, "}\n");
}

TypeInstance* instantiate_generic(TypeTable* table, const char* template_name, const char** args, int num_args) {
  // 1) Busca la plantilla base
  TypeEntry* tmpl = table->head;
  while (tmpl && strcmp(tmpl->name, template_name) != 0) {
    tmpl = tmpl->next;
  }
  if (!tmpl) {
    printf("Error: plantilla genérica '%s' no encontrada\n", template_name);
    return NULL;
  }

  // 2) Construye nombre instanciado
  char inst_name[64];
  snprintf(inst_name, sizeof(inst_name), "%s[%s]", template_name, args[0]);
  for (int i = 1; i < num_args; ++i) {
    strcat(inst_name, ",");
    strcat(inst_name, args[i]);
  }

  // 3) Verifica si ya existe
  TypeInstance* inst = table->instances;
  while (inst) {
    if (strcmp(inst->name, inst_name) == 0) return inst; // Ya existe
    inst = inst->next;
  }

  // 4) Crea nueva instancia
  TypeInstance* new_inst = malloc(sizeof(TypeInstance));
  strcpy(new_inst->name, inst_name);
  strcpy(new_inst->base_template, template_name);
  new_inst->num_args = num_args;
  for (int i = 0; i < num_args; ++i) {
    strcpy(new_inst->type_args[i], args[i]);
  }

  // 5) Concretiza miembros
  Member* m = tmpl->members;
  Member* prev = NULL;
  while (m) {
    Member* cm = malloc(sizeof(Member));
    strcpy(cm->name, m->name);

    // Reemplaza parámetros genéricos por argumentos concretos
    int param_matched = 0;
    for (int i = 0; i < tmpl->num_params; ++i) {
      if (strcmp(m->type, tmpl->type_params[i]) == 0) {
        strcpy(cm->type, args[i]);
        param_matched = 1;
        break;
      }
    }
    if (!param_matched) {
      strcpy(cm->type, m->type);
    }

    cm->next = NULL;

    if (prev) prev->next = cm;
    else new_inst->concretized_members = cm;
    prev = cm;

    m = m->next;
  }

  // 6) Agrega a la tabla de instancias
  new_inst->next = table->instances;
  table->instances = new_inst;

  return new_inst;
}

void emit_structs(CodeGenContext* ctx, TypeTable* table) {
  TypeEntry* t = table->head;
  while (t) {
    //fprintf(ctx->out, "%%%s = type { ", t->name);
    fprintf(ctx->out, "%%%s = type { i8**", t->name);

    int need_comma = 1;

    // Miembros propios
    Member* m = t->members;
    while (m) {
        fprintf(ctx->out, ", i32");
        m = m->next;
    }
    // while (m) {
    //   if (need_comma) fprintf(ctx->out, ", ");
    //   fprintf(ctx->out, "i32");
    //   m = m->next;
    //   need_comma = 1;
    // }

    // Miembros heredados recursivos
    for (int i = 0; i < t->num_bases; ++i) {
      TypeEntry* base = lookup_type(table, t->bases[i]);
      if (base) {
        // ⚡ Recurre bases anidadas
        Member* bm = base->members;
        while (bm) {
          if (need_comma) fprintf(ctx->out, ", ");
          fprintf(ctx->out, "i32");
          bm = bm->next;
          need_comma = 1;
        }
        // ⚡ Recurre bases de bases
        for (int j = 0; j < base->num_bases; ++j) {
          TypeEntry* nested = lookup_type(table, base->bases[j]);
          if (nested) {
            Member* nm = nested->members;
            while (nm) {
              if (need_comma) fprintf(ctx->out, ", ");
              fprintf(ctx->out, "i32");
              nm = nm->next;
              need_comma = 1;
            }
          }
        }
      }
    }

    fprintf(ctx->out, " }\n");
    t = t->next;
  }
}

void emit_vtables(CodeGenContext* ctx, TypeTable* table) {
  TypeEntry* t = table->head;
  while (t) {
    int n = t->method_count;
    // Definir la tabla global
    fprintf(ctx->out, "@%s_vtable = global [%d x i32 (%%%s*)*] [\n", t->name, n, t->name);
    // Inicializar punteros a métodos
    for (int i = 0; i < n; ++i) {
      fprintf(ctx->out, "  i32 (%%%s*)* @%s_%s%s\n", t->name, t->name, t->method_names[i], (i < n-1) ? "," : "");
    }
    fprintf(ctx->out, "]\n\n");
    t = t->next;
  }
}

int get_member_index(TypeTable* table, const char* type_name, const char* member_name) {
  TypeEntry* t = lookup_type(table, type_name);
  if (!t) {
    fprintf(stderr, "Error: tipo '%s' no encontrado\n", type_name);
    return -1;
  }

  int index = 0;
  Member* m = t->members;
  while (m) {
    if (strcmp(m->name, member_name) == 0) {
      return index;
    }
    index++;
    m = m->next;
  }

  for (int i = 0; i < t->num_bases; ++i) {
    int base_index = get_member_index(table, t->bases[i], member_name);
    if (base_index != -1) {
      return index + base_index;  // ⚡ Corrige offset acumulado
    }
  }

  return -1;
}
