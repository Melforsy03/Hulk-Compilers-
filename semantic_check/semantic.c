#include "semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    s->kind = kind;
    s->next = table->head;
    table->head = s;
}

// -------------------- Tabla de Tipos --------------------
void init_type_table(TypeTable* table) { table->head = NULL; }

TypeEntry* lookup_type(TypeTable* table, const char* name) {
    TypeEntry* current = table->head;
    while (current) {
        if (strcmp(current->name, name) == 0) return current;
        current = current->next;
    }
    return NULL;
}

void insert_type(TypeTable* table, const char* name, const char** bases, int num_bases) {
    TypeEntry* t = malloc(sizeof(TypeEntry));
    strcpy(t->name, name);
    t->num_bases = num_bases > 0 ? num_bases : 1;
    if (num_bases == 0) {
        strcpy(t->bases[0], "Object");
    } else {
        for (int i = 0; i < num_bases; ++i) {
            strncpy(t->bases[i], bases[i], 31);
            t->bases[i][31] = '\0';
        }
    }
    t->members = NULL;
    t->next = table->head;
    table->head = t;
}

void add_member_to_type(TypeTable* table, const char* type_name, const char* member_name, const char* member_type) {
    TypeEntry* t = lookup_type(table, type_name);
    if (!t) return;

    Member* m = malloc(sizeof(Member));
    strcpy(m->name, member_name);
    strcpy(m->type, member_type);
    m->next = t->members;
    t->members = m;
}

const char* lookup_member_type(TypeTable* table, const char* type_name, const char* member_name) {
    TypeEntry* t = lookup_type(table, type_name);
    if (!t) return "undefined";

    Member* m = t->members;
    while (m) {
        if (strcmp(m->name, member_name) == 0) return m->type;
        m = m->next;
    }
    for (int i = 0; i < t->num_bases; ++i) {
        const char* found = lookup_member_type(table, t->bases[i], member_name);
        if (strcmp(found, "undefined") != 0) return found;
    }
    return "undefined";
}

int conforms(TypeTable* table, const char* t1, const char* t2) {
    if (strcmp(t1, t2) == 0) return 1;
    if (strcmp(t2, "Object") == 0) return 1;

    TypeEntry* child = lookup_type(table, t1);
    if (!child) return 0;

    for (int i = 0; i < child->num_bases; ++i) {
        if (conforms(table, child->bases[i], t2)) return 1;
    }
    return 0;
}

// -------------------- Inferencia --------------------
const char* infer_type(ASTNode* node, SymbolTable* sym_table, TypeTable* type_table) {
    if (!node) return "unknown";

    switch (node->type) {
        case AST_NUMBER:
            return "Number"; // No existe Real; todo número es Number

        case AST_STRING:
            return "String";

        case AST_BOOL:
            return "Boolean";

        case AST_IDENTIFIER: {
            Symbol* s = lookup(sym_table, node->value);
            if (s) return s->type;
            printf("Error: símbolo '%s' no declarado\n", node->value);
            return "undefined";
        }

        case AST_BINOP: {
            if (node->num_children < 2) {
                printf("Error: BinOp incompleto\n");
                return "error";
            }
            const char* left = infer_type(node->children[0], sym_table, type_table);
            const char* right = infer_type(node->children[1], sym_table, type_table);

            if (strcmp(left, "Number") == 0 && strcmp(right, "Number") == 0) {
                return "Number";
            }

            if (!conforms(type_table, left, right)) {
                printf("Error: tipos incompatibles (%s vs %s)\n", left, right);
                return "error";
            }

            // Por defecto, hereda el tipo izquierdo si conforma
            return left;
        }

        case AST_IF: {
            if (node->num_children < 3) {
                printf("Error: If incompleto\n");
                return "Object";
            }
            const char* cond_type = infer_type(node->children[0], sym_table, type_table);
            if (!conforms(type_table, cond_type, "Boolean")) {
                printf("Error: condición If no es Boolean, es '%s'\n", cond_type);
            }
            const char* t1 = infer_type(node->children[1], sym_table, type_table);
            const char* t2 = infer_type(node->children[2], sym_table, type_table);

            if (conforms(type_table, t1, t2)) return t2;
            if (conforms(type_table, t2, t1)) return t1;

            return "Object"; // Mínimo ancestro común no existe
        }

        case AST_STATEMENT_LIST: {
            if (node->num_children == 0) return "Object";
            return infer_type(node->children[node->num_children - 1], sym_table, type_table);
        }

        case AST_RETURN: {
            if (node->num_children < 1) return "Object";
            return infer_type(node->children[0], sym_table, type_table);
        }

        default:
            return "unknown";
    }
}

void check_semantics(ASTNode* node, SymbolTable* sym_table, TypeTable* type_table, const char* expected_return) {
    if (!node) return;

    switch (node->type) {
        case AST_LET: {
            if (node->num_children < 2) {
                printf("Error: Let incompleto\n");
                return;
            }
            ASTNode* decl_list = node->children[0];
            for (int i = 0; i < decl_list->num_children; ++i) {
                check_semantics(decl_list->children[i], sym_table, type_table, expected_return);
            }
            check_semantics(node->children[1], sym_table, type_table, expected_return);
            break;
        }

        case AST_TYPE_DECL: {
            const char* type_name = NULL;
            const char* bases[8];
            int num_bases = 0;

            for (int i = 0; i < node->num_children; ++i) {
                if (node->children[i]->type == AST_IDENTIFIER && !type_name) {
                    type_name = node->children[i]->value;
                } else if (node->children[i]->type == AST_IDENTIFIER && type_name) {
                    bases[num_bases++] = node->children[i]->value;
                }
            }

            insert_type(type_table, type_name, bases, num_bases);

            for (int i = 0; i < node->num_children; ++i) {
                check_semantics(node->children[i], sym_table, type_table, expected_return);
            }
            break;
        }

        case AST_VAR_DECL: {
            const char* name = node->children[0]->value;
            const char* declared_type = NULL;
            ASTNode* init_expr = NULL;

            if (node->num_children == 4) {
                declared_type = node->children[1]->value;
                init_expr = node->children[3];
            } else {
                init_expr = node->children[node->num_children - 1];
            }

            const char* expr_type = infer_type(init_expr, sym_table, type_table);

            if (strcmp(expr_type, "unknown") == 0 || strcmp(expr_type, "undefined") == 0) {
                printf("Error: no se pudo inferir tipo para '%s'. Debe anotarse.\n", name);
                expr_type = "Object";
            }

            if (declared_type) {
                if (!conforms(type_table, expr_type, declared_type)) {
                    printf("Error: tipo '%s' no se ajusta a '%s'\n", expr_type, declared_type);
                }
            } else {
                printf("Info: '%s' se infiere como %s\n", name, expr_type);
            }

            insert_symbol(sym_table, name, declared_type ? declared_type : expr_type, SYMBOL_VARIABLE);
            break;
        }

        case AST_ASSIGN: {
            if (node->num_children < 2) {
                printf("Error: Assign incompleto\n");
                return;
            }
            const char* name = node->children[0]->value;
            Symbol* s = lookup(sym_table, name);
            if (!s) {
                printf("Error: variable '%s' no declarada\n", name);
                break;
            }
            const char* expr_type = infer_type(node->children[1], sym_table, type_table);
            if (!conforms(type_table, expr_type, s->type)) {
                printf("Error: tipo '%s' no se ajusta a '%s'\n", expr_type, s->type);
            }
            break;
        }

        case AST_FUNCTION_DECL: {
            const char* func_name = node->children[0]->value;
            const char* declared_return = node->num_children >= 3 ? node->children[2]->value : "Object";
            ASTNode* body = node->num_children >= 4 ? node->children[3] : NULL;

            // Chequeo semántico recursivo para cuerpo con expected_return
            if (body) check_semantics(body, sym_table, type_table, declared_return);

            const char* actual_return = body ? infer_type(body, sym_table, type_table) : "Object";
            if (!conforms(type_table, actual_return, declared_return)) {
                printf("Error: tipo de retorno '%s' no se ajusta a '%s'\n", actual_return, declared_return);
            }

            insert_symbol(sym_table, func_name, declared_return, SYMBOL_FUNCTION);
            break;
        }

        case AST_RETURN: {
            if (node->num_children < 1) {
                printf("Error: Return sin valor\n");
                return;
            }
            const char* ret_type = infer_type(node->children[0], sym_table, type_table);
            if (!conforms(type_table, ret_type, expected_return)) {
                printf("Error: tipo retorno '%s' no se ajusta a '%s'\n", ret_type, expected_return);
            }
            break;
        }

        case AST_IF: {
            if (node->num_children < 3) {
                printf("Error: If incompleto\n");
                return;
            }
            const char* cond_type = infer_type(node->children[0], sym_table, type_table);
            if (!conforms(type_table, cond_type, "Boolean")) {
                printf("Error: condición If no es Boolean, es '%s'\n", cond_type);
            }
            check_semantics(node->children[1], sym_table, type_table, expected_return);
            check_semantics(node->children[2], sym_table, type_table, expected_return);
            break;
        }

        case AST_WHILE:
        case AST_FOR: {
            for (int i = 0; i < node->num_children; ++i) {
                check_semantics(node->children[i], sym_table, type_table, expected_return);
            }
            break;
        }

        case AST_PRINT:
        case AST_BINOP:
        case AST_FUNCTION_CALL:
        case AST_MEMBER:
        case AST_IS:
        case AST_AS: {
            infer_type(node, sym_table, type_table);
            break;
        }

        default:
            for (int i = 0; i < node->num_children; ++i) {
                check_semantics(node->children[i], sym_table, type_table, expected_return);
            }
            break;
    }
}
