#include "semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

// ----------------- Tabla Virtual -------------------

TypeEntry* lookup_class(TypeTable* table, const char* name) {
    return lookup_type(table, name);
}

void build_vtable_info(TypeTable* table) {
    TypeEntry* t = table->head;
    while (t) {
        // Primero procesar las clases base para heredar métodos
        for (int i = 0; i < t->num_bases; i++) {
            TypeEntry* base = lookup_type(table, t->bases[i]);
            if (base) {
                MethodEntry* m = base->methods;
                while (m) {
                    // Heredar métodos de la base
                    if (!lookup_method(t, m->name)) {
                        add_method_to_type(t, m->name, m->signature);
                    }
                    m = m->next;
                }
            }
        }

        // Luego procesar los métodos propios
        Member* m = t->members;
        while (m) {
            if (m->body) { // Es un método
                MethodEntry* existing = lookup_method(t, m->name);
                if (existing) {
                    // Sobrescribir método
                    strcpy(existing->signature, m->type);
                } else {
                    // Agregar nuevo método
                    add_method_to_type(t, m->name, m->type);
                }
            }
            m = m->next;
        }
        t = t->next;
    }
}

MethodEntry* lookup_method(TypeEntry* type, const char* name) {
    MethodEntry* m = type->methods;
    while (m) {
        if (strcmp(m->name, name) == 0) return m;
        m = m->next;
    }
    return NULL;
}

void add_method_to_type(TypeEntry* type, const char* name, const char* signature) {
    MethodEntry* m = malloc(sizeof(MethodEntry));
    strcpy(m->name, name);
    strcpy(m->signature, signature);
    m->vtable_index = type->method_count++;
    m->next = type->methods;
    type->methods = m;
}

int get_method_index(TypeTable* table, const char* type_name, const char* method_name) {
    TypeEntry* t = lookup_type(table, type_name);
    if (!t) return -1;
    for (int i = 0; i < t->method_count; i++) {
        if (strcmp(t->method_names[i], method_name) == 0) {
            return i;
        }
    }
    return -1;
}

// -------------------- Tabla de Símbolos --------------------
void init_symbol_table(SymbolTable* table) { table->head = NULL; }

Symbol* lookup(SymbolTable* table, const char* name) {
    Symbol* current = table->head;
    while (current) {
        if (strcmp(current->name, name) == 0) return current;
        current = current->next;
    }
    return NULL;
}

void insert_symbol(SymbolTable* table, const char* name, const char* type, SymbolKind kind) {
    Symbol* s = malloc(sizeof(Symbol));
    strcpy(s->name, name);
   
    strcpy(s->type, type);
    s->dynamic_type[0] = '\0'; 
    s->kind = kind;
    s->next = table->head;
    table->head = s;
}

// -------------------- Tabla de Tipos --------------------
void init_type_table(TypeTable* table) {
    table->head = NULL;
    // Insertar tipos básicos
  
}
void add_error(ErrorList* list, const char* fmt, ...) {
    if (list->count >= MAX_ERRORS) return;

    va_list args;
    va_start(args, fmt);

    char* msg = malloc(256);
    vsnprintf(msg, 256, fmt, args);
    list->messages[list->count++] = msg;

    va_end(args);
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

        SymbolTable local_scope = *global_sym;
        insert_symbol(&local_scope, "this", t->name, SYMBOL_VARIABLE);

        // ⚡️ Pasa el tipo de la clase como expected_return, no el tipo de retorno
        check_semantics(m->body, &local_scope, type_table, t->name ,list);
      }
      m = m->next;
    }
    t = t->next;
  }
}

void insert_type(TypeTable* table, const char* name, const char** bases, int num_bases, ASTNode* members_node) {
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

void add_member_to_type(TypeTable* table, const char* type_name, const char* name, const char* type, ASTNode* body) {
  TypeEntry* t = lookup_type(table, type_name);
  if (!t) {
    fprintf(stderr, "Error: tipo '%s' no encontrado\n", type_name);
    exit(1);
  }

 
  Member* m = t->members;
  while (m) {
    if (strcmp(m->name, name) == 0) {
      return; // Ya existe
    }
    m = m->next;
  }

  Member* new_member = malloc(sizeof(Member));
  strcpy(new_member->name, name);
  strcpy(new_member->type, type);
  new_member->body = body; // ✅ CORRECTO
  new_member->next = t->members;
  t->members = new_member;
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

            add_error(error_list, "Error: símbolo '%s' no declarado", node->value);
            return "undefined";
        }

        case AST_BINOP: {
    if (node->num_children < 2) {
        add_error(error_list, "Error: BinOp incompleto");
        return "error";
    }

    const char* left = infer_type(node->children[0], sym_table, type_table, error_list);
    const char* right = infer_type(node->children[1], sym_table, type_table, error_list);
    const char* op = node->value;

    // Operaciones aritméticas solo si los tipos coinciden
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
        strcmp(op, "*") == 0 || strcmp(op, "/") == 0) {

        if (strcmp(left, right) == 0) {
            return left;  // Devuelve el tipo común (ej: Number, Boolean, String...)
        } else {
            add_error(error_list, "Error: operación aritmética con tipos diferentes (%s y %s)", left, right);
            return "error";
        }
    }

    // Comparaciones siempre retornan Boolean, pero tipos deben coincidir
    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
        strcmp(op, "<") == 0 || strcmp(op, ">") == 0 || strcmp(op, ">=") == 0 || strcmp(op, "<=") == 0 || strcmp(op, "&&") || strcmp(op, "or") ) {

        if (strcmp(left, right) == 0) {
            return "Boolean";
        } else {
            add_error(error_list, "Error: comparación entre tipos distintos (%s vs %s)", left, right);
            return "error";
        }
    }

    // Para operadores desconocidos
    add_error(error_list, "Error: operador binario '%s' no soportado", op);
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
                add_error(error_list, "Error: If incompleto");
                return "Object";
            }
            const char* cond_type = infer_type(node->children[0], sym_table, type_table, error_list);
            if (!conforms(type_table, cond_type, "Boolean")){
                add_error(error_list, "Error: condición If no es Boolean, es '%s'", cond_type);
            }

            const char* t1 = infer_type(node->children[1], sym_table, type_table, error_list);
            const char* t2 = infer_type(node->children[2], sym_table, type_table, error_list);

            if (conforms(type_table, t1, t2)) return t2;
            if (conforms(type_table, t2, t1)) return t1;
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
                    add_error(error_list, "Error: Elemento %d incompatible (%s vs %s)", i, elem_type, first);
                }
            }
            static char buffer[64];
            snprintf(buffer, sizeof(buffer), "Vector[%d]", node->num_children);
            return buffer;
        }

        case AST_FUNCTION_CALL: {
            if (node->num_children < 1 && !node->value) {
                add_error(error_list, "Error: FunctionCall incompleto");
                return "Object";
            }

            const char* func_name = NULL;

            if (node->num_children >= 1) {
                ASTNode* func_id = node->children[0];
                if (func_id->type == AST_IDENTIFIER) {
                    func_name = func_id->value;
                } else if (func_id->type == AST_MEMBER) {
                    const char* obj_type = infer_type(func_id->children[0], sym_table, type_table, error_list);
                    const char* member_name = func_id->children[1]->value;
                    const char* member_type = lookup_member_type(type_table, obj_type, member_name);
                    if (strcmp(member_type, "undefined") == 0) {
                        add_error(error_list, "Error: miembro '%s' no existe en '%s'", member_name, obj_type);
                        return "Number";
                    }
                    return member_type;
                } else {
                    add_error(error_list, "Error: FunctionCall mal formado (ni Identifier ni Member)");
                    return "Object";
                }
            } else if (node->value && strlen(node->value) > 0) {
                func_name = node->value;
            } else {
                add_error(error_list, "Error: FunctionCall mal formado (sin Identifier ni value)");
                return "Number";
            }

            if (func_name) {
                Symbol* s = lookup(sym_table, func_name);
                if (!s) {
                    add_error(error_list, "Error: función '%s' no declarada", func_name);
                    return "Number";
                }
                return s->type;
            }

            return "Number";
        }

        case AST_MEMBER: {
            const char* base_type = infer_type(node->children[0], sym_table, type_table, error_list);
            const char* member_name = node->children[1]->value;
            const char* member_type = lookup_member_type(type_table, base_type, member_name);

            if (strcmp(member_type, "undefined") == 0) {
                add_error(error_list, "Error: miembro '%s' no existe en tipo '%s'", member_name, base_type);
                return "error";
            }

            return member_type;
        }

        case AST_NEW_EXPR:
            if (node->num_children > 0 && node->children[0]->type == AST_IDENTIFIER) {
                const char* class_name = node->children[0]->value;
                if (!lookup_type(type_table, class_name)) {
                    add_error(error_list, "Error: tipo '%s' no declarado", class_name);
                    return "undefined";
                }
                return class_name;
            }
            add_error(error_list, "Error: constructor 'new' mal formado");
            return "undefined";

        default:
            return "unknown";
    }
}

void check_semantics(ASTNode* node, SymbolTable* sym_table, TypeTable* type_table, const char* expected_return, ErrorList* error_list) {
    if (!node) return;

    switch (node->type) {
        case AST_LET: {
            if (node->num_children < 2) {
                add_error(error_list, "Error: Let incompleto");
                return;
            }

            ASTNode* decl_list = node->children[0];
            for (int i = 0; i < decl_list->num_children; ++i) {
                ASTNode* decl = decl_list->children[i];
                if (decl->type == AST_VAR_DECL) {
                    const char* var_name = decl->children[0]->value;
                    const char* var_type = "Object";
                    if (decl->num_children > 2) {
                        ASTNode* init_expr = decl->children[decl->num_children - 1];
                        var_type = infer_type(init_expr, sym_table, type_table, error_list);
                    }
                    insert_symbol(sym_table, var_name, var_type, SYMBOL_VARIABLE);
                }
                check_semantics(decl, sym_table, type_table, expected_return, error_list);
            }

            check_semantics(node->children[1], sym_table, type_table, expected_return, error_list);
            break;
        }
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

        case AST_VAR_DECL: {
            const char* name = NULL;
            const char* declared_type = NULL;
            ASTNode* init_expr = NULL;

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
                else if (child->type == AST_FUNCTION_CALL || child->type == AST_BINOP ||
                         child->type == AST_NUMBER || child->type == AST_STRING ||
                         child->type == AST_VECTOR || child->type == AST_NEW_EXPR ||
                         child->type == AST_BOOL || child->type == AST_IDENTIFIER) {
                    init_expr = child;
                }
            }

            if (!name || !init_expr) {
                add_error(error_list, "Error: declaración de variable incompleta");
                break;
            }

            const char* expr_type = infer_type(init_expr, sym_table, type_table, error_list);

            if (strcmp(expr_type, "unknown") == 0 || strcmp(expr_type, "undefined") == 0) {
                add_error(error_list, "Error: no se pudo inferir tipo para '%s'. Debe anotarse.", name);
                expr_type = "Number";
            }

            if (declared_type) {
                if (!conforms(type_table, expr_type, declared_type)) {
                    add_error(error_list, "Error: tipo '%s' no se ajusta a '%s' en la variable '%s'",
                              expr_type, declared_type, name);
                }
            }

            insert_symbol(sym_table, name, declared_type ? declared_type : expr_type, SYMBOL_VARIABLE);
            break;
        }

        case AST_ASSIGN: {
            if (node->num_children < 2) {
                add_error(error_list, "Error: Assign incompleto");
                return;
            }

            ASTNode* lhs = node->children[0];
            const char* expr_type = infer_type(node->children[1], sym_table, type_table, error_list);

            if (lhs->type == AST_IDENTIFIER) {
                const char* name = lhs->value;
                if (!name || strlen(name) == 0) {
                    add_error(error_list, "Error: nombre de variable vacío en Assign");
                    break;
                }
                Symbol* s = lookup(sym_table, name);
                if (!s) {
                    add_error(error_list, "Error: variable '%s' no declarada", name);
                    break;
                }
                if (!conforms(type_table, expr_type, s->type)) {
                    add_error(error_list, "Error: tipo '%s' no se ajusta a '%s'", expr_type, s->type);
                }
            } else if (lhs->type == AST_MEMBER) {
                const char* base_type = infer_type(lhs->children[0], sym_table, type_table, error_list);
                const char* member_name = lhs->children[1]->value;
                const char* member_type = lookup_member_type(type_table, base_type, member_name);
                if (strcmp(member_type, "undefined") == 0) {
                    add_error(error_list, "Error: miembro '%s' no existe en '%s'", member_name, base_type);
                } else if (!conforms(type_table, expr_type, member_type)) {
                    add_error(error_list, "Error: tipo '%s' no se ajusta a '%s'", expr_type, member_type);
                }
            }
            break;
        }

        case AST_FUNCTION_DECL: {
            const char* func_name = node->children[0]->value;
            const char* declared_return = "Object";
            ASTNode* body = NULL;
            ASTNode* param_list = NULL;
            ASTNode* return_type_node = NULL;

            // 1️⃣ Identifica nodos clave
            for (int i = 1; i < node->num_children; ++i) {
                if (node->children[i]->type == AST_PARAM_LIST) {
                    param_list = node->children[i];
                } else if (node->children[i]->type == AST_TYPE_SPEC) {
                    return_type_node = node->children[i];
                } else if (node->children[i]->type == AST_STATEMENT_LIST) {
                    body = node->children[i];
                }
            }

            // 2️⃣ Extrae tipo de retorno directamente (AST_TYPE_SPEC → AST_IDENTIFIER)
            if (return_type_node && return_type_node->num_children > 0 &&
                return_type_node->children[0]->type == AST_IDENTIFIER) {
                declared_return = return_type_node->children[0]->value;
            }

            // 3️⃣ Crea scope local
            SymbolTable local_scope = *sym_table;
            if (expected_return && strcmp(expected_return, "Object") != 0) {
                insert_symbol(&local_scope, "this", expected_return, SYMBOL_VARIABLE);
            }

            // 4️⃣ Inserta parámetros
            if (param_list) {
                for (int i = 0; i < param_list->num_children; ++i) {
                    ASTNode* param = param_list->children[i];
                    if (param->type == AST_IDENTIFIER) {
                        const char* param_name = param->value;
                        const char* param_type = "Object";
                        if (param->num_children >= 1 && param->children[0]->type == AST_TYPE_SPEC &&
                            param->children[0]->num_children > 0 &&
                            param->children[0]->children[0]->type == AST_IDENTIFIER) {
                            param_type = param->children[0]->children[0]->value;
                        }
                        insert_symbol(&local_scope, param_name, param_type, SYMBOL_VARIABLE);
                    }
                }
            }

            // 5️⃣ Verifica cuerpo y tipo de retorno
            if (body) {
                check_semantics(body, &local_scope, type_table, declared_return, error_list);
                const char* inferred_return = infer_type(body, &local_scope, type_table, error_list);
                if (!conforms(type_table, inferred_return, declared_return)) {
                    add_error(error_list,
                        "Error en función '%s': tipo de retorno inferido '%s' no coincide con declarado '%s'",
                        func_name, inferred_return, declared_return);
                }
            }

            // 6️⃣ Inserta símbolo global
            insert_symbol(sym_table, func_name, declared_return, SYMBOL_FUNCTION);
            break;
        }

        case AST_TYPE_DECL: {
            if (node->num_children < 2) {
                add_error(error_list, "Error: declaración de tipo incompleta");
                break;
            }

            const char* type_name = NULL;
            const char* bases[8] = {0};
            int num_bases = 0;
            ASTNode* type_body = NULL;

            for (int i = 0; i < node->num_children; ++i) {
                ASTNode* child = node->children[i];
                if (child->type == AST_IDENTIFIER && !type_name) {
                    type_name = child->value;
                } else if (child->type == AST_BASES) {
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
                add_error(error_list, "Error: tipo sin nombre");
                break;
            }

            insert_type(type_table, type_name, bases, num_bases, type_body);
            if (type_body) {
                process_members(type_body, sym_table, type_table, type_name);
            }
            break;
        }

        case AST_RETURN: {
            if (node->num_children < 1) {
                add_error(error_list, "Error: Return sin valor");
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
                add_error(error_list, "Error: If incompleto");
                return;
            }
            const char* cond_type = infer_type(node->children[0], sym_table, type_table, error_list);
            if (!conforms(type_table, cond_type, "Boolean")){
                add_error(error_list, "Error: condición If no es Boolean, es '%s'", cond_type);
            }
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
                add_error(error_list, "Error: miembro '%s' no existe en '%s'", member_name, obj_type);
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
        // Recorre todos los hijos recursivamente
        for (int i = 0; i < node->num_children; ++i) {
            process_members(node->children[i], sym_table, type_table, type_name);
        }
    }
    else if (node->type == AST_ASSIGN) {
        // Ejemplo: radius := 1;
        if (node->num_children >= 1) {
            const char* attr_name = node->children[0]->value;
            const char* attr_type = "Number"; // O inferirlo si quieres

            add_member_to_type(type_table, type_name, attr_name, attr_type, NULL);
            printf("Guardado atributo %s::%s : %s\n", type_name, attr_name, attr_type);
        }
    }
    else if (node->type == AST_VAR_DECL) {
        // Ejemplo: var foo: Number = 1;
        const char* attr_name = node->children[0]->value;
        const char* attr_type = "Object";

        if (node->num_children >= 2) {
            // Puede tener TypeSpec
            if (node->children[1]->type == AST_TYPE_SPEC) {
                attr_type = node->children[1]->value;
            } else if (node->children[1]->type == AST_NUMBER) {
                attr_type = "Number";
            }
        }

        add_member_to_type(type_table, type_name, attr_name, attr_type, NULL);
        printf("Guardado atributo %s::%s : %s\n", type_name, attr_name, attr_type);
    }
    else if (node->type == AST_FUNCTION_DECL) {
        const char* func_name = node->children[0]->value;
        const char* return_type = (node->num_children >= 3) ? node->children[2]->value : "Object";
        ASTNode* body = (node->num_children >= 4) ? node->children[3] : NULL;

        add_member_to_type(type_table, type_name, func_name, return_type, body);
        printf("Guardado método %s::%s : %s\n", type_name, func_name, return_type);

        // Opcional: registrar en tabla de símbolos global
        insert_symbol(sym_table, func_name, return_type, SYMBOL_FUNCTION);
    }
    else {
        // Si hay nodos que anidan declaraciones
        for (int i = 0; i < node->num_children; ++i) {
            process_members(node->children[i], sym_table, type_table, type_name);
        }
    }
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

