# Makefile actualizado para compilación segura en 64 bits

# Variables
SCRIPT=script.hulk
BUILD_DIR=build
SRC=$(wildcard */*.c) $(wildcard *.c)
OBJ=$(patsubst %.c, $(BUILD_DIR)/%.o, $(SRC))
DEP=$(OBJ:.o=.d)
BIN=$(BUILD_DIR)/hulk

CC=gcc
ARCH_FLAGS=-m64 
CFLAGS = -Wall -Wextra -std=c99 -MMD -MP -I. -m64

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
	$(CC) $(ARCH_FLAGS) $(OBJ) -o $(BIN)

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


# Generar programa.ll en texto plano limpio
programa.ll: build/hulk.exe
	build\\hulk.exe > temp_programa.ll
	type temp_programa.ll > programa.ll
	del temp_programa.ll

# Compilar runtime.c a objeto
runtime.o: runtime.c
	gcc -c runtime.c -o runtime.o

# Compilar programa.ll a objeto
programa.o: programa.ll
	clang -c programa.ll -o programa.o

# Enlazar programa.o + runtime.o en un ejecutable
ejecutable.exe: programa.o runtime.o
	gcc programa.o runtime.o -o ejecutable.exe

# Flujo completo de generación, enlace y ejecución

run: build/hulk.exe
	build\\hulk.exe
#Para cuando ya este listo el parser 
#programa.ll programa.o runtime.o ejecutable.exe
#./ejecutable.exe

# Limpiar archivos de esta parte
clean-runtime:
	del /Q programa.ll temp_programa.ll programa.o runtime.o ejecutable.exe

# Limpiar TODO
clean-all: clean-runtime
	del /Q build\\hulk.exe