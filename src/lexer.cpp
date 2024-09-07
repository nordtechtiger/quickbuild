#include "lexer.hpp"
#include <iostream>

using namespace std;

Lexer::Lexer() { this->lex_state = LexState::None; }

vector<Token> Lexer::lex_bytes(const vector<unsigned char> input_bytes) {
  // Vector with all tokens
  vector<Token> tokens;
  string working_str;

  // Scan through every character
  for (int i = 0; i < input_bytes.size(); i++) {
    const char character = input_bytes[i];
    // Lex operations, curly brackets, line stops, etc
    if (this->lex_state == LexState::None) {
      if (character == '=') {
        tokens.push_back(Token{TokenType::Symbol, SymbolType::Equals});
        continue;
      } else if (character == ':') {
        tokens.push_back(Token{TokenType::Symbol, SymbolType::Modify});
        continue;
      } else if (character == ';') {
        // TODO: No need for empty string, revise token types?
        tokens.push_back(Token{TokenType::Symbol, SymbolType::LineStop});
        continue;
      } else if (character == ',') {
        tokens.push_back(Token{TokenType::Symbol, SymbolType::Separator});
        continue;
      } else if (character == '{') {
        tokens.push_back(Token{TokenType::Symbol, SymbolType::TargetOpen});
        continue;
      } else if (character == '}') {
        tokens.push_back(Token{TokenType::Symbol, SymbolType::TargetClose});
      } else if (i + 1 < input_bytes.size()) {
        if (character == '-' && input_bytes[i + 1] == '>') {
          tokens.push_back(Token{TokenType::Symbol, SymbolType::Arrow});
          i++;
          continue;
        } else if (character == 'a' && input_bytes[i + 1] == 's') {
          tokens.push_back(Token{TokenType::Symbol, SymbolType::IterateAs});
          i++;
          continue;
        }
      }
    }
  }

  // Done
  return tokens;
}
