#include "lexer.hpp"
#include <iostream>

using namespace std;

Lexer::Lexer() { this->lex_state = NONE; }

vector<Token> Lexer::lex_bytes(const vector<unsigned char> input_bytes) {
  // Vector with all tokens
  vector<Token> tokens;

  // Scan through every character
  for (auto const &character : input_bytes) {
    // Skip empty spaces and newlines
    if (this->lex_state == NONE && character == '\n') {
      continue;
    }

    // Handle expressions inside strings
    if (character == '[') {
      if (this->lex_state == IN_STRING) {

      }
    }

    // Handle string states
    if (character == '\"')
      this->lex_state = (this->lex_state == NONE ? IN_STRING : NONE);
    cout << character;
  }

  // Done
  return tokens;
}
