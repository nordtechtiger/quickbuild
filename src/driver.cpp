#include "driver.hpp"
#include "builder.hpp"
#include "error.hpp"
#include "format.hpp"
#include "lexer.hpp"
#include "parser.hpp"

#include <fstream>
#include <iostream>
#include <iomanip>
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
      throw DriverException("Driver-D002: Couldn't find config file");
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

std::tuple<int, std::string> get_line(size_t origin, std::vector<unsigned char> config) {
  int line_num = 1;
  int line_start = 0;
  int line_end = 0;
  for (size_t i = 0; i <= origin && i < config.size(); i++) {
    if (config[i] == '\n') {
      line_num++;
      line_start = i + 1;
    }
  }
  for (size_t i = line_start; config[i] != '\n' && i < config.size(); i++)
    line_end = i;
  std::vector<unsigned char> line_vec(config.begin() + line_start, config.begin() + line_end + 1);
  return {line_num, std::string(line_vec.begin(), line_vec.end())};
}

void Driver::display_error_stack(std::vector<unsigned char> config) {
  std::optional<ErrorInfo> error_info;
  LOG_STANDARD(RED << "! Build stopped." << RESET);
  while ((error_info = ErrorHandler::pop_error())) {
    if (0 <= error_info->origin) {
      auto [line_num, line_str] = get_line(error_info->origin, config);
      // Todo: This is ugly, rewrite?
      std::string underline;
      for (const auto &_ : line_str) {
        underline += "^";
      }
      LOG_STANDARD(std::right << std::setw(5) << " " << "| ...");
      LOG_STANDARD(std::right << std::setw(4) << line_num << " | " << line_str);
      LOG_STANDARD(std::right << std::setw(5) << " " << "| " << underline);
    }
    LOG_STANDARD(RED << "! Error: " << error_info->message);
    LOG_STANDARD("!   -> " << error_info->description << RESET);
  }
}

int Driver::run() {
  // Out: Greeting
  LOG_STANDARD(BRIGHT "[ Quickbuild Dev v0.2.0 ]" RESET);

  // === Build the configuration ===
  // Out: Compiling config
  LOG_STANDARD("=> Building configuration...");
  // Get config
  std::vector<unsigned char> config = get_config();
  // Lex
  Lexer lexer(config);
  std::vector<Token> t_stream;
  t_stream = lexer.get_token_stream();
  // Parse
  AST ast;
  try {
    Parser parser = Parser(t_stream);
    ast = parser.parse_tokens();
  } catch (BuildException e) {
    display_error_stack(config);
    return EXIT_FAILURE;
  }

  // === Build the project ===
  Builder builder(ast, m_setup);
  try {
    builder.build();
  } catch (BuildException e) {
    display_error_stack(config);
    return EXIT_FAILURE;
  }

  LOG_STANDARD("=> Build completed successfully.");

  return EXIT_SUCCESS;
}

// std::string debug_token_to_string(Token token) {
//   std::string out;
//   switch (token.type) {
//   case TokenType::Identifier:
//     out = "Identifier";
//     break;
//   case TokenType::Literal:
//     out = "Literal";
//     break;
//   case TokenType::Equals:
//     out = "Equals";
//     break;
//   case TokenType::Modify:
//     out = "Modify";
//     break;
//   case TokenType::LineStop:
//     out = "LineStop";
//     break;
//   case TokenType::Arrow:
//     out = "Arrow";
//     break;
//   case TokenType::IterateAs:
//     out = "IterateAs";
//     break;
//   case TokenType::Separator:
//     out = "Separator";
//     break;
//   case TokenType::ExpressionOpen:
//     out = "ExpressionOpen";
//     break;
//   case TokenType::ExpressionClose:
//     out = "ExpressionClose";
//     break;
//   case TokenType::TargetOpen:
//     out = "TargetOpen";
//     break;
//   case TokenType::TargetClose:
//     out = "TargetClose";
//     break;
//   case TokenType::ConcatLiteral:
//     out = "ConcatLiteral";
//     break;
//   case TokenType::Invalid:
//     out = "Invalid";
//     break;
//   }
//   if (token.context)
//     out += ":`" + *token.context + "`";
//   return out;
// }
//
// void debug_print_tokens(std::vector<Token> token_stream) {
//   for (const auto &token : token_stream) {
//     std::cout << "[" << debug_token_to_string(token) << "] ";
//     if (token.type == TokenType::LineStop)
//       std::cout << std::endl;
//   }
//   std::cout << std::endl;
// }
