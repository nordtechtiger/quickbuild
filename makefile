# General compiler arguments
CXX = clang++
CXXFLAGS = -g -O0 -Wall -Wextra -pthread -pedantic-errors

# Files to compile
sources := $(wildcard src/*.cpp)
headers := $(wildcard src/*.hpp)

# Files to create
objects := $(sources:src/%.cpp=obj/%.o)
binary := ./bin/quickbuild

# Main target
quickbuild: setup $(objects) $(headers)
	$(CXX) $(CXXFLAGS) -o $(binary) $(objects)

# Object files
obj/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Setup the build directories
setup:
	mkdir -p bin obj

# Run
run: quickbuild
	$(binary)

# Clean
clean:
	rm $(objects)
	rm $(binary)
