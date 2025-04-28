# --------------------------------------------------
# Compilador y opciones
# --------------------------------------------------
CC = gcc
CFLAGS = -Wall -g -Iinclude -Iinclude/grammar -Iinclude/parsing -Iinclude/utils

# --------------------------------------------------
# Archivos fuente
# --------------------------------------------------
SRCS = \
    main.c \
    src/grammar/grammar.c \
    src/grammar/symbol.c \
    src/grammar/production.c \
    src/parsing/containerset.c \
    src/parsing/first_follow.c \
    src/parsing/automaton.c \
    src/parsing/state.c \
    src/parsing/item.c \
    src/parsing/slr1_table.c \
    src/parsing/parser.c \
    src/utils/leer_gramatica.c \
    src/parsing/evaluation.c \
    src/parsing/token_mapper.c

# --------------------------------------------------
# Archivos objeto
# --------------------------------------------------
OBJS = $(SRCS:.c=.o)

# --------------------------------------------------
# Nombre del ejecutable
# --------------------------------------------------
TARGET = parser

# --------------------------------------------------
# Reglas
# --------------------------------------------------
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Regla para limpiar archivos objeto y ejecutable
clean:
	rm -f $(OBJS) $(TARGET)

# Limpiar y recompilar
rebuild: clean all
