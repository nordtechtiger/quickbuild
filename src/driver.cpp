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

#include "driver.hpp"
#include "builder.hpp"
#include "format.hpp"
#include "lexer.hpp"
#include "parser.hpp"

#include <fstream>
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
  throw DriverException("Driver-D001: Failed to get config.");
}

int Driver::run() {
  // Out: Greeting
  LOG_STANDARD(BRIGHT "[ Quickbuild Dev v0.1.0 ]" RESET);

  // === Build the configuration ===
  // Out: Compiling config
  LOG_STANDARD("=> Building configuration...");
  // Get config
  std::vector<unsigned char> config = get_config();
  // Lex
  Lexer lexer(config);
  std::vector<Token> t_stream;
  try {
    t_stream = lexer.get_token_stream();
  } catch (LexerException e) {
    // TODO: Handle properly
    LOG_QUIET(RED "! Failed to lex configuration. Exiting..." RESET);
    LOG_VERBOSE(RED "!   -> " + (std::string)e.what() + RESET);
    return EXIT_FAILURE;
  }
  // Parse
  AST ast;
  try {
    Parser parser = Parser(t_stream);
    ast = parser.parse_tokens();
  } catch (ParserException e) {
    // TODO: Handle properly
    LOG_QUIET(RED "! Failed to lex configuration. Exiting..." RESET);
    LOG_VERBOSE(RED "!   -> " + (std::string)e.what() + RESET);
    return EXIT_FAILURE;
  }

  // === Build the project ===
  Builder builder(ast, m_setup);
  try {
    builder.build();
  } catch (BuilderException e) {
    // TODO: Handle properly
    LOG_QUIET(RED "! Failed to lex configuration. Exiting..." RESET);
    LOG_VERBOSE(RED "!   -> " + (std::string)e.what() + RESET);
    return EXIT_FAILURE;
  }

  LOG_STANDARD("=> Build completed successfully.");

  return EXIT_SUCCESS;
}
