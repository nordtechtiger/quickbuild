#include "driver.hpp"
#include "lexer.hpp"
#include "parser.hpp"

#include <fstream>
#include <iostream>
#include <iterator>

#define LL LoggingLevel
#define LL_SETUP m_setup.logging_level
#define CONFIG_FILE "./quickbuild"

using namespace std;

Driver::Driver(Setup setup) { m_setup = setup; }

Setup Driver::default_setup() {
  return Setup{InputMethod::ConfigFile, LoggingLevel::Standard, false};
}

vector<unsigned char> Driver::get_config() {
  switch (m_setup.input_method) {
  case InputMethod::ConfigFile: {
    ifstream config_file(CONFIG_FILE, ios::binary);
    if (!config_file.is_open())
      throw DriverException("Driver-D002: Couldn't find config file");
    return vector<unsigned char>(istreambuf_iterator(config_file), {});
  }
  case InputMethod::Stdin: {
    string line;
    string all;
    while (getline(cin, line))
      all += line;
    return vector<unsigned char>(all.begin(), all.end());
  }
  }
  throw DriverException("Driver-D001: Failed to get config.");
}

int Driver::run() {
  // Out: Greeting
  if (LL_SETUP >= LL::Standard) {
    cout << "@ Quickbuild Dev v0.1.0" << endl;
  }

  // Out: Compiling config
  if (LL_SETUP >= LL::Standard) {
    cout << "=> Building configuration..." << endl;
  }

  // Get config
  vector<unsigned char> config = get_config();

  // Lex
  Lexer lexer(config);
  vector<Token> t_stream = lexer.get_token_stream(); // TODO: Handle properly
  cout << "Tokens: " << t_stream.size() << endl;

  // Parse
  try {
    Parser parser = Parser(t_stream);
    AST ast = parser.parse_tokens();
  } catch (ParserException e) {
    // TODO: Handle properly
    // cerr << e.what();
    return -1;
  }

  cerr << ">>> Driver exiting normally." << endl;
  return EXIT_FAILURE; // TODO: Replace this when driver is finished
}
