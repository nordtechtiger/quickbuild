# Quickbuild
> [!WARNING]
> Quickbuild is currently in early beta and is undergoing major changes. Expect frequent crashes, segmentation faults, and undefined behaviour. Please see [the roadmap](https://github.com/nordtechtiger/quickbuild/issues/1) for more information.

## "What is this?"

Tired of looking up what `$<` and `$@` does? Say hello to `[depends]` and `[obj]`. Your build system shouldn't get in the way of your next big idea. Quickbuild is an intuitive way of writing simple build scripts for your projects without having to worry about googling the Make syntax anymore.

## Features
- 🌱 Intuitive & pain-free setup
- ⚙️ Make-style configuration
- 🪶 Lightweight & performant

Quickbuild trades a slightly more verbose configuration with intuitive and simpler syntax that takes the pain out of writing makefiles. This makes it suitable for small to medium sized projects and for those who are starting out with low level development. However, this is *not* a replacement for Make - Quickbuild does not have, and will never achieve, feature parity with other build systems such as Make or CMake.

## Installation
There are currently no official packages of Quickbuild available, however, support for Arch is planned. In the meantime, the only supported way of running Quickbuild is by building from source.

> Dependencies:
> - make _or_ quickbuild
> - clang >= 18

```
$ git clone https://github.com/nordtechtiger/quickbuild
$ cd quickbuild
$ make
```

You should then copy or symlink the resultant binary (./bin/quickbuild) in order to use it from your terminal.

## Syntax
All configuration is to be stored at project root in a file named "quickbuild". Here's the config used to compile (and boostrap!) Quickbuild:
```
# General compiler arguments
compiler = "clang++";
flags = "-g -O0 -Wall -Wextra -pthread -pedantic-errors";

# Files to compile
sources = "src/*.cpp";
headers = "src/*.hpp";

# Files to create
objects = sources: "src/*.cpp" -> "obj/*.o";
binary = "./bin/quickbuild";

# Main target
"quickbuild" {
  depends = objects, headers;
  run = "[compiler] [flags] [objects] -o [binary]";
}

# Object files
objects as obj {
  depends = obj: "obj/*.o" -> "src/*.cpp";
  run = "[compiler] [flags] -c [depends] -o [obj]";
}

# Run
"run" {
  depends = "quickbuild";
  run = "[binary]";
}

# Clean
"clean" {
  run = "rm [objects]",
        "rm [binary]";
}
```
