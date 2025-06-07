# Carpetas
LEXER_GEN_DIR = lexer_gen
SRC_DIR = src
LEXER_DIR = lexer
BUILD_DIR = build

# Archivos fuente
LEXER_GEN_SRC = $(LEXER_GEN_DIR)/generar_lexer.c \
                $(LEXER_GEN_DIR)/regex_to_dfa.c \
                $(LEXER_GEN_DIR)/nfa_to_dfa.c \
				$(LEXER_GEN_DIR)/regex_parser.c \
                $(LEXER_GEN_DIR)/utils.c

SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
SRC_FILES += $(LEXER_DIR)/lexer.c  # ✅ Incluir lexer.c

# Archivos objeto en build/
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(filter $(SRC_DIR)/%.c, $(SRC_FILES)))
OBJ_FILES += $(BUILD_DIR)/lexer.o  # ✅ Incluir lexer.o explícitamente

# Ejecutables
GENERATOR_EXE = generar_lexer.exe
FINAL_EXE = compilador.exe

# Compilador
CC = gcc
CFLAGS = -Wall -O2 -mconsole

.PHONY: all lexer run clean lexer-clean

# Compilar todo
all: lexer $(FINAL_EXE)

# Ejecutable del compilador principal
$(FINAL_EXE): $(OBJ_FILES) 
	$(CC) $(CFLAGS) $^ -o $@

# Compilar cada .c de src/ a .o dentro de build/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compilar lexer.c a lexer.o
$(BUILD_DIR)/lexer.o: $(LEXER_DIR)/lexer.c
	if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Generar lexer.c y guardarlo en /lexer
lexer: $(GENERATOR_EXE)
	if not exist $(LEXER_DIR) mkdir $(LEXER_DIR)
	./$(GENERATOR_EXE)
	move lexer.c $(LEXER_DIR)\lexer.c
	@echo ✅ Lexer generado en $(LEXER_DIR)\lexer.c

# Compilar generador del lexer con main exclusivo
$(GENERATOR_EXE): $(LEXER_GEN_SRC)
	$(CC) $(CFLAGS) -DGENERAR_LEXER_MAIN -o $@ $^

# Eliminar el lexer generado explícitamente
lexer-clean:
	@echo 🧹 Eliminando lexer generado...
	-del /q $(LEXER_DIR)\lexer.c 2>nul
	@echo ✅ Lexer eliminado.

# Ejecutar el compilador principal
run: $(FINAL_EXE)
	./$(FINAL_EXE) archivo_entrada.txt


# Limpiar binarios y objetos pero conservar lexer.c
clean:
	@echo 🧹 Limpiando binarios y objetos...
	-del /s /q *.exe 2>nul
	-del /s /q $(BUILD_DIR)\*.o 2>nul
	-rd /s /q $(BUILD_DIR) 2>nul
	@echo ✅ Limpieza completada (lexer.c preservado)
