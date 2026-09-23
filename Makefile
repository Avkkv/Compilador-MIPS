CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic
CPPFLAGS = -Iinclude
BUILD_DIR = build
BIN_DIR = bin
TARGET = $(BIN_DIR)/main

SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRC))

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILD_DIR)/%.o: src/%.c include/*.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(TARGET): $(OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(OBJ) -o $@

run: $(TARGET)
	@if [ -z "$(INPUT)" ] || [ -z "$(OUTPUT_PRE)" ] || [ -z "$(OUTPUT_LEX)" ]; then \
		echo "Uso: make run INPUT=arquivo.asm OUTPUT_PRE=saida.pre OUTPUT_LEX=saida.lex"; \
		exit 1; \
	fi
	./$(TARGET) "$(INPUT)" "$(OUTPUT_PRE)" "$(OUTPUT_LEX)"

test: $(TARGET)
	./$(TARGET) tests/entradas/correto1.asm tests/saidas/correto1.pre tests/saidas/correto1.lex

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	rm -f tests/saidas/*.pre tests/saidas/*.lex tests/saidas/*.ts tests/saidas/*.err

.PHONY: all run test clean
