#include "lexer.hpp"
#include <iostream>

using namespace std;

#define IS_ALPHABETIC(x)                                                       \
  (((x >= 'A') && (x <= 'Z')) || ((x >= 'a') && (x <= 'z')))

Lexer::Lexer() { this->lex_state = LexState::None; }

vector<Token> Lexer::lex_bytes(const vector<unsigned char> input_bytes) {
  // Vector with all tokens
  vector<Token> tokens;
  string buf;

  // Scan through every character
  for (int i = 0; i < input_bytes.size(); i++) {
    // Lex operations, curly brackets, line stops, etc
    if (this->lex_state == LexState::None) {
      if (input_bytes[i] == '=') {
        tokens.push_back(Token{TokenType::Symbol, SymbolType::Equals});
        continue;
      } else if (input_bytes[i] == ':') {
        tokens.push_back(Token{TokenType::Symbol, SymbolType::Modify});
        continue;
      } else if (input_bytes[i] == ';') {
        tokens.push_back(Token{TokenType::Symbol, SymbolType::LineStop});
        continue;
      } else if (input_bytes[i] == ',') {
        tokens.push_back(Token{TokenType::Symbol, SymbolType::Separator});
        continue;
      } else if (input_bytes[i] == '{') {
        tokens.push_back(Token{TokenType::Symbol, SymbolType::TargetOpen});
        continue;
      } else if (input_bytes[i] == '}') {
        tokens.push_back(Token{TokenType::Symbol, SymbolType::TargetClose});
      } else if (i + 1 < input_bytes.size()) {
        if (input_bytes[i] == '-' && input_bytes[i + 1] == '>') {
          tokens.push_back(Token{TokenType::Symbol, SymbolType::Arrow});
          i++;
          continue;
        } else if (input_bytes[i] == 'a' && input_bytes[i + 1] == 's') {
          tokens.push_back(Token{TokenType::Symbol, SymbolType::IterateAs});
          i++;
          continue;
        }
      }
    }

    // Lex identifiers (variables)
    if (this->lex_state == LexState::None && IS_ALPHABETIC(input_bytes[i])) {
      this->lex_state = LexState::Identifier;
      buf += input_bytes[i];
      continue;
    }
    if (this->lex_state == LexState::Identifier) {
      if (IS_ALPHABETIC(input_bytes[i])) {
        buf += input_bytes[i];
        continue;
      } else {
        tokens.push_back(Token{TokenType::Identifier, buf});
        this->lex_state = LexState::None;
        buf = "";
        continue;
      }
    }

    // Lex literals (strings)
    if (this->lex_state == LexState::None && input_bytes[i] == '\"') {
      this->lex_state = LexState::Literal;
      continue;
    }
    if (this->lex_state == LexState::Literal && input_bytes[i] != '\"' /* &&
        input_bytes[i] != '[' */ ) {
      buf += input_bytes[i];
      continue;
    }
    if (this->lex_state == LexState::Literal && input_bytes[i] == '\"') {
      tokens.push_back(Token{TokenType::Literal, buf});
      this->lex_state = LexState::None;
      buf = "";
      continue;
    }
  }

  // Done
  return tokens;
}
