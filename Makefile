# Variables
SCRIPT=script.hulk
BUILD_DIR=build

.PHONY: all build run

all: build

build:
	@echo "==> Building project..."
	@if [ ! -f "$(SCRIPT)" ]; then \
		echo "Error: El archivo '$(SCRIPT)' no existe."; \
		exit 1; \
	fi
	@mkdir -p $(BUILD_DIR)
	@echo "Compilando $(SCRIPT) en $(BUILD_DIR)..."
	# Aquí iría la lógica de compilación real

run: build
	@echo "==> Running project..."
	# Aquí iría la ejecución (placeholder)
