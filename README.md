# Quickbuild
> [!WARNING]
> Quickbuild is currently in early beta and is undergoing major changes. Expect frequent crashes, segmentation faults, and undefined behaviour. Please see [the roadmap](https://github.com/nordtechtiger/quickbuild/issues/1) for more information.

## "What is this?"

Tired of looking up what `$<` and `$@` does? Say hello to `[depends]` and `[whatever-you-fancy]`. Your build system shouldn't get in the way of your next big idea. Quickbuild is an intuitive way of writing simple build scripts for your projects without having to worry about googling the Make syntax anymore.

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

Normal installation using Make:
```
$ git clone https://github.com/nordtechtiger/quickbuild
$ cd quickbuild
$ make
# make install
```

Alternatively, you can use a previous version of Quickbuild to build a newer one from source:
```
$ git clone https://github.com/nordtechtiger/quickbuild
$ cd quickbuild
$ quickbuild
# quickbuild install
```

## Syntax
All configuration is to be stored at project root in a file named "quickbuild". The structure of a Quickbuild config is very similar to that of a Makefile, but with slightly more verbose syntax.


### Variables
Variables are declared using the following syntax.
```
# this is a comment!
my_variable = "foo-bar";
another-var = "buz";
```

If an asterisk (wildcard) is present in a string, it is automatically expended into all matching filepaths.
```
my_source_files = "src/*.cpp"; # expands into "src/foo.cpp", "src/bar.cpp", ...
my_header_files = "src/*.hpp";
```

There is also an in-built operator for a simple search-and-replace.
```
sources = "src/thing.cpp", "src/another.cpp";
objects = sources: "src/*.cpp" -> "obj/*.o"; # expands into "obj/thing.o", "obj/another.p"
```

String interpolation is also implemented, and requires the use of brackets.
```
foo = "World";
bar = "Hello, [foo]!";
```

### Targets
Every target has to have a name and a `run` field. Targets can be declared as follows.
```
# this will always execute when quickbuild is run
"my-project" {
    run = "gcc foo.c";
}
```

If you don't want a target to execute a command, you can leave the `run` field blank.
```
"a-phony-target" {
    depends = some-other-stuff;
    run = "";
}
```

Dependencies can be specified using the `depends` field.
```
# this will only execute if the dependencies have changed
"another-project" {
    depends = "foo.c", "bar.c";
    run = "gcc foo.c bar.c";
}
```

Targets can also be specified from variables.
```
my_deps = "foo.c";

"my_main_project" {
    depends = my_deps;
    run = "./my_output_binary"
}

my_deps {
    run = "gcc [my_deps]";
}
```

Finally, for targets that can apply to multiple values, iterators can be used. This is essentially the equivalent to $@ in Make.
```
my_files = "a.c", "b.c", "c.c";

"project" {
    depends = my_files;
    run = "./output";
}

my_files as current_source_file {
    run = "echo Hello, [current_source_file]";
}

# this would print:
# Hello, a.c
# Hello, b.c
# Hello, c.c
```

### Example
All of these features are usually combined to create more powerful build scripts. Check out a comprehensive config that can compile (and boostrap!) Quickbuild:
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
