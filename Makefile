# ========= Configuración general =========
CC = gcc
CFLAGS = -Wall -O2 -Ilexer_gen -Iparser -Iast_nodes -Igrammar -Ilexer -Icodigo_generado -Isemantic_check

BUILD_DIR = build
HULK_DIR = hulk
LEXER_GEN_DIR = lexer_gen
PARSER_DIR = parser
AST_DIR = ast_nodes
GRAMMAR_DIR = grammar
LEXER_DIR = lexer
GEN_DIR = codigo_generado
SEMANTIC_DIR = semantic_check
SRC_DIR = src

LEXER_OUTPUT_DIR = lexer
LEXER_FILE = $(LEXER_OUTPUT_DIR)/lexer.c
LEXER_GEN_EXEC = $(BUILD_DIR)/generar_lexer
LLVM_FILE = $(HULK_DIR)/programa.ll

MAIN_SRC = $(SRC_DIR)/main.c
MAIN_OBJ = $(BUILD_DIR)/main.o
MAIN_EXEC = $(HULK_DIR)/main
EXECUTABLE_LL = $(HULK_DIR)/ejecutable

OS := $(shell uname 2>/dev/null || echo Windows)

# ========= Archivos fuente =========

LEXER_GEN_SRCS = $(wildcard $(LEXER_GEN_DIR)/*.c)
LEXER_GEN_OBJS = $(patsubst $(LEXER_GEN_DIR)/%.c, $(BUILD_DIR)/lexer_gen_%.o, $(LEXER_GEN_SRCS))

PARSER_SRCS = $(wildcard $(PARSER_DIR)/*.c)
PARSER_OBJS = $(patsubst $(PARSER_DIR)/%.c, $(BUILD_DIR)/parser_%.o, $(PARSER_SRCS))

AST_SRCS = $(wildcard $(AST_DIR)/*.c)
AST_OBJS = $(patsubst $(AST_DIR)/%.c, $(BUILD_DIR)/ast_%.o, $(AST_SRCS))

GRAMMAR_SRCS = $(wildcard $(GRAMMAR_DIR)/*.c)
GRAMMAR_OBJS = $(patsubst $(GRAMMAR_DIR)/%.c, $(BUILD_DIR)/grammar_%.o, $(GRAMMAR_SRCS))

LEXER_SRCS = $(wildcard $(LEXER_DIR)/*.c)
LEXER_OBJS = $(patsubst $(LEXER_DIR)/%.c, $(BUILD_DIR)/lexer_%.o, $(LEXER_SRCS))

GEN_SRCS = $(wildcard $(GEN_DIR)/*.c)
GEN_OBJS = $(patsubst $(GEN_DIR)/%.c, $(BUILD_DIR)/gen_%.o, $(GEN_SRCS))

SEMANTIC_SRCS = $(wildcard $(SEMANTIC_DIR)/*.c)
SEMANTIC_OBJS = $(patsubst $(SEMANTIC_DIR)/%.c, $(BUILD_DIR)/semantic_%.o, $(SEMANTIC_SRCS))

ALL_OBJS = $(PARSER_OBJS) $(AST_OBJS) $(GRAMMAR_OBJS) $(LEXER_OBJS) $(GEN_OBJS) $(SEMANTIC_OBJS) $(MAIN_OBJ)

# ========= Tareas principales =========

.PHONY: all compile generate_lexer execute run clean clean-lexer

all: $(MAIN_EXEC)

compile: all

generate_lexer: $(LEXER_GEN_EXEC)
	@echo "Generando lexer..."
	./$(LEXER_GEN_EXEC) > $(LEXER_FILE)
	@echo "Lexer generado en $(LEXER_FILE)"

execute: $(MAIN_EXEC)
	@echo "=== Ejecutando compilador ==="
	./$(MAIN_EXEC)
	@echo "\n--- Código LLVM generado (programa.ll) ---"
	@cat $(LLVM_FILE)
	@echo "\n=== Compilando programa.ll a ejecutable con Clang ==="
	clang $(LLVM_FILE) codigo_generado/runtime.c -o $(EXECUTABLE_LL) -Wl,-subsystem,console
	@echo "\n=== Ejecutando binario generado ==="
	./$(EXECUTABLE_LL)

run: execute

# ========= Reglas de compilación =========

$(BUILD_DIR)/lexer_gen_%.o: $(LEXER_GEN_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(LEXER_GEN_EXEC): $(LEXER_GEN_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/parser_%.o: $(PARSER_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_%.o: $(AST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/grammar_%.o: $(GRAMMAR_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lexer_%.o: $(LEXER_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/gen_%.o: $(GEN_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/semantic_%.o: $(SEMANTIC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main.o: $(MAIN_SRC) | $(BUILD_DIR)
	@if [ ! -f $(LEXER_FILE) ]; then \
		echo "ERROR: Falta lexer.c. Ejecute 'make generate_lexer' primero."; \
		exit 1; \
	fi
	$(CC) $(CFLAGS) -c $< -o $@

$(MAIN_EXEC): $(ALL_OBJS) | $(HULK_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# ========= Crear directorios =========

$(BUILD_DIR) $(HULK_DIR) $(LEXER_OUTPUT_DIR):
ifeq ($(OS),Windows)
	if not exist "$@" mkdir "$@"
else
	mkdir -p $@
endif

# ========= Limpieza =========

clean:
ifeq ($(OS),Windows)
	if exist "$(BUILD_DIR)" rmdir /s /q "$(BUILD_DIR)"
	if exist "$(MAIN_EXEC)" del /f /q "$(MAIN_EXEC)"
	if exist "$(EXECUTABLE_LL)" del /f /q "$(EXECUTABLE_LL)"
	if exist "$(LLVM_FILE)" del /f /q "$(LLVM_FILE)"
else
	rm -rf $(BUILD_DIR)
	rm -f $(MAIN_EXEC) $(EXECUTABLE_LL) $(LLVM_FILE)
endif

clean-lexer:
ifeq ($(OS),Windows)
	if exist "$(LEXER_FILE)" del /f /q "$(LEXER_FILE)"
	if exist "$(LEXER_GEN_EXEC)" del /f /q "$(LEXER_GEN_EXEC)"
else
	rm -f $(LEXER_FILE) $(LEXER_GEN_EXEC)
endif
