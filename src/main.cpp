#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

#include "lexer.hpp"

#define SEARCH_DIR "."
#define CONFIG_FILE "quickbuild"

using namespace std;

template <typename T> void print_vec(vector<T> vec) {
  for (auto const &i : vec) {
    cout << i << "\n";
  }
}

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
  try {
    auto tokens = lexer.lex_bytes(config_buffer);
  } catch (LexerException lexer_exception) {
    cerr << "= Error: Failed to lex config file. Details:\n";
    cerr << "=" << lexer_exception.what();
  }

  return EXIT_SUCCESS;
}
