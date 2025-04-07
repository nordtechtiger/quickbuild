#include "driver.hpp"
#include "interpreter.hpp"
#include "error.hpp"
#include "format.hpp"
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
}

// TODO: Perhaps also slightly messy. Refactor?
std::string get_line(Origin origin, std::vector<unsigned char> config) {
  int line_start = 0;
  int line_end = 0;
  for (size_t i = 0; i < origin.index && i < config.size(); i++) {
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
  LOG_STANDARD(RED << "! build stopped." << RESET);
  while ((error_info = ErrorHandler::pop_error())) {
    if (0 <= error_info->origin.index) {
      std::string line_str = get_line(error_info->origin, config);
      // Todo: This is ugly, rewrite?
      std::string underline;
      for (const auto &_ : line_str) {
        underline += "^";
      }
      LOG_STANDARD(std::right << std::setw(5) << " " << "| ...");
      LOG_STANDARD(std::right << std::setw(4) << error_info->origin.line << " | " << line_str);
      LOG_STANDARD(std::right << std::setw(5) << " " << "| " << underline);
    }
    LOG_STANDARD(RED << "! error: " << error_info->message);
    LOG_STANDARD("!   -> " << error_info->description << RESET);
  }
}

int Driver::run() {
  LOG_STANDARD(BOLD << "[ quickbuild dev v0.7.0 ]" << RESET);

  // compile the config.
  LOG_STANDARD("⧖ compiling config...");
  std::vector<unsigned char> config = get_config();

  Lexer lexer(config);
  std::vector<Token> token_stream;
  AST ast;
  try {
    token_stream = lexer.get_token_stream(); 
    Parser parser = Parser(token_stream);
    ast = parser.parse_tokens();
  } catch (BuildException e) {
    display_error_stack(config);
    return EXIT_FAILURE;
  }

  // build target.
  Interpreter interpreter(ast, m_setup);
  try {
    interpreter.build();
  } catch (BuildException e) {
    display_error_stack(config);
    return EXIT_FAILURE;
  }

  LOG_STANDARD("= build completed successfully.");
  return EXIT_SUCCESS;
}

