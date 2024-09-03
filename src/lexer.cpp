#include "lexer.hpp"
#include <iostream>

using namespace std;

Lexer::Lexer() { this->lex_state = NONE; }

vector<Token> Lexer::lex_bytes(const vector<unsigned char> input_bytes) {
  cout << this->lex_state;
  for (auto const &character : input_bytes) {
    if (this->lex_state == NONE && (character == ' ' || character == '\n')) {
      continue;
    }
    if (character == '\"')
      this->lex_state = (this->lex_state == NONE ? IN_STRING : NONE);
    cout << character;
  }
  // flush
  cout << endl;
}
