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
    else if (arg == "--configfile")
      setup.input_method = InputMethod::ConfigFile;
    else if (arg == "--log-quiet")
      setup.logging_level = LoggingLevel::Quiet;
    else if (arg == "--log-standard")
      setup.logging_level = LoggingLevel::Standard;
    else if (arg == "--log-verbose")
      setup.logging_level = LoggingLevel::Verbose;
    else if (arg == "--dry-run")
      setup.dry_run = true;
    else if (arg == "--help") {
      std::cout << "Usage: quickbuild [arguments] <task>\n"
                   "  --stdin: reads config from stdin\n"
                   "  --configfile: reads config from file\n"
                   "  --log-quiet: sets logging level to quiet\n"
                   "  --log-standard: sets logging level to standard\n"
                   "  --log-verbose: sets logging level to verbose\n"
                   "  --dry-run: doesn't execute any commands\n"
                   "  --help: shows this message and exits\n";
      exit(EXIT_SUCCESS);
    } else if (!setup.task)
      setup.task = arg;
    else {
      std::cerr << "Error: More than one task was selected. Cannot proceed."
                << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  // Run driver
  // try {
    Driver driver = Driver(setup);
    return driver.run();
  // } catch (const std::exception &e) {
  //   std::cerr << "! <Fatal crash>\n! The Quickbuild driver threw an unhandled "
  //                "exception. This is an internal bug and should be reported."
  //             << std::endl;
  //   std::cerr << "! Error data: " << e.what() << std::endl;
  //   exit(EXIT_FAILURE);
  // }
}
