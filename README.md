# Quickbuild
A simple, lightweight, and fast build system for C, C++, and more.

## Syntax
All configuration is to be stored at project root in a file named "quickbuild".
```
source_dir = "src";
build_dir = "bin";

sources = "[source_dir]/*.c";
headers = "[source_dir]/*.h";
objects = [
    sources: "[source_dir]/*.c" -> "[build_dir]/*.o"
];

fluent {
    depends = [objects], [headers];
    run = "gcc -c [objects] -o [build_dir]/fluent";
};

objects as obj {
    depends = [obj: "[source_dir]/*.o" -> "[build_dir]/*.c"];
    run = "gcc -c [depends] -o [obj]";
};
```
