CXX = clang++
# CXXFLAGS = -O3 -Wall -Wextra -pthread -pedantic-errors # release
CXXFLAGS = -g -O0 -Wall -Wextra -pthread -pedantic-errors # debug

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

SRC := $(wildcard $(SRC_DIR)/*.cpp)
DEPS := $(wildcard $(SRC_DIR)/*.hpp)
OBJ := $(SRC:$(SRC_DIR)%.cpp=$(OBJ_DIR)%.o)

.PHONY all: quickbuild

quickbuild: $(OBJ) $(DEPS)
	$(CXX) $(CXXFLAGS) -o $(BIN_DIR)/quickbuild $(OBJ)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: quickbuild
	./$(BIN_DIR)/quickbuild

clean:
	rm $(wildcard ${OBJ_DIR}/*.o)
	rm $(BIN_DIR)/quickbuild
