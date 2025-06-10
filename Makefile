# Carpetas
LEXER_GEN_DIR = lexer_gen
LEXER_DIR = lexer
SRC_DIR = src

# Archivos fuente
GEN_LEXER_SRC = $(LEXER_GEN_DIR)/generar_lexer.c
GEN_COMMON_SRC = \
	$(LEXER_GEN_DIR)/utils.c \
	$(LEXER_GEN_DIR)/regex_parser.c \
	$(LEXER_GEN_DIR)/regex_to_dfa.c \
	$(LEXER_GEN_DIR)/nfa_to_dfa.c

LEXER_SRC = $(LEXER_DIR)/lexer.c
MAIN_SRC = $(SRC_DIR)/main.c

# Ejecutable
EXEC = compilador

# Compilador y flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I. -I$(LEXER_DIR) -I$(LEXER_GEN_DIR)

# Regla principal
all: $(LEXER_SRC) $(EXEC)

# Generar lexer.c desde generar_lexer
$(LEXER_SRC): $(GEN_LEXER_SRC) $(LEXER_GEN_DIR)/tokens.def
	@echo "🔧 Generando lexer.c..."
	$(CC) $(CFLAGS) -o generar_lexer $(GEN_LEXER_SRC) $(GEN_COMMON_SRC)
	./generar_lexer
	mv lexer.c $(LEXER_DIR)/lexer.c
	rm -f generar_lexer

# Compilar ejecutable
$(EXEC): $(LEXER_SRC) $(MAIN_SRC) $(GEN_COMMON_SRC)
	@echo "🔨 Compilando ejecutable..."
	$(CC) $(CFLAGS) -o $(EXEC) $(LEXER_SRC) $(MAIN_SRC) $(GEN_COMMON_SRC)

# Limpiar todo
clean:
	rm -f $(EXEC) generar_lexer
	rm -f $(LEXER_SRC)

.PHONY: all clean
