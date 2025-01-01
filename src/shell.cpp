#include "shell.hpp"

#define __SHELL_SUFFIX " 2>&1"

// Executes a shell command and returns the output.
#ifdef __GNUC__
#include <unistd.h>
ShellResult Shell::execute(std::string cmdline) {
  // Setup pipes
  char buffer[128];
  std::string stdout;

  FILE *process = popen((cmdline + __SHELL_SUFFIX).c_str(), "r");
  if (!process)
    throw ShellException("Unable to execute command");

  while (fgets(buffer, sizeof(buffer), process) != NULL) {
    stdout += buffer;
  }

  int status = pclose(process);
  return {
      stdout,
      status,
  };
}
#elif __WIN32
std::string Shell::execute(std::string cmdline) {
  // TODO: IMPLEMENT THIS
  throw ShellException("Win32 not supported yet");
}
#else
#error Unsupported platform: Only Gnu and Win32 are supported
#endif
