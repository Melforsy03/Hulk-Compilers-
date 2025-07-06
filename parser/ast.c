#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

// Crear nodo AST
ASTNode* create_ast_node(ASTNodeType type, const char* value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = type;
    if (value) {
        strncpy(node->value, value, 31);
        node->value[31] = '\0';
    } else {
        node->value[0] = '\0';
    }
    node->capacity = 2;
    node->num_children = 0;
    node->children = malloc(sizeof(ASTNode*) * node->capacity);
    return node;
}

// Agregar hijo
void add_ast_child(ASTNode* parent, ASTNode* child) {
    if (parent->num_children >= parent->capacity) {
        parent->capacity *= 2;
        parent->children = realloc(parent->children, sizeof(ASTNode*) * parent->capacity);
    }
    parent->children[parent->num_children++] = child;
}

// Liberar AST
void free_ast(ASTNode* node) {
    for (int i = 0; i < node->num_children; ++i) {
        free_ast(node->children[i]);
    }
    free(node->children);
    free(node);
}

// Imprimir AST
void print_ast(const ASTNode* node, int indent) {
    if (!node) return;

    for (int i = 0; i < indent; ++i) printf("  ");
    printf("%s", node_type_to_str(node->type));
    if (strlen(node->value) > 0) printf(" [%s]", node->value);
    printf("\n");

    for (int i = 0; i < node->num_children; ++i) {
        print_ast(node->children[i], indent + 1);
    }
}

const char* node_type_to_str(ASTNodeType type) {
  switch (type) {
    case AST_BINOP: return "BinOp";
    case AST_ASSIGN: return "Assign";
    case AST_IDENTIFIER: return "Identifier";
    case AST_NUMBER: return "Number";
    case AST_PROGRAM: return "Program";
    case AST_STATEMENT_LIST: return "StatementList";
    case AST_PRINT: return "Print";
    case AST_ARGUMENT_LIST: return "ArgumentList";
    case AST_LET: return "Let";
    case AST_VAR_DECL_LIST: return "VarDeclList";
    case AST_VAR_DECL: return "VarDecl";
    case AST_ASSIGN_OP :return "AssignOp";
    case AST_WHILE: return "While";
    case AST_IF :return "If";
    case AST_FOR: return "For";
    case AST_VECTOR :return "Vector";
    case AST_FUNCTION_DECL :return "function";
    case AST_PARAM_LIST: return "ParamsList";
    case AST_UNARYOP: return "UnaryOp";
    case AST_TYPE_DECL: return "Type";
    case AST_STRING: return "String";
    case AST_INDEX  : return "Index";
    case AST_MEMBER : return "Member";
    case AST_FUNCTION_CALL : return "Function Call";
    case AST_IS: return "Is";
    case AST_AS: return "As";
    case AST_BOOL :return "Bool";


    default: return "Unknown";
  }
}
ASTNode* parse_atom_suffixes(ASTNode* base, CSTNode* suffixes) {
    if (!suffixes || suffixes->num_children == 0 || 
        strcmp(suffixes->children[0]->symbol, "ε") == 0) {
        return base;
    }

    // Procesar el primer sufijo
    CSTNode* first_suffix = suffixes->children[0];
    base = parse_atom_suffix(base, first_suffix);

    // Procesar sufijos restantes recursivamente
    if (suffixes->num_children > 1) {
        base = parse_atom_suffixes(base, suffixes->children[1]);
    }
    printf("📌 Suffix tipo: %s\n", suffixes->symbol);

    return base;
}
ASTNode* parse_atom_suffix(ASTNode* base, CSTNode* suffix) {
   if (strcmp(suffix->symbol, "AtomSuffix") == 0 && suffix->num_children >= 1) {
    // Baja a IndexSuffix o MemberSuffix
    return parse_atom_suffix(base, suffix->children[0]);
  }
    if (strcmp(suffix->symbol, "IndexSuffix") == 0) {
        // Buscar la expresión del índice
        ASTNode* index_expr = NULL;
        for (int i = 0; i < suffix->num_children; i++) {
            if (strcmp(suffix->children[i]->symbol, "Expr") == 0) {
                index_expr = cst_to_ast(suffix->children[i]);
                break;
            }
        }
        
        ASTNode* index_node = create_ast_node(AST_INDEX, NULL);
        add_ast_child(index_node, base);
        if (index_expr) {
            add_ast_child(index_node, index_expr);
        }
        return index_node;
    }
    else if (strcmp(suffix->symbol, "MemberSuffix") == 0) {
        // Buscar el identificador del miembro
        ASTNode* member = NULL;
        for (int i = 0; i < suffix->num_children; i++) {
            if (strcmp(suffix->children[i]->symbol, "IDENTIFIER") == 0) {
                member = create_ast_node(AST_IDENTIFIER, suffix->children[i]->token->lexema);
                break;
            }
        }
        
        ASTNode* member_node = create_ast_node(AST_MEMBER, NULL);
        add_ast_child(member_node, base);
        if (member) {
            add_ast_child(member_node, member);
        }
        return member_node;
    }
  
    return base;
}
void add_argument_list(ASTNode* parent, CSTNode* node) {
  if (!node) return;

  if (strcmp(node->symbol, "ArgumentList") == 0) {
    if (node->num_children >= 1) {
      ASTNode* expr = cst_to_ast(node->children[0]);
      if (expr) add_ast_child(parent, expr);
    }
    if (node->num_children >= 2) {
      add_argument_list(parent, node->children[1]); // ArgumentTail
    }
  }
  else if (strcmp(node->symbol, "ArgumentTail") == 0) {
    if (node->num_children >= 2) {
      ASTNode* expr = cst_to_ast(node->children[1]);
      if (expr) add_ast_child(parent, expr);
    }
    if (node->num_children >= 3) {
      add_argument_list(parent, node->children[2]); // ArgumentTail recursivo
    }
  }
}

void process_argument_list(ASTNode* parent, CSTNode* args) {
    if (!parent || !args) return;
    
    for (int i = 0; i < args->num_children; ++i) {
        CSTNode* child = args->children[i];
        
        // Saltar comas y otros delimitadores
        if (child->token && (strcmp(child->token->lexema, ",") == 0)) {
            continue;
        }
        
        // Procesar expresiones
        if (strcmp(child->symbol, "Expr") == 0) {
            ASTNode* arg = cst_to_ast(child);
            if (arg) add_ast_child(parent, arg);
        }
    }
}
// Transformación CST → AST (versión simplificada)
ASTNode* cst_to_ast(CSTNode* cst) {
    if (!cst) return NULL;
if (strcmp(cst->symbol, "ReturnType") == 0) {
  if (cst->num_children >= 2 && strcmp(cst->children[1]->symbol, "IDENTIFIER") == 0) {
    return create_ast_node(AST_IDENTIFIER, cst->children[1]->token->lexema);
  }
  return NULL;
}

if (strcmp(cst->symbol, "FunctionBody") == 0) {
  for (int i = 0; i < cst->num_children; ++i) {
    if (strcmp(cst->children[i]->symbol, "Expr") == 0 || strcmp(cst->children[i]->symbol, "ExprBlock") == 0) {
      return cst_to_ast(cst->children[i]);
    }
  }
}

// Solo salta ExprTail, no ParamsTail
if (strstr(cst->symbol, "ExprTail") != NULL) return NULL;

    // Caso IDENTIFIER o NUMBER
    if (cst->token && cst->token->lexema) {
        if (strcmp(cst->symbol, "STRING") == 0) {
            return create_ast_node(AST_STRING, cst->token->lexema);
        }
        if (strcmp(cst->symbol, "NUMBER") == 0) {
            return create_ast_node(AST_NUMBER, cst->token->lexema);
        }
    }
else if (strcmp(cst->symbol, "ConcatExpr") == 0) {
    // Usa el helper genérico para cadenas binarias
    return parse_binop_chain(cst, "ConcatExpr");
}

if (strcmp(cst->symbol, "ReturnStatement") == 0) {
  ASTNode* expr = NULL;
  for (int i = 0; i < cst->num_children; ++i) {
    if (strcmp(cst->children[i]->symbol, "Expr") == 0) {
      expr = cst_to_ast(cst->children[i]);
      break;
    }
  }
  ASTNode* return_node = create_ast_node(AST_RETURN, "");
  if (expr) add_ast_child(return_node, expr);
  return return_node;
}


else if (strcmp(cst->symbol, "TypeFuncList") == 0) {
    if (strcmp(cst->children[0]->symbol, "ε") == 0) {
      return create_ast_node(AST_STATEMENT_LIST, ""); // vacío
    } else {
      ASTNode* decl = cst_to_ast(cst->children[0]); // Declaration
      ASTNode* rest = cst_to_ast(cst->children[1]); // TypeFuncList recursivo

      ASTNode* list = create_ast_node(AST_STATEMENT_LIST, "");
      add_ast_child(list, decl);
      for (int i = 0; i < rest->num_children; i++) {
        add_ast_child(list, rest->children[i]);
      }
      return list;
    }
  }
else if (strcmp(cst->symbol, "Inherits") == 0) {
  if (cst->num_children >= 2) {

    ASTNode* inherits_node = create_ast_node(AST_IDENTIFIER, cst->children[1]->token->lexema);


    if (cst->num_children >= 3) {
      CSTNode* arg_list_opt = cst->children[2];

      // ArgumentListOpt ::= '(' ArgumentList ')'
      for (int i = 0; i < arg_list_opt->num_children; ++i) {
        if (strcmp(arg_list_opt->children[i]->symbol, "ArgumentList") == 0) {
          ASTNode* args_node = create_ast_node(AST_ARGUMENT_LIST, NULL);

          add_argument_list(args_node, arg_list_opt->children[i]);

          add_ast_child(inherits_node, args_node);
          break; // solo uno
        }
      }
    }

    return inherits_node;
  }
}

if (strcmp(cst->symbol, "STRING") == 0) {
  return create_ast_node(AST_STRING, cst->token->lexema);
}

// ParamsList
if (strcmp(cst->symbol, "ParamsList") == 0) {
  ASTNode* params = create_ast_node(AST_PARAM_LIST, NULL);
  for (int i = 0; i < cst->num_children; ++i) {
    if (strcmp(cst->children[i]->symbol, "IDENTIFIER") == 0) {
      ASTNode* param = create_ast_node(AST_IDENTIFIER, cst->children[i]->token->lexema);
      add_ast_child(params, param);
    }
    else if (strcmp(cst->children[i]->symbol, "ParamsTail") == 0) {
      ASTNode* tail = cst_to_ast(cst->children[i]);
      if (tail) {
        for (int j = 0; j < tail->num_children; ++j) {
          if (tail->children[j]) {
            add_ast_child(params, tail->children[j]);
          }
        }
      }
    }
  }
  return params;
}

// ParamsTail
if (strcmp(cst->symbol, "ParamsTail") == 0) {
  ASTNode* temp = create_ast_node(AST_ARGUMENT_LIST, NULL);
  for (int i = 0; i < cst->num_children; ++i) {
    if (strcmp(cst->children[i]->symbol, "IDENTIFIER") == 0) {
      ASTNode* param = create_ast_node(AST_IDENTIFIER, cst->children[i]->token->lexema);
      add_ast_child(temp, param);
    }
    else if (strcmp(cst->children[i]->symbol, "ParamsTail") == 0) {
      ASTNode* tail = cst_to_ast(cst->children[i]);
      if (tail) {
        for (int j = 0; j < tail->num_children; ++j) {
          if (tail->children[j]) {
            add_ast_child(temp, tail->children[j]);
          }
        }
      }
    }
  }
  return temp;
}

if (strcmp(cst->symbol, "TypeMembers") == 0) {
  ASTNode* members = create_ast_node(AST_STATEMENT_LIST, NULL);
  for (int i = 0; i < cst->num_children; ++i) {
    ASTNode* member = cst_to_ast(cst->children[i]);
    if (member) {
      // Evita agregar StatementList vacío
      if (!(member->type == AST_STATEMENT_LIST && member->num_children == 0)) {
        add_ast_child(members, member);
      }
    }
  }
  return members;
}


if (strcmp(cst->symbol, "Type") == 0) {
  ASTNode* type_node = create_ast_node(AST_TYPE_DECL, NULL);

  for (int i = 0; i < cst->num_children; ++i) {
    if (strcmp(cst->children[i]->symbol, "IDENTIFIER") == 0) {
      ASTNode* id = create_ast_node(AST_IDENTIFIER, cst->children[i]->token->lexema);
      add_ast_child(type_node, id);
    }
    else if (strcmp(cst->children[i]->symbol, "ParamsList") == 0) {
      ASTNode* params = cst_to_ast(cst->children[i]);
      if (params) add_ast_child(type_node, params);
    }
    else if (strcmp(cst->children[i]->symbol, "Inherits") == 0) {
      ASTNode* inherits = cst_to_ast(cst->children[i]);
      if (inherits) add_ast_child(type_node, inherits);
    }
    else if (strcmp(cst->children[i]->symbol, "TypeMembers") == 0) {
      ASTNode* members = cst_to_ast(cst->children[i]);
      if (members) add_ast_child(type_node, members);
    }
  }

  return type_node;
}
if (strcmp(cst->symbol, "TypeMember") == 0) {
  if (strcmp(cst->children[0]->symbol, "IDENTIFIER") == 0) {
    ASTNode* var = create_ast_node(AST_IDENTIFIER, cst->children[0]->token->lexema);
    ASTNode* expr = NULL;

    for (int i = 0; i < cst->num_children; ++i) {
      if (strcmp(cst->children[i]->symbol, "Expr") == 0) {
        expr = cst_to_ast(cst->children[i]);
      }
    }

    if (expr) {
      ASTNode* assign = create_ast_node(AST_ASSIGN, "=");
      add_ast_child(assign, var);
      add_ast_child(assign, expr);
      return assign;
    }
  }
}

if (strcmp(cst->symbol, "StatementList") == 0) {
    // Si solo tiene un hijo, devolver directamente ese hijo
    if (cst->num_children == 1) {
        return cst_to_ast(cst->children[0]);
    }
    
    ASTNode* list = create_ast_node(AST_STATEMENT_LIST, NULL);
    for (int i = 0; i < cst->num_children; ++i) {
        ASTNode* child = cst_to_ast(cst->children[i]);
        if (child) add_ast_child(list, child);
    }
    return list;
}
if (strcmp(cst->symbol, "UnaryExpr") == 0) {
  // Caso: +Factor | -Factor | !Factor
  if (cst->num_children >= 2) {
    const char* op = cst->children[0]->symbol;
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 || strcmp(op, "!") == 0) {
      ASTNode* unary = create_ast_node(AST_UNARYOP, op);
      ASTNode* factor = cst_to_ast(cst->children[1]);
      add_ast_child(unary, factor);
      return unary;
    }
  }

  // Caso especial: is/as tail
  if (cst->num_children > 1 && strcmp(cst->children[1]->symbol, "IsAsTail") == 0) {
    CSTNode* tail = cst->children[1];
    if (tail->num_children >= 2) {
      const char* op = tail->children[0]->symbol;
      const char* type_name = tail->children[1]->token->lexema;

      ASTNode* isas_node = create_ast_node(
          strcmp(op, "is") == 0 ? AST_IS : AST_AS, op);
      ASTNode* base = cst_to_ast(cst->children[0]);
      add_ast_child(isas_node, base);
      ASTNode* type_id = create_ast_node(AST_IDENTIFIER, type_name);
      add_ast_child(isas_node, type_id);
      return isas_node;
    }
  }

  // Si no, baja como Factor normal
  return cst_to_ast(cst->children[0]);
}


if (strcmp(cst->symbol, "AdditiveExpr") == 0) {
    return parse_binop_chain(cst, "AdditiveExpr");
}
if (strcmp(cst->symbol, "MultiplicativeExpr") == 0) {
    return parse_binop_chain(cst, "MultiplicativeExpr");
}
if (strcmp(cst->symbol, "RelationalExpr") == 0) {
    return parse_binop_chain(cst, "RelationalExpr");
}
if (strcmp(cst->symbol, "EqualityExpr") == 0) {
    return parse_binop_chain(cst, "EqualityExpr");
}
if (strcmp(cst->symbol, "AndExpr") == 0) {
    return parse_binop_chain(cst, "AndExpr");
}
if (strcmp(cst->symbol, "OrExpr") == 0) {
    return parse_binop_chain(cst, "OrExpr");
}
if (strcmp(cst->symbol, "Conditional") == 0) {
  ASTNode* cond = NULL;
  ASTNode* then_branch = NULL;
  ASTNode* else_branch = NULL;

  for (int i = 0; i < cst->num_children; ++i) {
    if (strcmp(cst->children[i]->symbol, "Expr") == 0 && !cond) {
      cond = cst_to_ast(cst->children[i]);
    } else if (strcmp(cst->children[i]->symbol, "Statement") == 0 && !then_branch) {
      then_branch = cst_to_ast(cst->children[i]);
    } else if (strcmp(cst->children[i]->symbol, "ElifOrElse") == 0) {
      else_branch = cst_to_ast(cst->children[i]);
    }
  }
  ASTNode* if_node = create_ast_node(AST_IF, NULL);
  if (cond) add_ast_child(if_node, cond);
  if (then_branch) add_ast_child(if_node, then_branch);
  if (else_branch) add_ast_child(if_node, else_branch);
  return if_node;
}

if (strcmp(cst->symbol, "ElifOrElse") == 0) {
  if (cst->num_children == 0) return NULL;

  if (strcmp(cst->children[0]->symbol, "elif") == 0) {
    ASTNode* cond = cst_to_ast(cst->children[2]);
    ASTNode* then_branch = cst_to_ast(cst->children[4]);
    ASTNode* next = cst_to_ast(cst->children[5]);
    ASTNode* elif_node = create_ast_node(AST_IF, NULL);
    if (cond) add_ast_child(elif_node, cond);
    if (then_branch) add_ast_child(elif_node, then_branch);
    if (next) add_ast_child(elif_node, next);
    return elif_node;
  }
  if (strcmp(cst->children[0]->symbol, "else") == 0) {
    return cst_to_ast(cst->children[1]);
  }
}

if (strcmp(cst->symbol, "WhileLoop") == 0) {
  ASTNode* cond = NULL;
  ASTNode* body = NULL;

  for (int i = 0; i < cst->num_children; ++i) {
    if (strcmp(cst->children[i]->symbol, "Expr") == 0) {
      cond = cst_to_ast(cst->children[i]);
    } else if (strcmp(cst->children[i]->symbol, "Statement") == 0) {
      body = cst_to_ast(cst->children[i]);
    }
  }

  ASTNode* while_node = create_ast_node(AST_WHILE, NULL);
  if (cond) add_ast_child(while_node, cond);
  if (body) add_ast_child(while_node, body);
  return while_node;
}

if (strcmp(cst->symbol, "ForLoop") == 0) {
    ASTNode* for_node = create_ast_node(AST_FOR, NULL);
    ASTNode* var = NULL;
    ASTNode* expr = NULL;
    ASTNode* body = NULL;

    for (int i = 0; i < cst->num_children; ++i) {
        if (strcmp(cst->children[i]->symbol, "IDENTIFIER") == 0) {
            var = create_ast_node(AST_IDENTIFIER, cst->children[i]->token->lexema);
        }
        else if (strcmp(cst->children[i]->symbol, "Expr") == 0) {
            expr = cst_to_ast(cst->children[i]);
        }
        else if (strcmp(cst->children[i]->symbol, "Statement") == 0) {
            body = cst_to_ast(cst->children[i]);
        }
    }

    if (var) add_ast_child(for_node, var);
    if (expr) add_ast_child(for_node, expr);
    if (body) add_ast_child(for_node, body);
    return for_node;
}
 if (strcmp(cst->symbol, "VarDeclList") == 0) {
    ASTNode* list = create_ast_node(AST_VAR_DECL_LIST, NULL);
    for (int i = 0; i < cst->num_children; ++i) {
      if (strcmp(cst->children[i]->symbol, "VarDecl") == 0)
        add_ast_child(list, cst_to_ast(cst->children[i]));
      else if (strcmp(cst->children[i]->symbol, "VarDeclListTail") == 0) {
        CSTNode* tail = cst->children[i];
        while (tail && tail->num_children >= 2) {
          ASTNode* decl = cst_to_ast(tail->children[1]);
          add_ast_child(list, decl);
          tail = tail->num_children > 2 ? tail->children[2] : NULL;
        }
      }
    }
    return list;
  }
if (strcmp(cst->symbol, "LetExpr") == 0) {
    ASTNode* let_node = create_ast_node(AST_LET, NULL);
    ASTNode* decls = NULL;
    ASTNode* body = NULL;
    
    for (int i = 0; i < cst->num_children; ++i) {
        if (strcmp(cst->children[i]->symbol, "VarDeclList") == 0) {
            decls = cst_to_ast(cst->children[i]);
        }
        else if (strcmp(cst->children[i]->symbol, "in") == 0 && i+1 < cst->num_children) {
            body = cst_to_ast(cst->children[i+1]);
            break; // El cuerpo es lo que sigue después de 'in'
        }
    }
    
    if (decls) add_ast_child(let_node, decls);
    if (body) add_ast_child(let_node, body);
    return let_node;
}
if (strcmp(cst->symbol, "VarDecl") == 0) {
  ASTNode* var_decl = create_ast_node(AST_VAR_DECL, NULL);
  ASTNode* var_name = NULL;
  ASTNode* type_ann = NULL;
  ASTNode* assign_op = NULL;
  ASTNode* expr = NULL;

  for (int i = 0; i < cst->num_children; ++i) {
    if (strcmp(cst->children[i]->symbol, "IDENTIFIER") == 0 && !var_name) {
      var_name = create_ast_node(AST_IDENTIFIER, cst->children[i]->token->lexema);
    }
    else if (strcmp(cst->children[i]->symbol, "TypeAnnotation") == 0) {
      type_ann = cst_to_ast(cst->children[i]);
    }
    else if (strcmp(cst->children[i]->symbol, "AssignOp") == 0 && cst->children[i]->num_children > 0) {
      // Puede ser '=' o ':='
      assign_op = create_ast_node(AST_ASSIGN_OP, cst->children[i]->children[0]->symbol);
    }
    else if (strcmp(cst->children[i]->symbol, "Expr") == 0) {
      expr = cst_to_ast(cst->children[i]); // ⚡️ Aquí recoge el UnaryOp o lo que sea
    }
  }

  if (var_name) add_ast_child(var_decl, var_name);
  if (type_ann) add_ast_child(var_decl, type_ann);
  if (assign_op) add_ast_child(var_decl, assign_op);
  if (expr) add_ast_child(var_decl, expr); // ⚡️ CLAVE: Expr nunca debe perderse

  return var_decl;
}


if (strcmp(cst->symbol, "TypeAnnotation") == 0) {
  if (cst->num_children >= 2 && strcmp(cst->children[1]->symbol, "IDENTIFIER") == 0) {
    return create_ast_node(AST_IDENTIFIER, cst->children[1]->token->lexema);
  }
  return NULL;
}

if (strcmp(cst->symbol, "ExprBlock") == 0) {
  for (int i = 0; i < cst->num_children; ++i) {
    if (strcmp(cst->children[i]->symbol, "StatementList") == 0)
      return cst_to_ast(cst->children[i]);
  }
}
if (strcmp(cst->symbol, "NewExpr") == 0) {
  const char* class_name = cst->children[1]->token->lexema;

  ASTNode* new_node = create_ast_node(AST_FUNCTION_CALL, "new");
  ASTNode* class_id = create_ast_node(AST_IDENTIFIER, class_name);
  add_ast_child(new_node, class_id);

  int added_args = 0;
  for (int i = 0; i < cst->num_children; ++i) {
    if (strcmp(cst->children[i]->symbol, "ArgumentListOpt") == 0) {
      if (cst->children[i]->num_children >= 2) {
        CSTNode* arg_list = cst->children[i]->children[1];
        ASTNode* args = create_ast_node(AST_ARGUMENT_LIST, NULL);
        add_argument_list(args, arg_list);
        add_ast_child(new_node, args);
        added_args = 1;
      }
    }
  }

  if (!added_args) {
    ASTNode* args = create_ast_node(AST_ARGUMENT_LIST, NULL);
    add_ast_child(new_node, args);
  }

  return new_node;
}

if (strcmp(cst->symbol, "AssignmentExpr") == 0) {
  if (cst->num_children == 1) {
    return cst_to_ast(cst->children[0]);  // OrExpr completo
  }

  ASTNode* left = cst_to_ast(cst->children[0]);
  CSTNode* tail = cst->children[1];

  if (tail->num_children >= 2 &&
      strcmp(tail->children[0]->symbol, "=") == 0) {
    ASTNode* right = cst_to_ast(tail->children[1]);
    ASTNode* assign = create_ast_node(AST_ASSIGN, "=");
    add_ast_child(assign, left);
    add_ast_child(assign, right);
    return assign;
  }

  return left;  // ⚡️ Aquí devuelve OrExpr entero, no solo un IDENTIFIER
}

 
    // Program o StatementList
    if (strcmp(cst->symbol, "Program") == 0 || strcmp(cst->symbol, "StatementList") == 0) {
        ASTNode* list = create_ast_node(
            strcmp(cst->symbol, "Program") == 0 ? AST_PROGRAM : AST_STATEMENT_LIST, NULL);

        for (int i = 0; i < cst->num_children; ++i) {
            ASTNode* child_ast = cst_to_ast(cst->children[i]);
            if (child_ast) add_ast_child(list, child_ast);
        }

        return list;
    }
    if (strcmp(cst->symbol, "Arguments") == 0) {
  ASTNode* list = create_ast_node(AST_ARGUMENT_LIST, NULL);
  for (int i = 0; i < cst->num_children; ++i) {
    ASTNode* arg = cst_to_ast(cst->children[i]);
    if (arg) add_ast_child(list, arg);
  }
  return list;
}
if (strcmp(cst->symbol, "Expr") == 0) {
  
  if (cst->num_children >= 2 &&
      strcmp(cst->children[0]->symbol, "IDENTIFIER") == 0 &&
      strcmp(cst->children[1]->symbol, "Arguments") == 0) {
    if (strcmp(cst->children[0]->token->lexema, "print") == 0) {
      ASTNode* print_node = create_ast_node(AST_PRINT, "print");
      ASTNode* args = cst_to_ast(cst->children[1]);
      if (args) add_ast_child(print_node, args);
      return print_node;
    }
  }
  // ✅ Baja toda la rama
  for (int i = 0; i < cst->num_children; ++i) {
    ASTNode* result = cst_to_ast(cst->children[i]);
    if (result) return result;
  }
  return NULL;
}


if (strcmp(cst->symbol, "PrintExpr") == 0) {
    ASTNode* print_node = create_ast_node(AST_PRINT, "print");
    
    // Buscar el argumento de print (nodo Expr)
    for (int i = 0; i < cst->num_children; i++) {
        if (strcmp(cst->children[i]->symbol, "Expr") == 0) {
            ASTNode* arg = cst_to_ast(cst->children[i]);
            if (arg) {
                add_ast_child(print_node, arg);
            }
            break;
        }
    }
    return print_node;
}
if (strcmp(cst->symbol, "Vector") == 0) {
  ASTNode* vec = create_ast_node(AST_VECTOR, NULL);

  for (int i = 0; i < cst->num_children; ++i) {
    if (strcmp(cst->children[i]->symbol, "VectorTail") == 0) {
      CSTNode* tail = cst->children[i];

      // Primer Expr
      if (tail->num_children > 0 && strcmp(tail->children[0]->symbol, "Expr") == 0) {
        ASTNode* first_element = cst_to_ast(tail->children[0]);
        if (first_element) add_ast_child(vec, first_element);
      }

      // VectorTailRest
      if (tail->num_children > 1 && strcmp(tail->children[1]->symbol, "VectorTailRest") == 0) {
        CSTNode* rest = tail->children[1];
        if (rest->num_children > 1 && strcmp(rest->children[1]->symbol, "ArgumentList") == 0) {
          add_argument_list(vec, rest->children[1]);
        }
      }
    }
  }
  return vec;
}

if (strcmp(cst->symbol, "VectorTail") == 0) {
  if (cst->num_children == 0) return NULL;

  if (strcmp(cst->children[0]->symbol, "ForExpr") == 0) {
    return cst_to_ast(cst->children[0]);
  } else if (strcmp(cst->children[0]->symbol, "Expr") == 0) {
  ASTNode* list = create_ast_node(AST_ARGUMENT_LIST, NULL);
  ASTNode* expr = cst_to_ast(cst->children[0]);
  if (expr) add_ast_child(list, expr);

  if (cst->num_children > 1) {
    if (strcmp(cst->children[1]->symbol, "VectorTailRest") == 0) {
      CSTNode* rest = cst->children[1];
      if (rest->num_children >= 2) {
        ASTNode* tail = cst_to_ast(rest);
        if (tail) {
          for (int j = 0; j < tail->num_children; ++j) {
            add_ast_child(list, tail->children[j]);
          }
        }
      }
    }
  }
  return list;
}

}

if (strcmp(cst->symbol, "VectorTailRest") == 0) {
  if (cst->num_children == 0) return NULL;

  ASTNode* list = create_ast_node(AST_ARGUMENT_LIST, NULL);
  CSTNode* arglist = cst->children[1];
  add_argument_list(list, arglist);
  return list;
}

if (strcmp(cst->symbol, "ForExpr") == 0) {
  // P.ej., [for (i in 1)]
  ASTNode* for_node = create_ast_node(AST_FOR, NULL);
  ASTNode* var = NULL;
  ASTNode* expr = NULL;

  for (int i = 0; i < cst->num_children; ++i) {
    if (strcmp(cst->children[i]->symbol, "IDENTIFIER") == 0) {
      var = create_ast_node(AST_IDENTIFIER, cst->children[i]->token->lexema);
    } else if (strcmp(cst->children[i]->symbol, "Expr") == 0) {
      expr = cst_to_ast(cst->children[i]);
    }
  }
  if (var) add_ast_child(for_node, var);
  if (expr) add_ast_child(for_node, expr);
  return for_node;
}

// 🚨 Detecta FunctionCall que sea print(x)
if (strcmp(cst->symbol, "FunctionCall") == 0) {
    if (strcmp(cst->children[0]->symbol, "IDENTIFIER") == 0 &&
        strcmp(cst->children[1]->symbol, "Arguments") == 0) {

        if (strcmp(cst->children[0]->token->lexema, "print") == 0) {
            ASTNode* print_node = create_ast_node(AST_PRINT, "print");
            ASTNode* args = cst_to_ast(cst->children[1]);
            if (args) add_ast_child(print_node, args);
            return print_node;
        }
    }
}

if (strcmp(cst->symbol, "Statement") == 0) {
  for (int i = 0; i < cst->num_children; ++i) {
    ASTNode* child_ast = cst_to_ast(cst->children[i]);
    if (child_ast) return child_ast;
  }
}
if (strcmp(cst->symbol, "Factor") == 0) {
  if (cst->num_children == 3 && strcmp(cst->children[0]->symbol, "(") == 0) {
    return cst_to_ast(cst->children[1]);
  }
  if (strcmp(cst->children[0]->symbol, "Atom") == 0) {
    return cst_to_ast(cst->children[0]);
  }
  return cst_to_ast(cst->children[0]);
}
if (strcmp(cst->symbol, "true") == 0 || strcmp(cst->symbol, "false") == 0) {
  return create_ast_node(AST_BOOL, cst->symbol);
}

if (strcmp(cst->symbol, "Function") == 0) {
  ASTNode* func_node = create_ast_node(AST_FUNCTION_DECL, NULL);

  for (int i = 0; i < cst->num_children; ++i) {
    if (strcmp(cst->children[i]->symbol, "IDENTIFIER") == 0) {
      ASTNode* id = create_ast_node(AST_IDENTIFIER, cst->children[i]->token->lexema);
      add_ast_child(func_node, id);
    }
    else if (strcmp(cst->children[i]->symbol, "ParamsList") == 0) {
      ASTNode* params = cst_to_ast(cst->children[i]);
      if (params) add_ast_child(func_node, params);
    }
    else if (strcmp(cst->children[i]->symbol, "ReturnType") == 0) {
      ASTNode* ret_type = cst_to_ast(cst->children[i]);
      if (ret_type) add_ast_child(func_node, ret_type);
    }
    else if (strcmp(cst->children[i]->symbol, "FunctionBody") == 0) {
      ASTNode* body = cst_to_ast(cst->children[i]);
      if (body) add_ast_child(func_node, body);
    }
    else if (strcmp(cst->children[i]->symbol, "ExprBlock") == 0) {
      // Por compatibilidad si usas ExprBlock directamente
      ASTNode* body = cst_to_ast(cst->children[i]);
      if (body) add_ast_child(func_node, body);
    }
  }
  return func_node;
}


if (strcmp(cst->symbol, "Atom") == 0) {
    // Procesar la base (IDENTIFIER, NUMBER, etc.)
    ASTNode* atom_node = cst_to_ast(cst->children[0]);
    
    // Procesar sufijos si existen
    if (cst->num_children > 1) {
        atom_node = parse_atom_suffixes(atom_node, cst->children[1]);
    }
    
    return atom_node;
}

if (strcmp(cst->symbol, "BaseAtom") == 0) {
  // Caso IDENTIFIER simple
  if (strcmp(cst->children[0]->symbol, "IDENTIFIER") == 0) {
    return create_ast_node(AST_IDENTIFIER, cst->children[0]->token->lexema);
  }

  // Caso NUMBER
  if (strcmp(cst->children[0]->symbol, "NUMBER") == 0) {
    return create_ast_node(AST_NUMBER, cst->children[0]->token->lexema);
  }

  // Caso STRING
  if (strcmp(cst->children[0]->symbol, "STRING") == 0) {
    return create_ast_node(AST_STRING, cst->children[0]->token->lexema);
  }

  // Caso 'new' IDENTIFIER '(' ArgumentListOpt ')'
  if (strcmp(cst->children[0]->symbol, "new") == 0) {
    const char* class_name = cst->children[1]->token->lexema;

    ASTNode* new_node = create_ast_node(AST_FUNCTION_CALL, "new");
    ASTNode* class_id = create_ast_node(AST_IDENTIFIER, class_name);
    add_ast_child(new_node, class_id);

    int added_args = 0;
    for (int i = 0; i < cst->num_children; ++i) {
      if (strcmp(cst->children[i]->symbol, "ArgumentListOpt") == 0) {
        if (cst->children[i]->num_children >= 2) {
          // Tiene '(' ArgumentList ')'
          CSTNode* arg_list = cst->children[i]->children[1];
          ASTNode* args = create_ast_node(AST_ARGUMENT_LIST, NULL);
          add_argument_list(args, arg_list);
          add_ast_child(new_node, args);
          added_args = 1;
        }
      }
    }

    // Si no hay argumentos, agrega ArgumentList vacío
    if (!added_args) {
      ASTNode* args = create_ast_node(AST_ARGUMENT_LIST, NULL);
      add_ast_child(new_node, args);
    }

    return new_node;
  }
}


if (strcmp(cst->symbol, "Term") == 0 ||
    strcmp(cst->symbol, "PrimaryExpr") == 0) {
    return cst_to_ast(cst->children[0]);
}


  // Fallback: pasa expr
  for (int i = 0; i < cst->num_children; ++i) {
    ASTNode* child_ast = cst_to_ast(cst->children[i]);
    if (child_ast) return child_ast;
  }

    return NULL;
}

ASTNode* parse_tail(ASTNode* left, CSTNode* tail) {
  if (!tail || tail->num_children == 0 ||
      strcmp(tail->symbol, "ε") == 0 || strcmp(tail->symbol, "╬╡") == 0)
    return left;

  // Si el primer hijo también es ε o ╬╡ → termina aquí
  if (tail->children[0] &&
      (strcmp(tail->children[0]->symbol, "ε") == 0 ||
       strcmp(tail->children[0]->symbol, "╬╡") == 0))
    return left;

  // ⚡️ Extrae el nodo del operador
  CSTNode* op_node = tail->children[0];

  // Baja si es un no terminal que envuelve el terminal real
  while (
     (strcmp(op_node->symbol, "AdditiveOp") == 0 ||
      strcmp(op_node->symbol, "MultiplicativeOp") == 0 ||
      strcmp(op_node->symbol, "RelationalOp") == 0 ||
      strcmp(op_node->symbol, "EqualityOp") == 0 ||
      strcmp(op_node->symbol, "ConcatOp") == 0 ||
      strcmp(op_node->symbol, "PowerOp") == 0)
     && op_node->num_children == 1
  ) {
     op_node = op_node->children[0];
  }

  // Usa el lexema real si existe, sino el símbolo
  const char* op = (op_node->token && op_node->token->lexema)
                      ? op_node->token->lexema
                      : op_node->symbol;

  // ⚡️ Debug temporal
  printf("✅ parse_tail: operador final = %s\n", op);

  // Lado derecho de la expresión
  ASTNode* right = cst_to_ast(tail->children[1]);
  if (!right) return left;

  // Construye el nodo binario
  ASTNode* op_node_ast = create_ast_node(AST_BINOP, op);
  add_ast_child(op_node_ast, left);
  add_ast_child(op_node_ast, right);

  // Encadena si hay más tail
  if (tail->num_children > 2 && tail->children[2]) {
    return parse_tail(op_node_ast, tail->children[2]);
  }

  return op_node_ast;
}

ASTNode* parse_binop_chain(CSTNode* cst, const char* op_name) {
  ASTNode* left = cst_to_ast(cst->children[0]);
  CSTNode* tail = cst->children[1];
  return parse_tail(left, tail);
}
