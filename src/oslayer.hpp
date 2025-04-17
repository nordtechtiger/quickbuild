#ifndef OSLAYER_H 
#define OSLAYER_H

#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <optional>

class OSLayer {
private:
  bool silent;
  bool parallel;
  std::atomic<bool> error = false;

  std::vector<std::string> queue = {};
  void _execute_command(std::string command, bool silent);
  
  bool _execute_queue_sync();
  bool _execute_queue_parallel();

public:
  OSLayer(bool parallel, bool silent);
  void queue_command(std::string command);
  bool execute_queue();

  static std::optional<size_t> get_file_timestamp(std::string path);
};

#endif
