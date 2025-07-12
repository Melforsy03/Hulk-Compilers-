#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "semantic.h"
static void indent(CodeGenContext* ctx) {
  for (int i = 0; i < ctx->indent; ++i)
    fprintf(ctx->out, "  ");
}
int already_exists(TypeEntry* t, const char* name) {
  Member* m = t->members;
  while (m) {
    if (strcmp(m->name, name) == 0) return 1;
    m = m->next;
  }
  return 0;
}
int lookup_method_slot(TypeTable* table, const char* type_name, const char* method_name) {
  TypeEntry* t = lookup_type(table, type_name);
  if (!t) {
    fprintf(stderr, "Error: tipo '%s' no encontrado\n", type_name);
    exit(1);
  }

  for (int i = 0; i < t->method_count; ++i) {
    if (strcmp(t->method_names[i], method_name) == 0) {
      return i; // slot correcto en la vtable
    }
  }

  fprintf(stderr, "Error: método '%s' no encontrado en '%s'\n", method_name, type_name);
  exit(1);
}

Symbol* create_symbol(const char* name, const char* type, SymbolKind kind) {
    Symbol* s = malloc(sizeof(Symbol));
    if (!s) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    memset(s, 0, sizeof(Symbol));

    strncpy(s->name, name, sizeof(s->name)-1);
    strncpy(s->type, type, sizeof(s->type)-1);
    strncpy(s->dynamic_type, "", sizeof(s->dynamic_type)-1);
    strncpy(s->last_branch_dynamic_type, "", sizeof(s->last_branch_dynamic_type)-1);

    s->kind = kind;
    s->next = NULL;
    s->last_temp_id = -1;

    return s;
}

const char* find_common_base(TypeTable* type_table, const char* t1, const char* t2) {
  if (strcmp(t1, t2) == 0) return t1;

  if (conforms(type_table, t1, t2)) return t2;
  if (conforms(type_table, t2, t1)) return t1;

  TypeEntry* entry1 = lookup_type(type_table, t1);
  if (entry1) {
    for (int i = 0; i < entry1->num_bases; ++i) {
      const char* common = find_common_base(type_table, entry1->bases[i], t2);
      if (strcmp(common, "Object") != 0) return common;
    }
  }

  TypeEntry* entry2 = lookup_type(type_table, t2);
  if (entry2) {
    for (int i = 0; i < entry2->num_bases; ++i) {
      const char* common = find_common_base(type_table, t1, entry2->bases[i]);
      if (strcmp(common, "Object") != 0) return common;
    }
  }

  // Fallback dinámico: si la raíz es Object, mantén t1
  const char* root = find_type_table_root(type_table);

  if (strcmp(root, "Object") == 0) {
    // Mantiene el tipo original para evitar bitcast inválido
    return t1;
  } else {
    return root;
  }
}

const char* find_type_table_root(TypeTable* table) {
  TypeEntry* entry = table->head;
  while (entry) {
    // Si solo tiene Object como base, se considera raíz real
    if (entry->num_bases == 0) {
      return entry->name;
    }
    if (entry->num_bases == 1 && strcmp(entry->bases[0], "Object") == 0) {
      return entry->name;  // Es raíz, no tiene base real
    }
    entry = entry->next;
  }
  fprintf(stderr, "Error: no se encontró tipo raíz\n");
  exit(1);
}

void parse_type_members(ASTNode* node, const char* type_name, TypeTable* type_table) {
  if (!node) return;

  if (node->type == AST_BASES) {
    TypeEntry* t = lookup_type(type_table, type_name);
    if (!t) return;
    for (int i = 0; i < node->num_children; ++i) {
      const char* base_name = node->children[i]->value;
      strcpy(t->bases[t->num_bases++], base_name);
    }
  }

  else if (node->type == AST_STATEMENT_LIST) {
    // ⚡ Visita en orden
    for (int i = 0; i < node->num_children; ++i) {
      parse_type_members(node->children[i], type_name, type_table);
    }
  }

  else if (node->type == AST_ASSIGN) {
    // Atributo de dato => no afecta VTABLE
    const char* attr_name = node->children[0]->value;
    int default_value = 0;
    if (node->num_children > 1 && node->children[1]->type == AST_NUMBER) {
      default_value = atoi(node->children[1]->value);
    }
    add_member_to_type(type_table, type_name, attr_name, "i32", NULL, default_value);
  }

  else if (node->type == AST_FUNCTION_DECL) {
    // ⚡ Método => influye en VTABLE
    const char* func_name = node->children[0]->value;
    ASTNode* body = (node->num_children >= 3) ? node->children[2] : NULL;

    // ⚡ Siempre append al final para mantener orden de slots
    add_member_to_type(type_table, type_name, func_name, "i32", body, 0);
  }

  else {
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
    int tmp0 = ctx->temp_count++;
    indent(ctx);
    fprintf(ctx->out, "%%%d = getelementptr %%%s, %%%s* %%%d, i32 0, i32 0\n",
        tmp0, s_obj->type, s_obj->type, s_obj->last_temp_id);

    int tmp1 = ctx->temp_count++;
    indent(ctx);
    fprintf(ctx->out, "%%%d = load i8**, i8*** %%%d\n", tmp1, tmp0);

    // ✅ Usa dynamic_type si está
    const char* dyn_type = strlen(s_obj->dynamic_type) > 0 ? s_obj->dynamic_type : s_obj->type;

    int idx = lookup_method_slot(ctx->type_table, dyn_type, method_name);


    int tmp2 = ctx->temp_count++;
    indent(ctx);
    fprintf(ctx->out, "%%%d = getelementptr i8*, i8** %%%d, i32 %d\n", tmp2, tmp1, idx);

    int tmp3 = ctx->temp_count++;
    indent(ctx);
    fprintf(ctx->out, "%%%d = load i8*, i8** %%%d\n", tmp3, tmp2);

    int tmp_func_cast = ctx->temp_count++;
    indent(ctx);
    fprintf(ctx->out, "%%%d = bitcast i8* %%%d to i32 (%%%s*)*\n",
        tmp_func_cast, tmp3, dyn_type);

    int tmp_cast = ctx->temp_count++;
    indent(ctx);
    fprintf(ctx->out, "%%%d = bitcast %%%s* %%%d to %%%s*\n",
        tmp_cast, s_obj->type, s_obj->last_temp_id, dyn_type);

    int tmp4 = ctx->temp_count++;
    indent(ctx);
    fprintf(ctx->out, "%%%d = call i32 %%%d(%%%s* %%%d)\n",
        tmp4, tmp_func_cast, dyn_type, tmp_cast);

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

    char arg_list[1024] = "";
    for (int i = 0; i < args->num_children; ++i) {
      char* arg_val = codegen_expr(ctx, args->children[i]);
      strcat(arg_list, "i32 ");
      strcat(arg_list, arg_val);
      if (i < args->num_children - 1) strcat(arg_list, ", ");
      free(arg_val);
    }

    int temp = ctx->temp_count++;
    indent(ctx);
    fprintf(ctx->out, "%%%d = call i32 @%s(%s)\n", temp, func_name, arg_list);

    char* buffer = malloc(32);
    snprintf(buffer, 32, "%%%d", temp);
    return buffer;
  }

  else if (callee->type == AST_MEMBER) {
    const char* obj_name = callee->children[0]->value;
    const char* method_name = callee->children[1]->value;

    Symbol* s_obj = lookup(ctx->sym_table, obj_name);
    if (!s_obj) {
      fprintf(stderr, "Error: objeto '%s' no declarado\n", obj_name);
      exit(1);
    }
    s_obj->last_branch_dynamic_type[0] = '\0';

    ctx->last_call_args[0] = s_obj;

    return emit_virtual_call(ctx, s_obj, method_name);
  }

  else {
    fprintf(stderr, "Error: tipo de llamada no soportado\n");
    exit(1);
  }
}

case AST_NEW_EXPR: {
  const char* type_name = node->children[0]->value;

  int alloc_temp = ctx->temp_count++;
  indent(ctx);
  fprintf(ctx->out, "%%%d = alloca %%%s\n", alloc_temp, type_name);

  int vtable_ptr_temp = ctx->temp_count++;
  indent(ctx);
  fprintf(ctx->out, "%%%d = getelementptr %%%s, %%%s* %%%d, i32 0, i32 0\n",
          vtable_ptr_temp, type_name, type_name, alloc_temp);

  indent(ctx);
  fprintf(ctx->out, "store i8** @%s_vtable, i8*** %%%d\n",
          type_name, vtable_ptr_temp);

  // Inicializa miembros
  TypeEntry* tentry = lookup_type(ctx->type_table, type_name);
  int index = 1;
  if (tentry) {
    Member* m = tentry->members;
    while (m) {
      if (m->body == NULL) {
        int field_ptr_temp = ctx->temp_count++;
        indent(ctx);
        fprintf(ctx->out, "%%%d = getelementptr %%%s, %%%s* %%%d, i32 0, i32 %d\n",
                field_ptr_temp, type_name, type_name, alloc_temp, index);
        indent(ctx);
        fprintf(ctx->out, "store i32 %d, i32* %%%d\n",
                m->default_value, field_ptr_temp);
        index++;
      }
      m = m->next;
    }
  }

  // ✅ Fallback base_type coherente
  const char* base_type = type_name;  // por defecto = self
  if (tentry && tentry->num_bases > 0) {
    base_type = tentry->bases[0];
  }

  // Si base no existe, fallback al tipo real
  TypeEntry* base_entry = lookup_type(ctx->type_table, base_type);
  if (!base_entry) {
    base_type = type_name;
  }

  int cast_temp = alloc_temp;
  if (strcmp(base_type, type_name) != 0) {
    cast_temp = ctx->temp_count++;
    indent(ctx);
    fprintf(ctx->out, "%%%d = bitcast %%%s* %%%d to %%%s*\n",
            cast_temp, type_name, alloc_temp, base_type);
  }

  char* buffer = malloc(32);
  snprintf(buffer, 32, "%%%d", cast_temp);
  return buffer;
}

case AST_MEMBER: {
  char* obj_expr = codegen_expr(ctx, node->children[0]);
  const char* member_name = node->children[1]->value;

  Symbol* s = lookup(ctx->sym_table, node->children[0]->value);
  if (!s) {
    fprintf(stderr, "Error: objeto '%s' no declarado\n", node->children[0]->value);
    exit(1);
  }
  s->last_branch_dynamic_type[0] = '\0';

  const char* struct_type = strlen(s->dynamic_type) > 0 ? s->dynamic_type : s->type;

  int index = get_member_index(ctx->type_table, struct_type, member_name);
  if (index == -1) {
    fprintf(stderr, "Error: miembro '%s' no encontrado en '%s'\n", member_name, struct_type);
    exit(1);
  }

  int gep = ctx->temp_count++;
  indent(ctx);
  fprintf(ctx->out, "%%%d = getelementptr %%%s, %%%s* %s, i32 0, i32 %d\n",
          gep, struct_type, struct_type, obj_expr, index);

  int load_id = ctx->temp_count++;
  indent(ctx);
  fprintf(ctx->out, "%%%d = load i32, i32* %%%d\n", load_id, gep);

  free(obj_expr);
  char* buffer = malloc(32);
  snprintf(buffer, 32, "%%%d", load_id);
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
  s->last_branch_dynamic_type[0] = '\0';

  int temp = ctx->temp_count++;

  if (strcmp(s->name, "this") == 0) {
    indent(ctx);
    fprintf(ctx->out, "%%%d = bitcast %%%s* %%this to %%%s*\n",
            temp, s->dynamic_type, s->dynamic_type);
  } else if (lookup_type(ctx->type_table, s->type)) {
    indent(ctx);
    fprintf(ctx->out, "%%%d = load %%%s*, %%%s** %%%s\n",
            temp, s->type, s->type, s->name);
  } else {
    indent(ctx);
    fprintf(ctx->out, "%%%d = load i32, i32* %%%s\n", temp, s->name);
  }

  char* buf = malloc(32);
  snprintf(buf, 32, "%%%d", temp);
  return buf;
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
        s->last_branch_dynamic_type[0] = '\0';
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

  char* rhs_val = codegen_expr(ctx, rhs);

  if (lhs->type == AST_IDENTIFIER) {
    Symbol* s = lookup(ctx->sym_table, lhs->value);
    s->last_branch_dynamic_type[0] = '\0';
    if (!s) {
      fprintf(stderr, "Error: variable '%s' no declarada\n", lhs->value);
      exit(1);
    }

    // ✅ Si el RHS es un new expr, guarda dynamic_type REAL
    if (rhs->type == AST_NEW_EXPR) {
      const char* dyn_type = rhs->children[0]->value;
      strcpy(s->dynamic_type, dyn_type);
      strcpy(s->last_branch_dynamic_type, dyn_type);
      printf("🔗 %s.dynamic_type = %s\n", s->name, s->dynamic_type);

      // ⚡️ CORRECTO: extrae el ID numérico del temp generado por codegen_expr
      if (rhs_val[0] == '%') {
        s->last_temp_id = atoi(rhs_val + 1);
      }
    }

    TypeEntry* tentry = lookup_type(ctx->type_table, s->type);
    if (tentry) {
      indent(ctx);
      fprintf(ctx->out, "store %%%s* %s, %%%s** %%%s\n",
              s->type, rhs_val, s->type, s->name);

      // ⚡️ Para asignaciones que no son new expr (por ejemplo re-uso), asegura last_temp_id
      if (rhs->type != AST_NEW_EXPR) {
        s->last_temp_id = ctx->temp_count++;
        indent(ctx);
        fprintf(ctx->out, "%%%d = load %%%s*, %%%s** %%%s\n",
                s->last_temp_id, s->type, s->type, s->name);
      }

    } else {
      indent(ctx);
      fprintf(ctx->out, "store i32 %s, i32* %%%s\n", rhs_val, s->name);
    }

  } else if (lhs->type == AST_MEMBER) {
    const char* var_name = lhs->children[0]->value;
    const char* member_name = lhs->children[1]->value;

    Symbol* s = lookup(ctx->sym_table, var_name);
    s->last_branch_dynamic_type[0] = '\0';
    if (!s) {
      fprintf(stderr, "Error: variable '%s' no declarada\n", var_name);
      exit(1);
    }

    const char* struct_type = strlen(s->dynamic_type) > 0 ? s->dynamic_type : s->type;

    int index = get_member_index(ctx->type_table, struct_type, member_name);
    if (index == -1) {
      fprintf(stderr, "Error: miembro '%s' no encontrado en '%s'\n",
              member_name, struct_type);
      exit(1);
    }

    int gep = ctx->temp_count++;
    indent(ctx);
    fprintf(ctx->out,
            "%%%d = getelementptr %%%s, %%%s* %%%d, i32 0, i32 %d\n",
            gep, struct_type, struct_type, s->last_temp_id, index);

    indent(ctx);
    fprintf(ctx->out, "store i32 %s, i32* %%%d\n", rhs_val, gep);

  } else {
    fprintf(stderr, "Error: LHS de asignación no soportado.\n");
    exit(1);
  }

  free(rhs_val);
  return strdup("0");
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
int extract_vector_size(const char* vector_type) {
  int size = 0;
  sscanf(vector_type, "Vector[%d]", &size);
  return size;
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
        s->last_branch_dynamic_type[0] = '\0';
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

  ASTNode* type_spect = NULL;
  ASTNode* expr_node = NULL;

  if (node->num_children == 4) {
    type_spect = node->children[1];
    expr_node = node->children[3];
  } else if (node->num_children == 3 && node->children[1]->type == AST_ASSIGN_OP) {
    expr_node = node->children[2];
  } else {
    expr_node = node->children[1];
  }

  const char* var_type = NULL;
  if (type_spect) {
    var_type = type_spect->children[0]->value;
  } else {
    var_type = infer_type(expr_node, ctx->sym_table, ctx->type_table, NULL);
  }

  Symbol* s = create_symbol(name, var_type, SYMBOL_VARIABLE);
  s->next = ctx->sym_table->head;
  ctx->sym_table->head = s;

  char* expr = codegen_expr(ctx, expr_node);

  if (strcmp(var_type, "Number") == 0 || strcmp(var_type, "Boolean") == 0) {
    indent(ctx);
    fprintf(ctx->out, "%%%s = alloca i32\n", name);
    indent(ctx);
    fprintf(ctx->out, "store i32 %s, i32* %%%s\n", expr, name);

    int temp = ctx->temp_count++;
    indent(ctx);
    fprintf(ctx->out, "%%%d = load i32, i32* %%%s\n", temp, name);
    s->last_temp_id = temp;

  } else if (lookup_type(ctx->type_table, var_type)) {
    indent(ctx);
    fprintf(ctx->out, "%%%s = alloca %%%s*\n", name, var_type);
    indent(ctx);
    fprintf(ctx->out, "store %%%s* %s, %%%s** %%%s\n",
            var_type, expr, var_type, name);

    int temp = ctx->temp_count++;
    indent(ctx);
    fprintf(ctx->out, "%%%d = load %%%s*, %%%s** %%%s\n",
            temp, var_type, var_type, name);
    s->last_temp_id = temp;
  }

  free(expr);
  break;
}
case AST_ASSIGN: {
  ASTNode* lhs = node->children[0];
  ASTNode* rhs = node->children[1];
  char* rhs_val = codegen_expr(ctx, rhs);

  if (lhs->type == AST_IDENTIFIER) {
    Symbol* s = lookup(ctx->sym_table, lhs->value);
    s->last_branch_dynamic_type[0] = '\0';
    if (!s) {
      fprintf(stderr, "Error: variable '%s' no declarada\n", lhs->value);
      exit(1);
    }

    // ✅ Si RHS es NEW, guarda dynamic_type real para llamadas virtuales
    if (rhs->type == AST_NEW_EXPR) {
      const char* dyn_type = rhs->children[0]->value;
      strcpy(s->dynamic_type, dyn_type);
      strcpy(s->last_branch_dynamic_type, dyn_type);
      printf("🔗 %s.dynamic_type = %s\n", s->name, s->dynamic_type);

      if (rhs_val[0] == '%') {
        s->last_temp_id = atoi(rhs_val + 1);
      }
    }

    TypeEntry* tentry = lookup_type(ctx->type_table, s->type);
    if (tentry) {
      const char* tname = s->type;
      indent(ctx);
      fprintf(ctx->out, "store %%%s* %s, %%%s** %%%s\n",
              tname, rhs_val, tname, s->name);

      if (rhs->type != AST_NEW_EXPR) {
        s->last_temp_id = ctx->temp_count++;
        indent(ctx);
        fprintf(ctx->out, "%%%d = load %%%s*, %%%s** %%%s\n",
                s->last_temp_id, tname, tname, s->name);
      }
    } else {
      indent(ctx);
      fprintf(ctx->out, "store i32 %s, i32* %%%s\n", rhs_val, s->name);
    }
  }

  else if (lhs->type == AST_MEMBER) {
    const char* var_name = lhs->children[0]->value;
    const char* member_name = lhs->children[1]->value;

    Symbol* s = lookup(ctx->sym_table, var_name);
    s->last_branch_dynamic_type[0] = '\0';
    if (!s) {
      fprintf(stderr, "Error: variable '%s' no declarada\n", var_name);
      exit(1);
    }

    const char* struct_type = strlen(s->dynamic_type) > 0 ? s->dynamic_type : s->type;

    int index = get_member_index(ctx->type_table, struct_type, member_name);
    if (index == -1) {
      fprintf(stderr, "Error: miembro '%s' no encontrado en '%s'\n",
              member_name, struct_type);
      exit(1);
    }

    int gep = ctx->temp_count++;
    indent(ctx);
    fprintf(ctx->out,
            "%%%d = getelementptr %%%s, %%%s* %%%d, i32 0, i32 %d\n",
            gep, struct_type, struct_type, s->last_temp_id, index);

    indent(ctx);
    fprintf(ctx->out, "store i32 %s, i32* %%%d\n", rhs_val, gep);
  }

  else {
    fprintf(stderr, "Error: LHS de asignación no soportado.\n");
    exit(1);
  }

  free(rhs_val);
  break;
}
case AST_IF: {
  char* cond = codegen_expr(ctx, node->children[0]);
  int label_id = ctx->temp_count++;

  indent(ctx);
  fprintf(ctx->out, "%%cond%d = icmp ne i32 %s, 0\n", label_id, cond);

  indent(ctx);
  fprintf(ctx->out, "br i1 %%cond%d, label %%then%d, label %%else%d\n",
          label_id, label_id, label_id);

  // THEN
  indent(ctx);
  fprintf(ctx->out, "then%d:\n", label_id);
  ctx->indent++;
  codegen_stmt(ctx, node->children[1]);

  Symbol* sym = ctx->sym_table->head;
  while (sym) {
    if (lookup_type(ctx->type_table, sym->type)) {
      strcpy(sym->last_branch_dynamic_type, strlen(sym->dynamic_type) > 0 ? sym->dynamic_type : sym->type);
    }
    sym = sym->next;
  }

  indent(ctx);
  fprintf(ctx->out, "br label %%endif%d\n", label_id);
  ctx->indent--;

  // ELSE
  indent(ctx);
  fprintf(ctx->out, "else%d:\n", label_id);
  ctx->indent++;
  codegen_stmt(ctx, node->children[2]);

  sym = ctx->sym_table->head;
  while (sym) {
    if (lookup_type(ctx->type_table, sym->type)) {
      strcpy(sym->last_branch_dynamic_type_else, strlen(sym->dynamic_type) > 0 ? sym->dynamic_type : sym->type);
    }
    sym = sym->next;
  }

  indent(ctx);
  fprintf(ctx->out, "br label %%endif%d\n", label_id);
  ctx->indent--;

  // ENDIF
  indent(ctx);
  fprintf(ctx->out, "endif%d:\n", label_id);

  sym = ctx->sym_table->head;
  while (sym) {
    if (lookup_type(ctx->type_table, sym->type)) {
      sym->last_temp_id = ctx->temp_count++;
      indent(ctx);
      fprintf(ctx->out, "%%%d = load %%%s*, %%%s** %%%s\n",
              sym->last_temp_id, sym->type, sym->type, sym->name);

      const char* merged = find_common_base(ctx->type_table,
                                            sym->last_branch_dynamic_type,
                                            sym->last_branch_dynamic_type_else);
      strcpy(sym->dynamic_type, merged);

      printf("🔄 Recarga después de IF: %s => last_temp_id=%d, dynamic_type=%s\n",
             sym->name, sym->last_temp_id, sym->dynamic_type);
    }
    sym = sym->next;
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
                              child->type == AST_IDENTIFIER );

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
  for (TypeEntry* t = type_table->head; t; t = t->next) {
    Member* m = t->members;
    while (m) {
      if (m->body != NULL) {
        // Define solo si implementado localmente
        indent(ctx);
        fprintf(ctx->out, "define i32 @%s_%s(%%%s* %%this) {\n", t->name, m->name, t->name);
        ctx->indent++;

        SymbolTable local_scope = *ctx->sym_table;
        Symbol* this_sym = create_symbol("this", t->name, SYMBOL_VARIABLE);
        strcpy(this_sym->dynamic_type, t->name);
        this_sym->last_temp_id = -1;
        this_sym->next = local_scope.head;
        local_scope.head = this_sym;

        SymbolTable* prev_scope = ctx->sym_table;
        ctx->sym_table = &local_scope;

        if (m->body->type == AST_STATEMENT_LIST) {
          codegen_stmt(ctx, m->body);
        } else {
          char* result = codegen_expr(ctx, m->body);
          indent(ctx);
          fprintf(ctx->out, "ret i32 %s\n", result);
          free(result);
        }

        ctx->sym_table = prev_scope;
        ctx->indent--;
        indent(ctx);
        fprintf(ctx->out, "}\n\n");
      }
      m = m->next;
    }
  }
}

void collect_stmts_and_funcs(ASTNode* node, FuncBuffer* buf, ASTNode** main_stmts, TypeTable* type_table) {
  if (!node) return;

  if (node->type == AST_FUNCTION_DECL) {
    const char* func_name = node->children[0]->value;

    // ⚡️ Filtra: Si el nombre está registrado como método en la tabla de tipos, NO lo agregues.
    int is_method = 0;
    for (TypeEntry* t = type_table->head; t; t = t->next) {
      for (int i = 0; i < t->method_count; ++i) {
        if (strcmp(func_name, t->method_names[i]) == 0) {
          is_method = 1;
          break;
        }
      }
      if (is_method) break;
    }

    if (!is_method) {
      // Solo funciones libres van al buffer
      if (buf->count >= buf->capacity) {
        buf->capacity *= 2;
        buf->funcs = realloc(buf->funcs, sizeof(ASTNode*) * buf->capacity);
      }
      buf->funcs[buf->count++] = node;
    }
  }

  else if (node->type == AST_TYPE_DECL) {
    // ⚡️ Los métodos se procesan vía `codegen_type_methods`. No necesitas hacer nada aquí.
  }

  else if (node->type == AST_STATEMENT_LIST || node->type == AST_PROGRAM) {
    for (int i = 0; i < node->num_children; ++i) {
      collect_stmts_and_funcs(node->children[i], buf, main_stmts, type_table);
    }
  }

  else {
    // Otros statements: guárdalos como parte de main
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

  fprintf(ctx->out, "declare i32 @printf(i8*, ...)\n");
  fprintf(ctx->out, "@print.str = constant [4 x i8] c\"%%d\\0A\\00\"\n\n");

  resolve_virtual_methods(ctx->type_table);
  build_vtable_info(ctx->type_table); 
  debug_print_types(ctx->type_table); 
  emit_structs(ctx, ctx->type_table);
  emit_vtables(ctx, ctx->type_table);

  codegen_type_methods(ctx, ctx->type_table);

  register_all_function_returns(ctx, root);

  FuncBuffer buf;
  buf.funcs = malloc(sizeof(ASTNode*) * 4);
  buf.count = 0;
  buf.capacity = 4;

  ASTNode* main_stmts = NULL;
  collect_stmts_and_funcs(root, &buf, &main_stmts, ctx->type_table);


  for (int i = 0; i < buf.count; ++i) {
    codegen_function_decl(ctx, buf.funcs[i]);
  }

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

  for (int i = 0; i < params->num_children; ++i) {
    ASTNode* param = params->children[i];
    const char* param_name = param->value;

    const char* param_type = "i32";
    for (int j = 0; j < param->num_children; ++j) {
      if (param->children[j]->type == AST_TYPE_SPEC) {
        param_type = param->children[j]->children[0]->value;
      }
    }

    Symbol* s = create_symbol(param_name, param_type, SYMBOL_VARIABLE);
    s->last_temp_id = -1;
    s->next = ctx->sym_table->head;
    ctx->sym_table->head = s;
  }

  fprintf(ctx->out, "define %s @%s(", ret_type, func_name);
  for (int i = 0; i < params->num_children; ++i) {
    ASTNode* param = params->children[i];
    const char* param_type = "i32";
    for (int j = 0; j < param->num_children; ++j) {
      if (param->children[j]->type == AST_TYPE_SPEC) {
        param_type = param->children[j]->children[0]->value;
      }
    }

    if (lookup_type(ctx->type_table, param_type)) {
      fprintf(ctx->out, "%%%s* %%%s", param_type, param->value); 
    } else {
      fprintf(ctx->out, "i32");
    }

    if (i < params->num_children - 1) fprintf(ctx->out, ", ");
  }
  fprintf(ctx->out, ") {\n");

  ctx->indent++;
  ctx->temp_count = params->num_children + 1;

  for (int i = 0; i < params->num_children; ++i) {
    ASTNode* param = params->children[i];
    const char* param_name = param->value;

    const char* param_type = "i32";
    for (int j = 0; j < param->num_children; ++j) {
      if (param->children[j]->type == AST_TYPE_SPEC) {
        param_type = param->children[j]->children[0]->value;
      }
    }

    if (lookup_type(ctx->type_table, param_type)) {
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
      if (s_param) s_param->last_temp_id = temp_id;

    } else {
      indent(ctx);
      fprintf(ctx->out, "%%%s = alloca i32\n", param_name);
      indent(ctx);
      fprintf(ctx->out, "store i32 %%%d, i32* %%%s\n", i, param_name);

      int temp = ctx->temp_count++;
      indent(ctx);
      fprintf(ctx->out, "%%%d = load i32, i32* %%%s\n", temp, param_name);

      Symbol* s = lookup(ctx->sym_table, param_name);
      if (s) s->last_temp_id = temp;
    }
  }

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
  for (TypeEntry* t = table->head; t; t = t->next) {
    if (t->method_count == 0) continue; // 🟢 Evita generar una tabla vacía

    // ⚡ Usa la firma de slot 0 como tipo base homogéneo
    fprintf(ctx->out, "@%s_vtable = global [%d x %s] [\n",
            t->name, t->method_count, t->method_signatures[0]);

    for (int i = 0; i < t->method_count; ++i) {
      fprintf(ctx->out, "  %s @%s_%s",
              t->method_signatures[i],
              t->method_impls[i],
              t->method_names[i]);

      if (i < t->method_count - 1) fprintf(ctx->out, ",\n");
      else fprintf(ctx->out, "\n");
    }

    fprintf(ctx->out, "]\n\n");
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
void resolve_virtual_methods(TypeTable* table) {
  for (TypeEntry* t = table->head; t; t = t->next) {
    for (int i = 0; i < t->num_bases; ++i) {
      TypeEntry* base = lookup_type(table, t->bases[i]);
      if (!base) continue;

      for (int j = 0; j < base->method_count; ++j) {
        const char* base_method = base->method_names[j];

        // Busca si la clase actual tiene un método con ese nombre
        int found = -1;
        for (int k = 0; k < t->method_count; ++k) {
          if (strcmp(t->method_names[k], base_method) == 0) {
            found = k;
            break;
          }
        }

        if (found != -1) {
          // ⚡️ OVERRIDE: usa mismo slot del padre
          if (found != j) {
            printf("⚡ Corrigiendo override: %s::%s => slot %d\n", t->name, base_method, j);
            strcpy(t->method_names[j], base_method);
            strcpy(t->method_signatures[j], base->method_signatures[j]);
          }
          // Siempre apunta a la implementación de la clase derivada
          strcpy(t->method_impls[j], t->name);

          // Ajusta tamaño si es necesario
          if (j >= t->method_count) t->method_count = j + 1;

          printf("✔️ %s overridea %s::%s en slot %d\n", t->name, base->name, base_method, j);
        } else {
          // ⚡️ HEREDA sin override
          strcpy(t->method_names[j], base_method);
          strcpy(t->method_signatures[j], base->method_signatures[j]);
          strcpy(t->method_impls[j], base->method_impls[j]);
          if (j >= t->method_count) t->method_count = j + 1;

          printf("✔️ %s hereda %s::%s en slot %d\n", t->name, base->name, base_method, j);
        }
      }
    }
  }
}
