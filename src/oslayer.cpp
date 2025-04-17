#include "oslayer.hpp"
#include <iostream>

OSLayer::OSLayer(bool parallel, bool silent) {
  this->parallel = parallel;
  this->silent = silent;
}

void OSLayer::queue_command(std::string command) { queue.push_back(command); }

bool OSLayer::execute_queue() {
  return false; // debug.
  if (parallel)
    return _execute_queue_parallel();
  else
    return _execute_queue_sync();
}

bool OSLayer::_execute_queue_parallel() {
  std::vector<std::thread> pool;
  for (std::string const &command : queue) {
    pool.push_back(std::thread(&OSLayer::_execute_command, this, command, silent));
  }
  queue = {};
  for (std::thread &thread : pool) {
    thread.join();
  }
  return error;
}

bool OSLayer::_execute_queue_sync() {
  for (std::string const &command : queue) {
    _execute_command(command, silent);
  }
  queue = {};
  return error;
}

void OSLayer::_execute_command(std::string command, bool silent) {
  int code;
  if (silent)
    code = system((command + " 1>/dev/null 2>&1").c_str());
  else
    code = system(command.c_str());

  if (code != 0)
    error = true;
}

// #define __SHELL_SUFFIX " 2>&1"
//
// // Executes a shell command and returns the output.
// #ifdef __GNUC__
// #include <unistd.h>
// ShellResult Shell::execute(std::string cmdline) {
//   return {"", 0};
//   // Setup pipes
//   char buffer[128];
//   std::string stdout;
//
//   FILE *process = popen((cmdline + __SHELL_SUFFIX).c_str(), "r");
//   if (!process)
//     throw ShellException("Unable to execute command");
//
//   while (fgets(buffer, sizeof(buffer), process) != NULL) {
//     stdout += buffer;
//   }
//
//   int status = pclose(process);
//   return {
//       stdout,
//       status,
//   };
// }
// #elif __WIN32
// std::string Shell::execute(std::string cmdline) {
//   // TODO: IMPLEMENT THIS
//   throw ShellException("Win32 not supported yet");
// }
// #else
// #error Unsupported platform: Only Gnu and Win32 are supported
// #endif
