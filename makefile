C = clang
CFLAGS = -g -o0 -Wall -Wextra -pthread -pedantic-errors

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

SRC := $(wildcard $(SRC_DIR)/*.c)
DEPS := $(wildcard $(SRC_DIR)/*.h)
OBJ := $(SRC:$(SRC_DIR)%.c=$(OBJ_DIR)%.o)

.PHONY all: quickbuild

quickbuild: $(OBJ) $(DEPS)
	$(C) $(CXXFLAGS) -o $(BIN_DIR)/quickbuild $(OBJ)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(C) $(CXXFLAGS) -c $< -o $@

run: quickbuild
	./$(BIN_DIR)/quickbuild

clean:
	rm $(wildcard ${obj_dir}/*.o)
	rm quickbuild
