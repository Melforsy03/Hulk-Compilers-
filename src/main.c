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
    Grammar* g = (Grammar*)malloc(sizeof(Grammar));
    if (!g) {
        fprintf(stderr, "Error: No se pudo asignar memoria para la gramática\n");
        exit(1);
    }

    init_grammar(g);
    load_grammar_from_file(g, "grammar/grammar.bnf");
    //  print_grammar(g);

    FirstFollowTable* table = malloc(sizeof(FirstFollowTable));
    if (!table) {
        fprintf(stderr, "Error: No se pudo asignar memoria para la tabla\n");
        free(g);
        return EXIT_FAILURE;
    }

    // Cambio clave aquí: pasar los punteros directamente, no sus direcciones
    init_first_follow_table(table, g);
    compute_first(g, table);
    compute_follow(g, table);
    
    //  print_first_follow(table);
    //  fprintf(stderr, "Error: No se pudo asignar memoria para la tabla LL1\n");
    LL1Table *ll1 = malloc(sizeof(LL1Table));
    if (!ll1) {
        fprintf(stderr, "Error: No se pudo asignar memoria para la tabla LL1\n");
        free(table);
        free(g);
        return EXIT_FAILURE;
    }
    // fprintf(stderr, "Error: No se pudo asignar memoria para la tabla LL1\n");
    init_ll1_table(ll1);

    generate_ll1_table(g, table, ll1);
    // print_ll1_table(ll1);
    // ✅ Declaras la variable FILE* f en este bloque
    FILE* f = fopen("entrada.txt", "r");
    if (!f) {
        fprintf(stderr, "Error: no se pudo abrir input.txt\n");
        return EXIT_FAILURE;
    }

    // 2️⃣ Obtener tamaño del archivo
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);

    // 3️⃣ Reservar memoria para el contenido
    char* input = malloc(length + 1);
    if (!input) {
        fprintf(stderr, "Error: no se pudo asignar memoria para la entrada\n");
        fclose(f);
        free(ll1);
        free(table);
        free(g);
        return EXIT_FAILURE;
    }

    // 4️⃣ Leer contenido y cerrar archivo
    size_t read = fread(input, 1, length, f);
    input[read] = '\0';
    fclose(f);

    // 5️⃣ Usar input como antes
    LL1Parser parser;
    init_parser(&parser, input, ll1, g);

    CSTNode* cst = parse(&parser, g->start_symbol);
    // print_cst(cst , 1);
    ASTNode* ast_root = cst_to_ast(cst);

    
    printf("\n=== AST generado ===\n");
    print_ast(ast_root, 3);
    SymbolTable sym_table;
    TypeTable type_table;
    init_symbol_table(&sym_table);
    init_type_table(&type_table);

    printf("=== Chequeo Semántico ===\n");
    ErrorList errors = {.count = 0};
    check_semantics(ast_root, &sym_table, &type_table, NULL ,&errors);
    print_type_table(&type_table);
    print_symbol_table(&sym_table);
    FILE* ir_file = fopen("hulk/programa.ll", "w");
    if (!ir_file) {
        perror("No se pudo abrir programa.ll");
        return 1;
    }
    if (errors.count > 0) {
    fprintf(stderr, "\n❌ Se encontraron errores semánticos:\n");
    for (int i = 0; i < errors.count; ++i) {
        fprintf(stderr, "- %s\n", errors.messages[i]);
        free(errors.messages[i]);  // Libera memoria de cada mensaje
    }

    // Limpieza antes de salir
    free(input);
    free(ll1);
    free(table);
    free(g);
    return EXIT_FAILURE;
}

    ReturnTypeTable typereturn ;
    // ✅ 2) Pasa el FILE* a tu contexto de codegen
    CodeGenContext ctx = { .out = ir_file, .temp_count = 0, .indent = 0 , .sym_table = &sym_table ,.type_table = &type_table ,.return_table=typereturn , .last_call_args = { NULL } };

    // ✅ 3) Genera LLVM IR SOLO al archivo
    generate_code(&ctx, ast_root);

    fclose(ir_file);
    fclose(f);
    free(input);
    free(ll1);
    free(table);
    free(g);
    return 0;
}
