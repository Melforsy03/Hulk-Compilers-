#include "semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "codegen.h"
// ----------------- Tabla Virtual -------------------

TypeEntry* lookup_class(TypeTable* table, const char* name) {
    return lookup_type(table, name);
}

void build_vtable_info(TypeTable* table) {
  TypeEntry* t = table->head;
  while (t) {
    int count = 0;

    // ⚡ Métodos propios: definen slots nuevos o sobrescriben
    Member* m = t->members;
    while (m) {
      if (m->body) {
        strcpy(t->method_names[count], m->name);

        // Por convención: siempre "i8* (%%Type*)*" para puntero en vtable
        emit_method_signature(t->method_signatures[count], 64, m->type, t->name);

        // Implementación local: apunta a Type actual
        strcpy(t->method_impls[count], t->name);
        count++;
      }
      m = m->next;
    }

    // ⚡ Métodos heredados: rellena slots faltantes
    for (int i = 0; i < t->num_bases; ++i) {
      TypeEntry* base = lookup_type(table, t->bases[i]);
      if (base) {
        for (int j = 0; j < base->method_count; ++j) {
          int exists = 0;
          for (int k = 0; k < count; ++k) {
            if (strcmp(t->method_names[k], base->method_names[j]) == 0) {
              exists = 1; // Ya sobrescrito
              break;
            }
          }
          if (!exists) {
            strcpy(t->method_names[count], base->method_names[j]);
            strcpy(t->method_signatures[count], base->method_signatures[j]);

            // ⚡ Usa impl de la base porque no se sobrescribe
            strcpy(t->method_impls[count], base->method_impls[j]);
            count++;
          }
        }
      }
    }

    t->method_count = count;
    t = t->next;
  }
}

int get_method_index(TypeTable* table, const char* type_name, const char* method_name) {
  TypeEntry* t = lookup_type(table, type_name);
  if (!t) return -1;
  for (int i = 0; i < t->method_count; i++) {
    if (strcmp(t->method_names[i], method_name) == 0) {
      return i;  // Slot correcto
    }
  }
  return -1;
}

// -------------------- Tabla de Símbolos --------------------
void init_symbol_table(SymbolTable* table) { table->head = NULL; }

Symbol* lookup(SymbolTable* table, const char* name) {
    printf("🔍 lookup: %s\n", name);
    Symbol* current = table->head;
    while (current) {
        printf("   ↳ symbol: %s (%s)\n", current->name, current->type);
        if (strcmp(current->name, name) == 0) {
            printf("✅ Encontrado: %s (%s)\n", current->name, current->type);
            return current;
        }
        current = current->next;
    }
    printf("❌ No encontrado: %s\n", name);
    return NULL;
}


void insert_symbol(SymbolTable* table, const char* name, const char* type, SymbolKind kind) {
    Symbol* s = create_symbol(name, type, kind);
    s->next = table->head;
    table->head = s;
}

// -------------------- Tabla de Tipos --------------------
void init_type_table(TypeTable* table) {
    table->head = NULL;
    // Insertar tipos básicos
  
}
void add_error(ErrorList* list, int line, int column, const char* fmt, ...) {
    if (list->count >= MAX_ERRORS) return;

    va_list args;
    va_start(args, fmt);

    char* msg = malloc(256);
    vsnprintf(msg, 256, fmt, args);
    va_end(args);

    list->errors[list->count].message = msg;
    list->errors[list->count].line = line;
    list->errors[list->count].column = column;
    list->count++;
}
TypeEntry* lookup_type(TypeTable* table, const char* name) {
    TypeEntry* current = table->head;
    while (current) {
        if (strcmp(current->name, name) == 0) return current;
        current = current->next;
    }
    return NULL;
}
void check_all_type_methods(SymbolTable* global_sym, TypeTable* type_table, ErrorList* list) {
  TypeEntry* t = type_table->head;
  while (t) {
    Member* m = t->members;
    while (m) {
      if (m->body != NULL) {
        printf("Chequeando método %s::%s\n", t->name, m->name);

        ASTNode* func_decl = m->body; // 🚩 Nodo FUNCTION_DECL entero

        SymbolTable local_scope = *global_sym;
        insert_symbol(&local_scope, "this", t->name, SYMBOL_VARIABLE);

        // ✅ Busca ParamList y cuerpo real
        ASTNode* param_list = NULL;
        ASTNode* real_body = NULL;

        for (int i = 0; i < func_decl->num_children; ++i) {
          if (func_decl->children[i]->type == AST_PARAM_LIST) {
            param_list = func_decl->children[i];
          }
          if (func_decl->children[i]->type == AST_STATEMENT_LIST) {
            real_body = func_decl->children[i];
          }
        }

        if (param_list) {
          for (int j = 0; j < param_list->num_children; ++j) {
            ASTNode* param = param_list->children[j];
            if (param->type == AST_VAR_DECL && param->num_children >= 2) {
              const char* param_name = param->children[0]->value;
              const char* param_type = param->children[1]->value;
              printf("✅ Insertando parámetro: %s : %s\n", param_name, param_type);
              insert_symbol(&local_scope, param_name, param_type, SYMBOL_VARIABLE);
            }
          }
        }

        check_semantics(real_body, &local_scope, type_table, t->name, list);
      }
      m = m->next;
    }
    t = t->next;
  }
}
void insert_type(
    TypeTable* table,
    const char* name,
    const char** bases,
    int num_bases,
    ASTNode* bases_node,  
    ASTNode* members_node, 
    ErrorList* error_list
) {
    for (int i = 0; i < num_bases; ++i) {
        if (has_circular_dependency(table, name, bases[i])) {
            // Busca el nodo IDENTIFIER de la base conflictiva
            int line = -1, column = -1;
            if (bases_node && i < bases_node->num_children) {
                ASTNode* base_id = bases_node->children[i];
                if (base_id) {
                    line = base_id->line;
                    column = base_id->column;
                }
            }
            add_error(
                error_list,
                line, column,
                "Error: herencia circular detectada en '%s' con base '%s'",
                name, bases[i]
            );
            return;
        }
    }

    TypeEntry* t = malloc(sizeof(TypeEntry));
    strcpy(t->name, name);
    t->num_bases = num_bases > 0 ? num_bases : 1;

    if (num_bases == 0) {
        strcpy(t->bases[0], "Object");
    } else {
        for (int i = 0; i < num_bases; ++i) {
            strcpy(t->bases[i], bases[i]);
        }
    }

    t->members = NULL;
    t->next = table->head;
    table->head = t;

    if (members_node) {
        parse_type_members(members_node, name, table);
    }
}


void add_member_to_type(TypeTable* table, const char* type_name, 
                        const char* member_name, const char* member_type, 
                        ASTNode* body, int default_value) {
  TypeEntry* t = lookup_type(table, type_name);
  if (!t) return;

  // Evita duplicados
  Member* m = t->members;
  while (m) {
    if (strcmp(m->name, member_name) == 0) return;
    m = m->next;
  }

  Member* new_member = malloc(sizeof(Member));
  strcpy(new_member->name, member_name);
  strcpy(new_member->type, member_type);
  new_member->body = body;  
  new_member->default_value = default_value;
  new_member->next = t->members;
  t->members = new_member;

  // Si es método, añade slot VTABLE
  if (body != NULL) {
    int found = -1;
    for (int i = 0; i < t->method_count; ++i) {
      if (strcmp(t->method_names[i], member_name) == 0) {
        found = i;
        break;
      }
    }
    if (found == -1) {
      int slot = t->method_count++;
      strcpy(t->method_names[slot], member_name);
      sprintf(t->method_signatures[slot], "i8* (%%%s*)*", t->name);
      strcpy(t->method_impls[slot], t->name);
    }
  }
}

const char* lookup_member_type(TypeTable* table, const char* type_name, const char* member_name) {
    // Manejar tipos genéricos (ej: Box(Number) -> buscar Box)
    char base_type[32];
    strncpy(base_type, type_name, 31);
    base_type[31] = '\0';
    
    char* open_paren = strchr(base_type, '(');
    if (open_paren) *open_paren = '\0';

    TypeEntry* t = lookup_type(table, base_type);
    if (!t) return "undefined";

    // Buscar en miembros directos
    Member* m = t->members;
    while (m) {
        if (strcmp(m->name, member_name) == 0) return m->type;
        m = m->next;
    }

    // Buscar en tipos base
    for (int i = 0; i < t->num_bases; ++i) {
        const char* found = lookup_member_type(table, t->bases[i], member_name);
        if (strcmp(found, "undefined") != 0) return found;
    }

    return "undefined";
}
void serialize_type(ASTNode* type_node, char* buffer, size_t size) {
  if (!type_node) return;

  if (type_node->num_children >= 1 && type_node->children[0]->type == AST_IDENTIFIER) {
    strncat(buffer, type_node->children[0]->value, size - strlen(buffer) - 1);

    // Solo agrega paréntesis si hay argumentos reales
    if (type_node->num_children >= 2 && type_node->children[1]->type == AST_ARGUMENT_LIST &&
        type_node->children[1]->num_children > 0) {

      strncat(buffer, "(", size - strlen(buffer) - 1);

      for (int i = 0; i < type_node->children[1]->num_children; ++i) {
        if (i > 0) strncat(buffer, ",", size - strlen(buffer) - 1);
        serialize_type(type_node->children[1]->children[i], buffer, size);
      }

      strncat(buffer, ")", size - strlen(buffer) - 1);
    }
  }
}

int conforms(TypeTable* table, const char* t1, const char* t2) {
    if ((strcmp(t1, "Number") == 0 && strcmp(t2, "i32") == 0) ||
    (strcmp(t1, "i32") == 0 && strcmp(t2, "Number") == 0)) {
    return 1; // ✅ Son equivalentes
}
    // Manejar tipos genéricos
    char base_t1[32], base_t2[32];
    strncpy(base_t1, t1, 31);
    base_t1[31] = '\0';
    strncpy(base_t2, t2, 31);
    base_t2[31] = '\0';
    
    char* open_paren = strchr(base_t1, '(');
    if (open_paren) *open_paren = '\0';
    open_paren = strchr(base_t2, '(');
    if (open_paren) *open_paren = '\0';

    if (strcmp(base_t1, base_t2) == 0) return 1;
    if (strcmp(base_t2, "Object") == 0) return 1;

    TypeEntry* child = lookup_type(table, base_t1);
    if (!child) return 0;

    // Prevenir recursión infinita
    static int depth = 0;
    if (depth > 16) return 0;
    depth++;

    for (int i = 0; i < child->num_bases; ++i) {
        if (conforms(table, child->bases[i], base_t2)) {
            depth--;
            return 1;
        }
    }

    depth--;
    return 0;
}
// -------------------- Inferencia --------------------
const char* infer_type(ASTNode* node, SymbolTable* sym_table, TypeTable* type_table, ErrorList* error_list) {
    if (!node) return "unknown";

    switch (node->type) {
        case AST_NUMBER:
            return "Number";
        case AST_STRING:
            return "String";
        case AST_BOOL:
            return "Boolean";

      case AST_IDENTIFIER: {
            Symbol* s = lookup(sym_table, node->value);
            if (s) return s->type;

            Symbol* this_symbol = lookup(sym_table, "this");
            if (this_symbol) {
                const char* in_this = lookup_member_type(type_table, this_symbol->type, node->value);
                if (strcmp(in_this, "undefined") != 0) return in_this;
            }

            add_error(error_list, node->line, node->column,
                    "Error: símbolo '%s' no declarado", node->value);
            return "error";  
        }


    case AST_BINOP: {
    if (node->num_children < 2) {
        add_error(error_list,node->line , node->column , "Error: BinOp incompleto");
        return "error";
    }

    const char* left = infer_type(node->children[0], sym_table, type_table, error_list);
    const char* right = infer_type(node->children[1], sym_table, type_table, error_list);
    const char* op = node->value;

    // --- Operaciones aritméticas ---
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
        strcmp(op, "*") == 0 || strcmp(op, "/") == 0) {

        if (strcmp(left, "Number") == 0 && strcmp(right, "Number") == 0) {
            return "Number";
        } else {
            add_error(error_list,node->line , node->column , "Error: operación aritmética requiere Number, pero recibió (%s vs %s)", left, right);
            return "error";
        }
    }

    // --- Comparaciones relacionales (<, >, <=, >=) ---
    if (strcmp(op, "<") == 0 || strcmp(op, ">") == 0 ||
        strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0) {

        if (strcmp(left, "Number") == 0 && strcmp(right, "Number") == 0) {
            return "Boolean";
        } else {
            add_error(error_list, node->line , node->column ,"Error: comparación relacional requiere Number, pero recibió (%s vs %s)", left, right);
            return "error";
        }
    }

    // --- Comparadores de igualdad (==, !=) ---
    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) {
        if (strcmp(left, right) == 0) {
            return "Boolean";
        } else {
            add_error(error_list,node ->line , node->column , "Error: comparación de igualdad entre tipos distintos (%s vs %s)", left, right);
            return "error";
        }
    }

    // --- Operadores lógicos (&&, or) ---
    if (strcmp(op, "&&") == 0 || strcmp(op, "or") == 0) {
        if (strcmp(left, "Boolean") == 0 && strcmp(right, "Boolean") == 0) {
            return "Boolean";
        } else {
            add_error(error_list, node ->line , node->column ,"Error: operador lógico requiere Boolean, pero recibió (%s vs %s)", left, right);
            return "error";
        }
    }

    // --- Operador desconocido ---
    add_error(error_list,node->line , node->column , "Error: operador binario '%s' no soportado", op);
    return "error";
}

     case AST_TYPE_SPEC:
    // No se infiere tipo para anotaciones de tipo
         return "Number";

        case AST_UNARYOP:
            if (node->num_children == 1)
                return infer_type(node->children[0], sym_table, type_table, error_list);
            return "unknown";

        case AST_IF: {
            if (node->num_children < 3) {
                add_error(error_list, node->line , node->column ,"Error: If incompleto");
                return "Object";
            }
            const char* cond_type = infer_type(node->children[0], sym_table, type_table, error_list);
            if (!conforms(type_table, cond_type, "Boolean")){
                add_error(error_list, node->line , node->column ,"Error: condición If no es Boolean, es '%s'", cond_type);
            }

            const char* t1 = infer_type(node->children[1], sym_table, type_table, error_list);
            const char* t2 = infer_type(node->children[2], sym_table, type_table, error_list);

            if (conforms(type_table, t1, t2)) return t2;
            if (conforms(type_table, t2, t1)) return t1;
            if (strcmp(t1, t2) == 0) return t1;
            return "Object";
        }

        case AST_STATEMENT_LIST:
            if (node->num_children == 0) return "Object";
            return infer_type(node->children[node->num_children - 1], sym_table, type_table, error_list);

        case AST_RETURN:
            if (node->num_children < 1) return "Object";
            return infer_type(node->children[0], sym_table, type_table, error_list);

        case AST_VECTOR: {
            if (node->num_children == 0) return "Vector[0]";
            const char* first = infer_type(node->children[0], sym_table, type_table, error_list);
            for (int i = 1; i < node->num_children; ++i) {
                const char* elem_type = infer_type(node->children[i], sym_table, type_table, error_list);
                if (!conforms(type_table, elem_type, first)) {
                    add_error(error_list, node->line , node->column ,"Error: Elemento %d incompatible (%s vs %s)", i, elem_type, first);
                }
            }
            static char buffer[64];
            snprintf(buffer, sizeof(buffer), "Vector[%d]", node->num_children);
            return buffer;
        }

  case AST_FUNCTION_CALL: {
    if (node->num_children < 1 || !node->children[0]) {
        add_error(error_list, node->line, node->column, "Error: FunctionCall incompleto");
        return "Object";
    }

    ASTNode* target = node->children[0];

    if (target->type == AST_MEMBER) {
        const char* obj_type = infer_type(target->children[0], sym_table, type_table, error_list);
        const char* method_name = target->children[1]->value;
        const char* method_type = lookup_member_type(type_table, obj_type, method_name);

        if (strcmp(method_type, "undefined") == 0) {
            add_error(error_list, node->line, node->column,
                      "Error: método '%s' no existe en tipo '%s'",
                      method_name, obj_type);
            return "error";
        }

        return method_type;
    }

 
    if (target->type != AST_IDENTIFIER) {
        add_error(error_list, node->line, node->column, "Error: FunctionCall mal formado");
        return "Object";
    }

    const char* func_name = target->value;
    Symbol* s = lookup(sym_table, func_name);
    if (!s) {
        add_error(error_list, node->line, node->column, "Error: función '%s' no declarada", func_name);
        return "error";
    }

    if (s->func_decl_node) {
        ASTNode* decl = s->func_decl_node;
        ASTNode* param_list = NULL;
        for (int i = 1; i < decl->num_children; ++i) {
            if (decl->children[i]->type == AST_PARAM_LIST) {
                param_list = decl->children[i];
                break;
            }
        }

        ASTNode* arg_list = (node->num_children > 1) ? node->children[1] : NULL;
        int expected = param_list ? param_list->num_children : 0;
        int actual = arg_list ? arg_list->num_children : 0;

        if (expected != actual) {
            add_error(error_list, node->line, node->column,
              "Error: número de argumentos (%d) no coincide con parámetros esperados (%d)",
              actual, expected);
        }

        int n = expected < actual ? expected : actual;
        for (int i = 0; i < n; ++i) {
            const char* param_type = "Object";
            if (param_list->children[i]->type == AST_VAR_DECL &&
                param_list->children[i]->num_children >= 2) {
                ASTNode* type_spec = param_list->children[i]->children[1];
                if (type_spec->type == AST_TYPE_SPEC && type_spec->num_children > 0) {
                    param_type = type_spec->children[0]->value;
                }
            }

            const char* arg_type = infer_type(arg_list->children[i], sym_table, type_table, error_list);

            if (!conforms(type_table, arg_type, param_type)) {
                add_error(error_list, node->line, node->column,
                  "Error: tipo argumento '%s' no se ajusta a parámetro '%s'",
                  arg_type, param_type);
            }
        }
    }

    return s->type;
}

        case AST_MEMBER: {
    const char* base_type = infer_type(node->children[0], sym_table, type_table, error_list);
    const char* member_name = node->children[1]->value;

    const char* member_type = lookup_member_type(type_table, base_type, member_name);

    if (strcmp(member_type, "undefined") == 0) {
        add_error(error_list, node->line, node->column,
                  "Error: miembro '%s' no existe en tipo '%s'",
                  member_name, base_type);
        return "error";   // ⚡️ CLAVE: devuelve 'error'
    }

    return member_type;
}



        case AST_NEW_EXPR:
            if (node->num_children > 0 && node->children[0]->type == AST_IDENTIFIER) {
                const char* class_name = node->children[0]->value;
                if (!lookup_type(type_table, class_name)) {
                    add_error(error_list,node->line , node->column , "Error: tipo '%s' no declarado", class_name);
                    return "undefined";
                }
                return class_name;
            }
            add_error(error_list,node->line , node->column , "Error: constructor 'new' mal formado");
            return "undefined";

        default:
            return "Boolean";
    }
}
// -------------------- Chequeo de dependencia circular --------------------
int has_circular_dependency(TypeTable* table, const char* type_name, const char* current) {
    if (strcmp(type_name, current) == 0) return 1;

    TypeEntry* t = lookup_type(table, current);
    if (!t) return 0;

    for (int i = 0; i < t->num_bases; ++i) {
        if (has_circular_dependency(table, type_name, t->bases[i])) return 1;
    }
    return 0;
}

void check_semantics(ASTNode* node, SymbolTable* sym_table, TypeTable* type_table, const char* expected_return, ErrorList* error_list) {
    if (!node) return;

    switch (node->type) {
       
        case AST_VECTOR :
        break;
        case AST_TYPE_SPEC:
            break;
        case AST_PROGRAM: {

            for (int i = 0; i < node->num_children; ++i) {
                if (node->children[i]->type == AST_TYPE_DECL || node->children[i]->type == AST_FUNCTION_DECL) {
                    check_semantics(node->children[i], sym_table, type_table, expected_return, error_list);
                }
            }

            check_all_type_methods(sym_table, type_table, error_list);

            // --- VTABLE SUPPORT: generar info de vtables justo después de registrar todos los tipos
            build_vtable_info(type_table);
            print_vtable_info(type_table);

            for (int i = 0; i < node->num_children; ++i) {
                if (node->children[i]->type != AST_TYPE_DECL && node->children[i]->type != AST_FUNCTION_DECL) {
                    check_semantics(node->children[i], sym_table, type_table, expected_return, error_list);
                }
            }
            break;
        }
 case AST_LET: {
    if (node->num_children < 2) {
        add_error(error_list,node->line , node->column , "Error: Let incompleto");
        return;
    }

    // 1️⃣ Crea un scope anidado
    SymbolTable inner_scope = *sym_table;

    // 2️⃣ Inserta variables en el inner_scope
    ASTNode* decl_list = node->children[0];
    for (int i = 0; i < decl_list->num_children; ++i) {
        ASTNode* decl = decl_list->children[i];
        if (decl->type == AST_VAR_DECL) {
            const char* var_name = decl->children[0]->value;
            const char* var_type = "Object";

            if (decl->num_children > 2) {
                ASTNode* init_expr = decl->children[decl->num_children - 1];
                var_type = infer_type(init_expr, &inner_scope, type_table, error_list);
            }

            insert_symbol(&inner_scope, var_name, var_type, SYMBOL_VARIABLE);
        }

        check_semantics(decl, &inner_scope, type_table, expected_return, error_list);
    }

    // 3️⃣ Cuerpo del let se chequea con inner_scope
    check_semantics(node->children[1], &inner_scope, type_table, expected_return, error_list);
    break;
}

 case AST_VAR_DECL: {
    const char* name = NULL;
    const char* declared_type = NULL;
    ASTNode* init_expr = NULL;

    // 🔍 Buscar nombre, tipo y RHS
    for (int i = 0; i < node->num_children; ++i) {
        ASTNode* child = node->children[i];

        if (child->type == AST_IDENTIFIER && name == NULL) {
            name = child->value;
        }
        else if (child->type == AST_TYPE_SPEC) {
            if (child->num_children > 0 && child->children[0]->type == AST_IDENTIFIER) {
                declared_type = child->children[0]->value;
            }
        }
        else if (child->type == AST_ASSIGN || child->type == AST_ASSIGN_OP) {
            if (child->num_children > 1) {
                init_expr = child->children[1];
            }
        }
        else if (
            child->type == AST_FUNCTION_CALL || child->type == AST_BINOP ||
            child->type == AST_NUMBER || child->type == AST_STRING ||
            child->type == AST_VECTOR || child->type == AST_NEW_EXPR ||
            child->type == AST_BOOL || child->type == AST_IDENTIFIER ||
            child->type == AST_MEMBER  // ⚡️ ¡Claves como D.age!
        ) {
            init_expr = child;
        }
    }

    if (!name) {
        add_error(error_list, node->line, node->column,
                  "Error: variable sin nombre en declaración");
        break;
    }

    if (!init_expr) {
        if (!declared_type) {
            add_error(error_list, node->line, node->column,
                      "Error: declaración incompleta de '%s' sin tipo ni valor", name);
            break;
        }
        insert_symbol(sym_table, name, declared_type, SYMBOL_VARIABLE);
        break;
    }

    const char* expr_type = infer_type(init_expr, sym_table, type_table, error_list);

    if (strcmp(expr_type, "error") == 0) {
        // ⚡️ Por ejemplo: D.age no existe → infer_type ya reportó el error
        break; // No dispares otro error genérico
    }
    if (strcmp(expr_type, "unknown") == 0 || strcmp(expr_type, "undefined") == 0) {
        // ⚡️ Caso var_interna no existe → ya se reportó con lookup
        break;
    }

    if (declared_type) {
        if (!conforms(type_table, expr_type, declared_type)) {
            add_error(error_list, node->line, node->column,
                      "Error: tipo '%s' no se ajusta a '%s' en la variable '%s'",
                      expr_type, declared_type, name);
        }
    }

    insert_symbol(sym_table, name, declared_type ? declared_type : expr_type, SYMBOL_VARIABLE);
    break;
}

        case AST_ASSIGN: {
            if (node->num_children < 2) {
                add_error(error_list,node->line , node->column , "Error: Assign incompleto");
                return;
            }

            ASTNode* lhs = node->children[0];
            const char* expr_type = infer_type(node->children[1], sym_table, type_table, error_list);

            if (lhs->type == AST_IDENTIFIER) {
                const char* name = lhs->value;
                if (!name || strlen(name) == 0) {
                    add_error(error_list,node->line , node->column , "Error: nombre de variable vacío en Assign");
                    break;
                }
                Symbol* s = lookup(sym_table, name);
                if (!s) {
                    add_error(error_list,node->line , node->column , "Error: variable '%s' no declarada", name);
                    break;
                }
                if (!conforms(type_table, expr_type, s->type)) {
                    add_error(error_list,node ->line , node ->column ,  "Error: tipo '%s' no se ajusta a '%s'", expr_type, s->type);
                }
            } else if (lhs->type == AST_MEMBER) {
                const char* base_type = infer_type(lhs->children[0], sym_table, type_table, error_list);
                const char* member_name = lhs->children[1]->value;
                const char* member_type = lookup_member_type(type_table, base_type, member_name);
                if (strcmp(member_type, "undefined") == 0) {
                    add_error(error_list,node->line , node->column , "Error: miembro '%s' no existe en '%s'", member_name, base_type);
                } else if (!conforms(type_table, expr_type, member_type)) {
                    add_error(error_list, node ->line , node->column ,"Error: tipo '%s' no se ajusta a '%s'", expr_type, member_type);
                }
            }
            break;
        }
case AST_PARAM_LIST: {
  for (int i = 0; i < node->num_children; ++i) {
    check_semantics(node->children[i], sym_table, type_table, expected_return, error_list);
  }
  break;
}

case AST_FUNCTION_DECL: {
    const char* func_name = node->children[0]->value;
    const char* declared_return = "Object";
    ASTNode* body = NULL;
    ASTNode* param_list = NULL;

    // 1️⃣ Buscar nodos importantes
    for (int i = 1; i < node->num_children; ++i) {
        if (node->children[i]->type == AST_PARAM_LIST) {
            param_list = node->children[i];
        } else if (node->children[i]->type == AST_TYPE_SPEC && node->children[i]->num_children > 0) {
            declared_return = node->children[i]->children[0]->value;
        } else if (node->children[i]->type == AST_STATEMENT_LIST) {
            body = node->children[i];
        }
    }

    // 2️⃣ Crear scope local
    SymbolTable local_scope;
    init_symbol_table(&local_scope);
    Symbol* current = sym_table->head;
    while (current) {
        insert_symbol(&local_scope, current->name, current->type, current->kind);
        current = current->next;
    }

    // 3️⃣ Insertar parámetros
    if (param_list) {
        for (int i = 0; i < param_list->num_children; ++i) {
            ASTNode* param = param_list->children[i];
            if (param->type == AST_VAR_DECL && param->num_children >= 2) {
                const char* param_name = param->children[0]->value;
                const char* param_type = "Object";
                ASTNode* type_spec = param->children[1];
                if (type_spec->type == AST_TYPE_SPEC && type_spec->num_children > 0) {
                    param_type = type_spec->children[0]->value;
                }
                insert_symbol(&local_scope, param_name, param_type, SYMBOL_VARIABLE);
            }
        }
    }

    // 4️⃣ Chequear cuerpo y tipo de retorno
    if (body) {
        check_semantics(body, &local_scope, type_table, declared_return, error_list);
        const char* actual_return = infer_type(body, &local_scope, type_table, error_list);
        if (!conforms(type_table, actual_return, declared_return)) {
            add_error(error_list, node->line, node->column,
                "Error: tipo de retorno '%s' no se ajusta a '%s'",
                actual_return, declared_return);
        }
    }

    // ✅ Guarda nodo en símbolo
    Symbol* func_sym = create_symbol(func_name, declared_return, SYMBOL_FUNCTION);
    func_sym->func_decl_node = node;
    func_sym->next = sym_table->head;
    sym_table->head = func_sym;

    break;
}

case AST_TYPE_DECL: {
    if (node->num_children < 2) {
        add_error(error_list, node->line, node->column, "Error: declaración de tipo incompleta");
        break;
    }

    const char* type_name = NULL;
    const char* bases[8] = {0};
    int num_bases = 0;
    ASTNode* bases_node = NULL;   
    ASTNode* type_body = NULL;

    for (int i = 0; i < node->num_children; ++i) {
        ASTNode* child = node->children[i];
        if (child->type == AST_IDENTIFIER && !type_name) {
            type_name = child->value;
        } else if (child->type == AST_BASES) {
            bases_node = child;   // ⚡️ Guarda la rama Bases entera
            for (int j = 0; j < child->num_children && num_bases < 8; ++j) {
                if (child->children[j]->type == AST_IDENTIFIER) {
                    bases[num_bases++] = child->children[j]->value;
                }
            }
        } else if (child->type == AST_STATEMENT_LIST) {
            type_body = child;
        }
    }

    if (!type_name) {
        add_error(error_list, node->line, node->column, "Error: tipo sin nombre");
        break;
    }

    // ⚡️ Ahora pasas bases_node también
    insert_type(type_table, type_name, bases, num_bases, bases_node, type_body, error_list);

    if (type_body) {
        process_members(type_body, sym_table, type_table, type_name);
    }
    break;
}

        case AST_RETURN: {
            if (node->num_children < 1) {
                add_error(error_list,node->line , node->column , "Error: Return sin valor");
                return;
            }
            // const char* ret_type = infer_type(node->children[0], sym_table, type_table, error_list);
            // if (!conforms(type_table, ret_type, expected_return)) {
            //     add_error(error_list, "Error: tipo retorno '%s' no se ajusta a '%s'", ret_type, expected_return);
            // }
            break;
        }

        case AST_IF: {
            if (node->num_children < 3) {
                add_error(error_list,node->line , node->column , "Error: If incompleto");
                return;
            }
            const char* cond_type = infer_type(node->children[0], sym_table, type_table, error_list);
            // if (!conforms(type_table, cond_type, "Boolean")){
            //     // add_error(error_list, "Error: condición If no es Boolean, es '%s'", cond_type);
            // }
            check_semantics(node->children[1], sym_table, type_table, expected_return, error_list);
            check_semantics(node->children[2], sym_table, type_table, expected_return, error_list);
            break;
        }

        case AST_WHILE:
        case AST_FOR:
            for (int i = 0; i < node->num_children; ++i)
                check_semantics(node->children[i], sym_table, type_table, expected_return, error_list);
            break;

        case AST_MEMBER: {
            if (node->num_children < 2) break;
            const char* obj_type = infer_type(node->children[0], sym_table, type_table, error_list);
            const char* member_name = node->children[1]->value;
            const char* member_type = lookup_member_type(type_table, obj_type, member_name);
            if (strcmp(member_type, "undefined") == 0) {
                add_error(error_list, node ->line , node->column ,"Error: miembro '%s' no existe en '%s'", member_name, obj_type);
            }
            break;
        }

        case AST_PRINT:
        case AST_BINOP:
        case AST_FUNCTION_CALL:
        case AST_IS:
        case AST_AS:
            infer_type(node, sym_table, type_table, error_list);
            break;

        case AST_STATEMENT_LIST:
            for (int i = 0; i < node->num_children; ++i)
                check_semantics(node->children[i], sym_table, type_table, expected_return, error_list);
            break;

        default:
            for (int i = 0; i < node->num_children; ++i)
                check_semantics(node->children[i], sym_table, type_table, expected_return, error_list);
            break;
    }
}

void process_members(ASTNode* node, SymbolTable* sym_table, TypeTable* type_table, const char* type_name) {
  if (!node) return;

  if (node->type == AST_STATEMENT_LIST) {
    for (int i = 0; i < node->num_children; ++i) {
      process_members(node->children[i], sym_table, type_table, type_name);
    }
  }

  else if (node->type == AST_ASSIGN) {
    if (node->num_children >= 1) {
      const char* attr_name = node->children[0]->value;

 
      const char* attr_type = "Object";  // fallback genérico
      if (node->num_children > 1) {
        ASTNode* rhs = node->children[1];
        if (rhs->type == AST_NUMBER) {
          attr_type = "Number";
        } else if (rhs->type == AST_STRING) {
          attr_type = "String";
        } else if (rhs->type == AST_BOOL) {
          attr_type = "Boolean";
        }
      }

      int default_value = 0;
      if (node->num_children > 1 && node->children[1]->type == AST_NUMBER) {
        default_value = atoi(node->children[1]->value);
      }

      add_member_to_type(type_table, type_name, attr_name, attr_type, NULL, default_value);
      printf("✅ Guardado atributo %s::%s : %s (default=%d)\n",
             type_name, attr_name, attr_type, default_value);
    }
  }

  else if (node->type == AST_VAR_DECL) {
    const char* attr_name = node->children[0]->value;
    const char* attr_type = "Object";  // fallback genérico
    int default_value = 0;

    if (node->num_children >= 2) {
      ASTNode* maybe_type = node->children[1];
      if (maybe_type->type == AST_TYPE_SPEC && maybe_type->num_children > 0) {
        attr_type = maybe_type->children[0]->value;  // Usa IDENTIFIER de TypeSpec
      } else if (maybe_type->type == AST_NUMBER) {
        attr_type = "Number";
        default_value = atoi(maybe_type->value);
      } else if (maybe_type->type == AST_STRING) {
        attr_type = "String";
      }
    }

    add_member_to_type(type_table, type_name, attr_name, attr_type, NULL, default_value);
    printf("✅ Guardado atributo %s::%s : %s (default=%d)\n",
           type_name, attr_name, attr_type, default_value);
  }

  else if (node->type == AST_FUNCTION_DECL) {
    const char* func_name = node->children[0]->value;
    const char* return_type = "Object";  // Usa Object como default si no tienes el tipo inferido

    // 🚩 Guarda el nodo FUNCTION_DECL entero
    add_member_to_type(type_table, type_name, func_name, return_type, node, 0);
    printf("✅ Guardado método %s::%s : %s\n", type_name, func_name, return_type);

    insert_symbol(sym_table, func_name, return_type, SYMBOL_FUNCTION);
  }

  else {
    for (int i = 0; i < node->num_children; ++i) {
      process_members(node->children[i], sym_table, type_table, type_name);
    }
  }
}

void emit_method_signature(char* buffer, size_t buf_size,
                           const char* return_type,
                           const char* class_name) {
  const char* llvm_ret = map_type_to_llvm(return_type);
  snprintf(buffer, buf_size, "%s (%%%s*)*", llvm_ret, class_name);
}
const char* map_type_to_llvm(const char* type) {
  if (strcmp(type, "Number") == 0) return "i32";
  if (strcmp(type, "Boolean") == 0) return "i1";
  if (strcmp(type, "String") == 0) return "i8*";
  return "i8*"; // fallback
}

// --- VTABLE DEBUG: imprimir todas las vtables ---
void print_vtable_info(TypeTable* table) {
    printf("=== VTABLE INFO ===\n");
    for (TypeEntry* t = table->head; t; t = t->next) {
        printf("Clase %s: %d método(s)\n", t->name, t->method_count);
        for (int i = 0; i < t->method_count; ++i) {
            printf("  [%2d] %s\n", i, t->method_names[i]);
        }
    }
    printf("===================\n");
}

