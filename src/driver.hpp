#ifndef DRIVER_H
#define DRIVER_H

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
  InputMethod input_method;
  LoggingLevel logging_level;
  bool dry_run;
};

class Driver {
private:
  Setup m_setup;
  std::vector<unsigned char> get_config();

public:
  Driver(Setup);
  static Setup default_setup();
  int run();
};

#endif
