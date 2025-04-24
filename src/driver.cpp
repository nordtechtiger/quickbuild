#include "driver.hpp"
#include "error.hpp"
#include "format.hpp"
#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>

#define CONFIG_FILE "./quickbuild"

Driver::Driver(Setup setup) { m_setup = setup; }

Setup Driver::default_setup() {
  return Setup{std::nullopt, InputMethod::ConfigFile, LoggingLevel::Standard,
               false};
}

std::vector<unsigned char> Driver::get_config() {
  switch (m_setup.input_method) {
  case InputMethod::ConfigFile: {
    std::ifstream config_file(CONFIG_FILE, std::ios::binary);
    if (!config_file.is_open())
      throw DriverException("driver-d002: couldn't find config file");
    return std::vector<unsigned char>(std::istreambuf_iterator(config_file),
                                      {});
  }
  case InputMethod::Stdin: {
    std::string line;
    std::string all;
    while (getline(std::cin, line))
      all += line;
    return std::vector<unsigned char>(all.begin(), all.end());
  }
  }
  __builtin_unreachable();
}

// todo: perhaps also slightly messy. refactor?
std::string get_line(Origin origin, std::vector<unsigned char> config) {
  if (!std::holds_alternative<InputStreamPos>(origin)) {
    // todo: error out here.
  }
  InputStreamPos pos = std::get<InputStreamPos>(origin);
  int line_start = 0;
  int line_end = 0;
  for (size_t i = 0; i < pos.index && i < config.size(); i++) {
    if (config[i] == '\n') {
      line_start = i + 1;
    }
  }
  line_end = line_start;
  for (size_t i = line_start; config[i] != '\n' && i < config.size(); i++)
    line_end = i;
  std::vector<unsigned char> line_vec(config.begin() + line_start,
                                      config.begin() + line_end + 1);
  return std::string(line_vec.begin(), line_vec.end());
}

// TODO: Not too bad, but consider a refactor
void Driver::display_error_stack(std::vector<unsigned char> config) {
  std::optional<ErrorInfo> error_info;
  LOG_STANDARD(RED << "! build stopped. unwinding error stack..." << RESET);
  while ((error_info = ErrorHandler::pop_error())) {
    // render trace/point of failure.
    if (std::holds_alternative<InternalNode>(error_info->origin)) {\
      LOG_STANDARD("! warning: internal compiler reference, no trace available");
    }
    else if (std::holds_alternative<InputStreamPos>(error_info->origin)) {
      InputStreamPos pos = std::get<InputStreamPos>(error_info->origin);
      std::string line_str = get_line(error_info->origin, config);
      std::string underline;
      for (const auto &_ : line_str) {
        underline += "^";
      }
      LOG_STANDARD(std::right << std::setw(5) << " " << "| ...");
      LOG_STANDARD(std::right << std::setw(4) << pos.line
                              << " | " << line_str);
      LOG_STANDARD(std::right << std::setw(5) << " " << "| " << underline);
    } else {
      LOG_STANDARD("! warning: couldn't retrieve trace, consider submitting a bug report");
    }
    // print error message.
    LOG_STANDARD(RED << "! error: " << error_info->message);
  }
}

int Driver::run() {
  LOG_STANDARD(BOLD << "[ quickbuild dev v0.7.0 ]" << RESET);

  // config needs to be fetched out of scope so that
  // it can be read when unwinding the error stack.
  LOG_STANDARD("⧗ compiling config...");
  std::vector<unsigned char> config = get_config();

  try {
    // build script.
    Lexer lexer(config);
    std::vector<Token> token_stream;
    token_stream = lexer.get_token_stream();

    Parser parser = Parser(token_stream);
    AST ast(parser.parse_tokens());

    // build target.
    Interpreter interpreter(ast, m_setup);
    interpreter.build();

  } catch (BuildException &e) {
    display_error_stack(config);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
