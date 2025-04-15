# Variables
SCRIPT=script.hulk
BUILD_DIR=build
SRC=$(wildcard */*.c) $(wildcard *.c)
OBJ=$(patsubst %.c, $(BUILD_DIR)/%.o, $(SRC))
DEP=$(OBJ:.o=.d)
BIN=$(BUILD_DIR)/hulk

CC=gcc
# -I. para permitir includes como "lexer/lexer.h"
CFLAGS=-Wall -Wextra -std=c99 -MMD -MP -I.

# Comandos multiplataforma
ifeq ($(OS),Windows_NT)
	RM = cmd /C "if exist $(BUILD_DIR) rmdir /S /Q $(BUILD_DIR)"
	MKDIR = cmd /C "if not exist $(subst /,\\,$(dir $@)) mkdir $(subst /,\\,$(dir $@))"
else
	RM = rm -rf $(BUILD_DIR)
	MKDIR = mkdir -p $(dir $@)
endif

.PHONY: all build run clean

# Compilar todo
all: build

# Construir binario
build: $(BIN)

# Enlazar objetos en ejecutable final
$(BIN): $(OBJ)
	@echo "==> Linking..."
	$(CC) $(OBJ) -o $(BIN)

# Compilar .c a .o respetando estructura de carpetas
$(BUILD_DIR)/%.o: %.c
	@echo "==> Compiling $<..."
	@$(MKDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Ejecutar el programa
run: build
	@echo "==> Running..."
	@if not exist $(SCRIPT) echo Error: El archivo '$(SCRIPT)' no existe. & exit /b 1
	@./$(BIN) $(SCRIPT)

# Limpiar todo lo compilado
clean:
	@echo "==> Cleaning..."
	@$(RM)

# Incluir dependencias para recompilar si cambia un .h
-include $(DEP)
