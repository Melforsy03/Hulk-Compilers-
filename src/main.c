#include "grammar.h"
#include "first_follow.h"
#include "ll1_table.h"
#include "ll1_parser.h"
#include "ast.h"
#include "cst.h"
#include "semantic.h"
#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
void print_type_table(TypeTable* table) {
  printf("=== Type Table ===\n");
  TypeEntry* t = table->head;
  while (t) {
    printf("Type: %s\n", t->name);
    printf("  Bases: ");
    for (int i = 0; i < t->num_bases; i++) {
      printf("%s ", t->bases[i]);
    }
    printf("\n  Members:\n");
    Member* m = t->members;
    while (m) {
      printf("    %s : %s\n", m->name, m->type);
      m = m->next;
    }
    t = t->next;
  }
  printf("==================\n");
}
void print_symbol_table(SymbolTable* table) {
  printf("=== Symbol Table ===\n");
  Symbol* s = table->head;
  while (s) {
    printf("Symbol: %s\n", s->name);
    printf("  Type: %s\n", s->type);
    printf("  Kind: %s\n", s->kind == SYMBOL_VARIABLE ? "Variable"
                 : s->kind == SYMBOL_FUNCTION ? "Function"
                 : "Type");
    s = s->next;
  }
  printf("=====================\n");
}

int main() {
    // ---- Inicializar gramática ----
    Grammar* g = malloc(sizeof(Grammar));
    if (!g) {
        fprintf(stderr, "Error: No se pudo asignar memoria para la gramática\n");
        return EXIT_FAILURE;
    }

    init_grammar(g);
    load_grammar_from_file(g, "grammar/grammar.bnf");

    // ---- Tabla FirstFollow ----
    FirstFollowTable* table = malloc(sizeof(FirstFollowTable));
    if (!table) {
        fprintf(stderr, "Error: No se pudo asignar memoria para la tabla\n");
        free(g);
        return EXIT_FAILURE;
    }

    init_first_follow_table(table, g);
    compute_first(g, table);
    compute_follow(g, table);

    // ---- Tabla LL1 ----
    LL1Table* ll1 = malloc(sizeof(LL1Table));
    if (!ll1) {
        fprintf(stderr, "Error: No se pudo asignar memoria para la tabla LL1\n");
        free(table);
        free(g);
        return EXIT_FAILURE;
    }

    init_ll1_table(ll1);
    generate_ll1_table(g, table, ll1);

    // ---- Leer archivo de entrada ----
    FILE* f = fopen("entrada.txt", "r");
    if (!f) {
        fprintf(stderr, "Error: no se pudo abrir entrada.txt\n");
        free(ll1);
        free(table);
        free(g);
        return EXIT_FAILURE;
    }

    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);

    char* input = malloc(length + 1);
    if (!input) {
        fprintf(stderr, "Error: no se pudo asignar memoria para la entrada\n");
        fclose(f);
        free(ll1);
        free(table);
        free(g);
        return EXIT_FAILURE;
    }

    size_t read = fread(input, 1, length, f);
    input[read] = '\0';
    fclose(f);  // ✅ Cerrar después de leer

    // ---- Parsear ----
    LL1Parser parser;
    init_parser(&parser, input, ll1, g);

    CSTNode* cst = parse(&parser, g->start_symbol);
    ASTNode* ast_root = cst_to_ast(cst);
    //  print_cst(cst , 1);
    printf("\n=== AST generado ===\n");
    print_ast(ast_root, 3);

    // ---- Semántica ----
    SymbolTable sym_table;
    init_symbol_table(&sym_table);
    TypeTable type_table;
    init_type_table(&type_table);

    ErrorList errors = { .count = 0 };
    check_semantics(ast_root, &sym_table, &type_table, NULL, &errors);

    printf("\n=== Tabla de Tipos ===\n");
    print_type_table(&type_table);
    printf("\n=== Tabla de Símbolos ===\n");
    print_symbol_table(&sym_table);

    // ---- Verificar errores semánticos ----
    if (errors.count > 0) {
        fprintf(stderr, "\n❌ Se encontraron errores semánticos:\n");
        for (int i = 0; i < errors.count; ++i) {
            printf("- Línea %d Columna %d: %s\n",
                   errors.errors[i].line,
                   errors.errors[i].column,
                   errors.errors[i].message);
            free(errors.errors[i].message); // Libera cada mensaje
        }

        // Limpieza
        free(input);
        free(ll1);
        free(table);
        free(g);
        return EXIT_FAILURE;
    }

    // ---- LLVM IR ----
    FILE* ir_file = fopen("hulk/programa.ll", "w");
    if (!ir_file) {
        perror("No se pudo abrir programa.ll");
        free(input);
        free(ll1);
        free(table);
        free(g);
        return EXIT_FAILURE;
    }

    ReturnTypeTable typereturn;
    CodeGenContext ctx = {
        .out = ir_file,
        .temp_count = 0,
        .indent = 0,
        .sym_table = &sym_table,
        .type_table = &type_table,
        .return_table = typereturn,
        .last_call_args = { NULL }
    };

    generate_code(&ctx, ast_root);

    fclose(ir_file);

    // ---- Limpieza ----
    free(input);
    free(ll1);
    free(table);
    free(g);

    return 0;
}
