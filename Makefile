# ========= Configuración general =========
CC = gcc
CFLAGS = -Wall -O2 -Ilexer_gen -Iparser -Iast_nodes -Igrammar -Ilexer -Icodigo_generado -Isemantic_check

BUILD_DIR = build
LEXER_GEN_DIR = lexer_gen
PARSER_DIR = parser
AST_DIR = ast_nodes
GRAMMAR_DIR = grammar
LEXER_DIR = lexer
GEN_DIR = codigo_generado
SRC_DIR = src
LEXER_OUTPUT_DIR = lexer
LEXER_FILE = $(LEXER_OUTPUT_DIR)/lexer.c 
HULK_DIR = hulk
LLVM_FILE = $(HULK_DIR)/programa.ll
SEMANTIC_DIR = semantic_check

OS := $(shell uname 2>/dev/null || echo Windows)

# ========= Archivos fuente =========
LEXER_GEN_SRCS = $(wildcard $(LEXER_GEN_DIR)/*.c)
LEXER_GEN_OBJS = $(patsubst $(LEXER_GEN_DIR)/%.c, $(BUILD_DIR)/%.o, $(LEXER_GEN_SRCS))
LEXER_GEN_EXEC = $(BUILD_DIR)/generar_lexer # El ejecutable que genera el lexer.c

PARSER_SRCS = $(wildcard $(PARSER_DIR)/*.c)
PARSER_OBJS = $(patsubst $(PARSER_DIR)/%.c, $(BUILD_DIR)/parser_%.o, $(PARSER_SRCS))

AST_SRCS = $(wildcard $(AST_DIR)/*.c)
AST_OBJS = $(patsubst $(AST_DIR)/%.c, $(BUILD_DIR)/ast_%.o, $(AST_SRCS))

GRAMMAR_SRCS = $(wildcard $(GRAMMAR_DIR)/*.c)
GRAMMAR_OBJS = $(patsubst $(GRAMMAR_DIR)/%.c, $(BUILD_DIR)/grammar_%.o, $(GRAMMAR_SRCS))

# Incluir lexer.c generado aquí
LEXER_SRCS = $(LEXER_FILE) $(filter-out $(LEXER_FILE), $(wildcard $(LEXER_DIR)/*.c)) # Asegúrate de que lexer.c esté aquí
LEXER_OBJS = $(patsubst $(LEXER_DIR)/%.c, $(BUILD_DIR)/lexer_%.o, $(LEXER_SRCS))

GEN_SRCS = $(wildcard $(GEN_DIR)/*.c)
GEN_OBJS = $(patsubst $(GEN_DIR)/%.c, $(BUILD_DIR)/gen_%.o, $(GEN_SRCS))

SEMANTIC_SRCS = $(wildcard $(SEMANTIC_DIR)/*.c)
SEMANTIC_OBJS = $(patsubst $(SEMANTIC_DIR)/%.c, $(BUILD_DIR)/semantic_%.o, $(SEMANTIC_SRCS))

MAIN_SRC = $(SRC_DIR)/main.c
MAIN_OBJ = $(BUILD_DIR)/main.o

ALL_OBJS = $(PARSER_OBJS) $(AST_OBJS) $(GRAMMAR_OBJS) $(LEXER_OBJS) $(GEN_OBJS) $(SEMANTIC_OBJS) $(MAIN_OBJ)
MAIN_EXEC = $(HULK_DIR)/main

# ========= Tareas principales =========

.PHONY: all
all: $(MAIN_EXEC)

.PHONY: compile
compile: $(MAIN_EXEC)

# Tarea para generar explícitamente el lexer.c
.PHONY: generate_lexer
generate_lexer: $(LEXER_FILE)

# Regla para crear lexer.c si no existe o si generar_lexer ha cambiado
$(LEXER_FILE): $(LEXER_GEN_EXEC) | $(LEXER_OUTPUT_DIR)
	@echo "Generando lexer..."
	./$(LEXER_GEN_EXEC) > $@
	@echo "Lexer generado en $@"

.PHONY: execute
execute: $(MAIN_EXEC)
	./$(MAIN_EXEC)
	@echo "\n--- Código LLVM generado (programa.ll) ---"
	@cat $(LLVM_FILE)

.PHONY: run
run: $(MAIN_EXEC)
	./$(MAIN_EXEC)

# ========= Reglas de compilación =========

$(BUILD_DIR)/%.o: $(LEXER_GEN_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(LEXER_GEN_EXEC): $(LEXER_GEN_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/parser_%.o: $(PARSER_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_%.o: $(AST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/grammar_%.o: $(GRAMMAR_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Regla explícita para compilar el lexer.c generado
$(BUILD_DIR)/lexer_lexer.o: $(LEXER_FILE) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Regla genérica para otros archivos en LEXER_DIR (si los hay)
$(BUILD_DIR)/lexer_%.o: $(LEXER_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/gen_%.o: $(GEN_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/semantic_%.o: $(SEMANTIC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main.o: $(MAIN_SRC) | $(BUILD_DIR) $(LEXER_FILE) # main.o debe depender del lexer generado
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

.PHONY: clean
clean:
ifeq ($(OS),Windows)
	if exist "$(BUILD_DIR)" rmdir /s /q "$(BUILD_DIR)"
	if exist "$(HULK_DIR)\main" del /f /q "$(HULK_DIR)\main"
	if exist "$(HULK_DIR)\programa.ll" del /f /q "$(HULK_DIR)\programa.ll"
else
	rm -rf $(BUILD_DIR)
	rm -f $(HULK_DIR)/main $(HULK_DIR)/programa.ll
endif

.PHONY: clean-lexer
clean-lexer:
ifeq ($(OS),Windows)
	if exist "$(LEXER_FILE)" del /f /q "$(LEXER_FILE)"
	if exist "$(LEXER_GEN_EXEC)" del /f /q "$(LEXER_GEN_EXEC)"
else
	rm -f $(LEXER_FILE) $(LEXER_GEN_EXEC)
endif