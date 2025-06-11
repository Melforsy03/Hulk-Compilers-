# ========= Configuración =========
CC = gcc
CFLAGS = -Wall -O2 -Ilexer_gen -Iparser -Iast_nodes -Igrammar -Ilexer

BUILD_DIR = build
LEXER_GEN_DIR = lexer_gen
PARSER_DIR = parser
AST_DIR = ast_nodes
GRAMMAR_DIR = grammar
LEXER_DIR = lexer
SRC_DIR = src
LEXER_OUTPUT_DIR = lexer
LEXER_FILE = $(LEXER_OUTPUT_DIR)/lexer.c

# ========= Archivos fuente y objetos =========
LEXER_GEN_SRCS = $(wildcard $(LEXER_GEN_DIR)/*.c)
LEXER_GEN_OBJS = $(patsubst $(LEXER_GEN_DIR)/%.c, $(BUILD_DIR)/%.o, $(LEXER_GEN_SRCS))
LEXER_GEN_EXEC = $(BUILD_DIR)/generar_lexer

PARSER_SRCS = $(wildcard $(PARSER_DIR)/*.c)
PARSER_OBJS = $(patsubst $(PARSER_DIR)/%.c, $(BUILD_DIR)/parser_%.o, $(PARSER_SRCS))

AST_SRCS = $(wildcard $(AST_DIR)/*.c)
AST_OBJS = $(patsubst $(AST_DIR)/%.c, $(BUILD_DIR)/ast_%.o, $(AST_SRCS))

GRAMMAR_SRCS = $(wildcard $(GRAMMAR_DIR)/*.c)
GRAMMAR_OBJS = $(patsubst $(GRAMMAR_DIR)/%.c, $(BUILD_DIR)/grammar_%.o, $(GRAMMAR_SRCS))

LEXER_SRCS = $(wildcard $(LEXER_DIR)/*.c)
LEXER_OBJS = $(patsubst $(LEXER_DIR)/%.c, $(BUILD_DIR)/lexer_%.o, $(LEXER_SRCS))

MAIN_SRC = $(SRC_DIR)/main.c
MAIN_OBJ = $(BUILD_DIR)/main.o

ALL_OBJS = $(PARSER_OBJS) $(AST_OBJS) $(GRAMMAR_OBJS) $(LEXER_OBJS) $(MAIN_OBJ)
MAIN_EXEC = $(BUILD_DIR)/main

# ========= Targets =========

# Compilar todo
all: $(MAIN_EXEC)

# Generar lexer.c
lexer: $(LEXER_GEN_EXEC) | $(LEXER_OUTPUT_DIR)
	./$(LEXER_GEN_EXEC) > $(LEXER_FILE)

# Ejecutar el programa
.PHONY: run
run: $(MAIN_EXEC)
	./$(MAIN_EXEC)

# ========= Reglas de compilación =========

# lexer_gen
$(BUILD_DIR)/%.o: $(LEXER_GEN_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(LEXER_GEN_EXEC): $(LEXER_GEN_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# parser
$(BUILD_DIR)/parser_%.o: $(PARSER_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ast_nodes
$(BUILD_DIR)/ast_%.o: $(AST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# grammar
$(BUILD_DIR)/grammar_%.o: $(GRAMMAR_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# lexer (func_aux_lexer.c y lexer.c generado)
$(BUILD_DIR)/lexer_%.o: $(LEXER_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# main
$(BUILD_DIR)/main.o: $(MAIN_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Enlazar todo (NO recompilar lexer.c manualmente)
$(MAIN_EXEC): lexer $(ALL_OBJS)
	$(CC) $(CFLAGS) $(ALL_OBJS) -o $@

# ========= Utilidades =========

# Crear carpetas si no existen
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LEXER_OUTPUT_DIR):
	mkdir -p $(LEXER_OUTPUT_DIR)

# Limpiar todo menos lexer/
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# Limpiar solo lexer.c
.PHONY: clean-lexer
clean-lexer:
	rm -f $(LEXER_FILE)
