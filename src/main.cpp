#include "driver.hpp"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char **argv) {
  // Collect all arguments except first which is the binary
  std::vector<std::string> args(argv + 1, argv + argc);

  // Parse arguments to create the preferred setup
  // TODO: Consider exiting with an error if argument isn't recognized
  Setup setup = Driver::default_setup();
  for (const auto &arg : args) {
    if (arg == "--stdin")
      setup.input_method = InputMethod::Stdin;
    if (arg == "--configfile")
      setup.input_method = InputMethod::ConfigFile;
    if (arg == "--log-quiet")
      setup.logging_level = LoggingLevel::Quiet;
    if (arg == "--log-standard")
      setup.logging_level = LoggingLevel::Standard;
    if (arg == "--log-verbose")
      setup.logging_level = LoggingLevel::Verbose;
    if (arg == "--dry-run")
      setup.dry_run = true;
  }

  // Run driver
  try {
    Driver driver = Driver(setup);
    return driver.run();
  } catch (...) {
    std::cerr << "! <Fatal crash>\n! The Quickbuild driver threw an unhandled "
                 "exception. This is an internal bug and should be reported."
              << std::endl;
    return -1;
  }
}
