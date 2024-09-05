#include "lexer.hpp"
#include <iostream>

using namespace std;

Lexer::Lexer() { this->lex_state = NONE; }

vector<Token> Lexer::lex_bytes(const vector<unsigned char> input_bytes) {
  // Vector with all tokens
  vector<Token> tokens;
  string working_str;

  // Scan through every character
  for (auto const &character : input_bytes) {
    // Skip empty spaces and newlines
    if (this->lex_state == NONE && character == '\n') {
      continue;
    }

    // Lex literals
    if (this->lex_state == IN_STRING) {
      if (character != '\"') {
        working_str += character;
      } else if (character == '\"') {
        tokens.push_back(
          Token {
            Literal,
            TokenContext {working_str},
          }
        );
        working_str = "";
      }
    }

    // Lex identifiers
    if (this->lex_state == IN_LITERAL) {
      if (character != '=' && character != ',') {
        working_str += character;
      } else if (character == ',') {
        tokens.push_back(
          Token {
            Identifier,
            TokenContext {working_str},
          }
            );
        working_str = "";
      }
    } else if (character != ' ' && character != ',') {
      this->lex_state = IN_LITERAL;
      working_str += character;
    }

    // Handle expressions inside strings
    // if (character == '[') {
    //   if (this->lex_state == IN_STRING) {

    //   }
    // }

    // Handle string states
    if (character == '\"')
      this->lex_state = (this->lex_state == NONE ? IN_STRING : NONE);
    cout << character;
  }

  // Done
  return tokens;
}
