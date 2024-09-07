#include "lexer.hpp"
#include <iostream>

using namespace std;

Lexer::Lexer() { this->lex_state = LexState::None; }

vector<Token> Lexer::lex_bytes(const vector<unsigned char> input_bytes) {
  // Vector with all tokens
  vector<Token> tokens;
  string working_str;

  // Scan through every character
  for (auto const &character : input_bytes) {
  }

  // Done
  return tokens;
}
