#ifndef DRIVER_H
#define DRIVER_H

#include "errors.hpp"

#include <optional>
#include <string>
#include <vector>

enum class InputMethod {
  ConfigFile,
  Stdin,
};

enum class LoggingLevel {
  Quiet,
  Standard,
  Verbose,
};

struct Setup {
  std::optional<std::string> target;
  InputMethod input_method;
  LoggingLevel logging_level;
  bool dry_run;
};

class Driver {
private:
  Setup m_setup;
  void display_error_stack(std::vector<unsigned char> config);
  std::vector<unsigned char> get_config();

public:
  Driver(Setup);
  static Setup default_setup();
  int run();
};

class DriverException : public std::exception {
private:
  const char *details;

public:
  DriverException(const char *details) : details(details) {};
  const char *what() const noexcept override { return details; }
};

#endif
