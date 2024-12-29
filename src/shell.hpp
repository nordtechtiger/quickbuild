#ifndef SHELL_H
#define SHELL_H
#include <string>

struct ShellResult {
  std::string stdout;
  int status;
};

// Cross-platform class for executing shell commands
class Shell {
public:
  static ShellResult execute(std::string cmdline);
};

class ShellException : std::exception {
private:
  const char *details;

public:
  ShellException(const char *details) : details(details) {};
  const char *what() { return details; }
};

#endif
