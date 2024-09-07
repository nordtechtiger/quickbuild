#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

#include "lexer.hpp"

#define SEARCH_DIR "."
#define CONFIG_FILE "quickbuild"

using namespace std;

int main(int argc, char **argv) {
  cout << "= Quickbuild Beta v0.1.0\n";

  // Get a list of all files in this diectory, recursively
  cout << "- Scanning directory...\n";
  vector<filesystem::directory_entry> file_tree;
  auto dir_iterator = filesystem::recursive_directory_iterator(SEARCH_DIR);
  for (auto &entry : dir_iterator) {
    // Ignore .git and directories
    if (entry.is_directory() ||
        entry.path().string().find(".git") != string::npos)
      continue;
    file_tree.push_back(entry);
  }

  // Read config file
  ifstream config_file(CONFIG_FILE, ios::binary);
  if (!config_file.is_open()) {
    cerr << "= Error: Couldn't find config file\n";
    return EXIT_FAILURE;
  }
  vector<unsigned char> config_buffer(istreambuf_iterator(config_file), {});

  // Feed config into lexer
  Lexer lexer = Lexer();
  vector<Token> tokens;
  try {
    tokens = lexer.lex_bytes(config_buffer);
  } catch (LexerException lexer_exception) {
    cerr << "= Error: Failed to lex config file. Details:\n";
    cerr << "=" << lexer_exception.what();
  }

  // NOTE: Debugging purposes only!
  for (const auto &i : tokens) {
    cout << "token: [" << static_cast<typename underlying_type<TokenType>::type>(i.token_type) << ", \"" << get<string>(i.token_context)
         << "\"]" << endl;
  }

  return EXIT_SUCCESS;
}
