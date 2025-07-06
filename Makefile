# ============================================
# ⚡ CONFIGURACIÓN GENERAL
# ============================================

CC = gcc
CFLAGS = -Wall -Wextra -O2
INCLUDES = -Igrammar -Ilexer -Iparser -Isemantic_check -Isrc -Ilexer_gen -Icodigo_generado

LLVM_AS = llvm-as
LLVM_CC = clang

SRC_DIRS = src lexer parser semantic_check grammar codigo_generado
BUILD_DIR = build
OUTPUT_DIR = hulk

COMPILER_BIN = $(BUILD_DIR)/mi_compilador.exe

LLVM_IR = $(OUTPUT_DIR)/programa.ll
BC = $(OUTPUT_DIR)/programa.bc
EXE = $(OUTPUT_DIR)/codigo_generado.exe

LEXER_C = lexer/lexer.c  # Archivo generado por tu regla de lexer

# Encuentra todos los .c dentro de todas las carpetas fuente
SRCS = $(shell find $(SRC_DIRS) -name "*.c")
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)

# ============================================
# ⚡ FLUJO PRINCIPAL
# ============================================

# ✅ Compila todo: solo genera lexer si no existe
all: $(LEXER_C) $(EXE)

# ✅ Fuerza generación del lexer (manual)
lexer:
	@echo "✅ Forzando generación de lexer..."
	./lexer_gen  # <-- Tu generador real aquí

# ✅ Genera lexer solo si no existe
$(LEXER_C):
	@echo "✅ Generando lexer porque no existe..."
	./lexer_gen

# ✅ Objeto del lexer depende del archivo generado
$(BUILD_DIR)/lexer/%.o: lexer/%.c | $(BUILD_DIR) $(LEXER_C)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ✅ Compila tu compilador
$(COMPILER_BIN): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o $@

# ✅ Ejecuta tu compilador para generar IR (NO redirige stdout)
$(LLVM_IR): $(COMPILER_BIN) | $(OUTPUT_DIR)
	$(COMPILER_BIN) < entrada.txt

# ✅ LLVM IR → Bitcode
$(BC): $(LLVM_IR)
	$(LLVM_AS) $(LLVM_IR) -o $(BC)

# ✅ Bitcode → Ejecutable final
$(EXE): $(BC)
	$(LLVM_CC) $(BC) -o $(EXE)

# ✅ Nueva regla: ejecuta el programa generado
run: $(EXE)
	@echo "✅ Ejecutando ./$(EXE)..."
	./$(EXE)

# ============================================
# ⚡ Carpeta de build y salida
# ============================================

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

# ============================================
# ⚡ Regla genérica para todos los objetos
# ============================================

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ============================================
# ⚡ Limpieza
# ============================================

clean:
	rm -rf $(BUILD_DIR) $(OUTPUT_DIR)
	@echo "✅ Limpiado build/ y hulk/, lexer generado se mantiene."

clean-lexer:
	rm -f $(LEXER_C)
	@echo "✅ Lexer generado eliminado."

# ============================================
# ⚡ Mostrar info para debug
# ============================================

show:
	@echo "SRCS: $(SRCS)"
	@echo "OBJS: $(OBJS)"
	@echo "COMPILER_BIN: $(COMPILER_BIN)"
	@echo "LLVM_IR: $(LLVM_IR)"
	@echo "BC: $(BC)"
	@echo "EXE: $(EXE)"
