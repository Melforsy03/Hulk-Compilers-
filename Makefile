# Variables
CC = gcc
CFLAGS = -Wall -O2 -Ilexer_gen -Iparser -Iast_nodes
BUILD_DIR = build
LEXER_GEN_DIR = lexer_gen
PARSER_DIR = parser
AST_DIR = ast_nodes
SRC_DIR = src
LEXER_OUTPUT_DIR = lexer
LEXER_FILE = $(LEXER_OUTPUT_DIR)/lexer.c

# Archivos fuente
LEXER_GEN_SRCS = $(wildcard $(LEXER_GEN_DIR)/*.c)
LEXER_GEN_OBJS = $(patsubst $(LEXER_GEN_DIR)/%.c, $(BUILD_DIR)/%.o, $(LEXER_GEN_SRCS))
LEXER_GEN_EXEC = $(BUILD_DIR)/generar_lexer

PARSER_SRCS = $(wildcard $(PARSER_DIR)/*.c)
PARSER_OBJS = $(patsubst $(PARSER_DIR)/%.c, $(BUILD_DIR)/parser_%.o, $(PARSER_SRCS))

AST_SRCS = $(wildcard $(AST_DIR)/*.c)
AST_OBJS = $(patsubst $(AST_DIR)/%.c, $(BUILD_DIR)/ast_%.o, $(AST_SRCS))

MAIN_SRC = $(SRC_DIR)/main.c
MAIN_OBJ = $(BUILD_DIR)/main.o

ALL_OBJS = $(PARSER_OBJS) $(AST_OBJS) $(MAIN_OBJ)
MAIN_EXEC = $(BUILD_DIR)/main

# ========= Targets =========

# Target principal
all: $(MAIN_EXEC)

# Compilar y generar lexer.c
lexer: $(LEXER_GEN_EXEC) | $(LEXER_OUTPUT_DIR)
	./$(LEXER_GEN_EXEC) > $(LEXER_FILE)

# Ejecutable del generador de lexer
$(LEXER_GEN_EXEC): $(LEXER_GEN_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# Compilar .c a .o para lexer_gen/
$(BUILD_DIR)/%.o: $(LEXER_GEN_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compilar .c a .o para parser/
$(BUILD_DIR)/parser_%.o: $(PARSER_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compilar .c a .o para ast_nodes/
$(BUILD_DIR)/ast_%.o: $(AST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compilar main.c
$(BUILD_DIR)/main.o: $(MAIN_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Ejecutable final del proyecto
$(MAIN_EXEC): lexer $(ALL_OBJS)
	$(CC) $(CFLAGS) $(ALL_OBJS) $(LEXER_FILE) -o $@

# Ejecutar el programa
.PHONY: run
run: $(MAIN_EXEC)
	./$(MAIN_EXEC)

# Crear carpetas
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LEXER_OUTPUT_DIR):
	mkdir -p $(LEXER_OUTPUT_DIR)

# Limpiar
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(LEXER_OUTPUT_DIR)
