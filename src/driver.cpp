#include "driver.hpp"
#include "errors.hpp"
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
std::string get_line(InputStreamPos pos, std::vector<unsigned char> config) {
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
  LOG_STANDARD(RED << "⮾ build stopped." << RESET
                   << " unwinding error stack...");
  int error_n = 0;
  while ((error_info = ErrorHandler::pop_error())) {
    std::string prefix(error_n * 2, ' ');
    LOG_STANDARD(prefix << "┊");
    std::string ref = (error_info->context.ref
                           ? " (object: '" + *error_info->context.ref + "')"
                           : "");
    LOG_STANDARD(prefix << "├" << RED << "❬error #" << error_n
                        << "❭: " + error_info->message << ref << RESET);
    if (error_info->context.stream_pos) {
      std::string line_str = get_line(*error_info->context.stream_pos, config);
      std::string underline;
      for (const auto &_ : line_str) {
        underline += "^";
      }
      LOG_STANDARD(prefix << "│" << std::right << std::setw(4)
                          << error_info->context.stream_pos->line << " │ "
                          << line_str);
      LOG_STANDARD(prefix << "╰───" << std::right << std::setw(2) << " " << "╵ "
                          << underline);
    } else {
      LOG_STANDARD(prefix << "╰───"
                          << " warning: couldn't trace origin of error");
    }
    error_n++;
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

    // build task.
    Interpreter interpreter(ast, m_setup);
    interpreter.build();

  } catch (BuildException &e) {
    display_error_stack(config);
    LOG_STANDARD("");
    LOG_STANDARD("➤ build " << RED << "failed" << RESET);
    return EXIT_FAILURE;
  }

  LOG_STANDARD("➤ build completed");
  return EXIT_SUCCESS;
}
