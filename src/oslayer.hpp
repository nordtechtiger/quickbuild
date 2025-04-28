#ifndef OSLAYER_H 
#define OSLAYER_H

#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <optional>
#include <mutex>
#include "lexer.hpp"
#include "errors.hpp"

struct Command {
  std::string cmdline;
  Origin origin;
};

class OSLayer {
private:
  bool silent;
  bool parallel;

  std::vector<Command> queue = {};
  std::vector<ErrorContext> errors;
  std::mutex error_lock;

  void _execute_command(Command command);
  
  void _execute_queue_sync();
  void _execute_queue_parallel();

public:
  OSLayer(bool parallel, bool silent);
  void queue_command(Command command);
  void execute_queue();
  std::vector<ErrorContext> get_errors();

  static std::optional<size_t> get_file_timestamp(std::string path);
};

#endif
