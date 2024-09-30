#include "lexer.hpp"
#include <iostream>

using namespace std;

#define IS_ALPHABETIC(x)                                                       \
  (((x >= 'A') && (x <= 'Z')) || ((x >= 'a') && (x <= 'z')))

int debug_lexer_state(Lexer &, vector<Token> &);
int skip_whitespace(Lexer &, vector<Token> &);
int match_equals(Lexer &, vector<Token> &);
int match_modify(Lexer &, vector<Token> &);
int match_linestop(Lexer &, vector<Token> &);
int match_arrow(Lexer &, vector<Token> &);
int match_iterateas(Lexer &, vector<Token> &);
int match_separator(Lexer &, vector<Token> &);
int match_expressionopen(Lexer &, vector<Token> &);
int match_expressionclose(Lexer &, vector<Token> &);
int match_targetopen(Lexer &, vector<Token> &);
int match_targetclose(Lexer &, vector<Token> &);
int match_literal(Lexer &, vector<Token> &);
int match_identifier(Lexer &, vector<Token> &);

// contains all tokens to match against
const vector<int (*)(Lexer &, vector<Token> &)> match_tokens{
    debug_lexer_state,     skip_whitespace,  match_equals,
    match_modify,          match_linestop,   match_arrow,
    match_iterateas,       match_separator,  match_expressionopen,
    match_expressionclose, match_targetopen, match_targetclose,
    match_literal,         match_identifier,
};

// initializes new lexer
Lexer::Lexer(vector<unsigned char> input_bytes) {
  m_index = 0;
  m_input = input_bytes;
  m_current = (m_input.size() >= m_index + 1) ? m_input[m_index] : '\0';
  m_next = (m_input.size() >= m_index + 2) ? m_input[m_index + 1] : '\0';
}

unsigned char Lexer::advance_input_byte() {
  m_index++;
  m_current = (m_input.size() >= m_index + 1) ? m_input[m_index] : '\0';
  m_next = (m_input.size() >= m_index + 2) ? m_input[m_index + 1] : '\0';
  return m_current;
}

void Lexer::insert_next_byte(unsigned char byte) {
  m_input.insert(m_input.begin() + m_index, byte);
}

// gets next token from stream
vector<Token> Lexer::get_token_stream() {
  vector<Token> tokens;
  while (m_current != '\0') {
    for (const auto &fn : match_tokens) {
      int result = fn(*this, tokens);
      if (0 > result) {
        // Token doesn't match
        continue;
      }
      // Token matches
      break;
    }
    advance_input_byte();
  }
  return tokens;
}

// == all functions for validating/checking tokens below ==

// debugging
int debug_lexer_state(Lexer &lexer, vector<Token> &tokenstream) {
  cout << "Lex round initial char: "
       << lexer.m_current << endl;
  return -1;
}

// skip whitespace
int skip_whitespace(Lexer &lexer, vector<Token> &tokenstream) {
  // always return false - we just want to increase the iterator until we hit a
  // non-whitespace
  while (lexer.m_current == ' ')
    lexer.advance_input_byte();
  return -1;
}

// match =
int match_equals(Lexer &lexer, vector<Token> &tokenstream) {
  if (lexer.m_current == '=') {
    tokenstream.push_back(Token{TokenType::Symbol, SymbolType::Equals});
    return 0;
  } else {
    return -1;
  }
}

// match :
int match_modify(Lexer &lexer, vector<Token> &tokenstream) {
  if (lexer.m_current == ':') {
    tokenstream.push_back(Token{TokenType::Symbol, SymbolType::Modify});
    return 0;
  } else {
    return -1;
  }
}

// match ;
int match_linestop(Lexer &lexer, vector<Token> &tokenstream) {
  if (lexer.m_current == ';') {
    tokenstream.push_back(Token{TokenType::Symbol, SymbolType::LineStop});
    return 0;
  } else {
    return -1;
  }
}

// match ->
int match_arrow(Lexer &lexer, vector<Token> &tokenstream) {
  if (lexer.m_current == '-' && lexer.m_next == '>') {
    // skip an extra byte due to 2-character token
    lexer.advance_input_byte();
    tokenstream.push_back(Token{TokenType::Symbol, SymbolType::Arrow});
    return 0;
  } else {
    return -1;
  }
}

// match as
int match_iterateas(Lexer &lexer, vector<Token> &tokenstream) {
  if (lexer.m_current == 'a' && lexer.m_next == 's') {
    // skip an extra byte due to 2-character token
    lexer.advance_input_byte();
    tokenstream.push_back(Token{TokenType::Symbol, SymbolType::IterateAs});
    return 0;
  } else {
    return -1;
  }
}

// match ,
int match_separator(Lexer &lexer, vector<Token> &tokenstream) {
  if (lexer.m_current == ',') {
    tokenstream.push_back(Token{TokenType::Symbol, SymbolType::Separator});
    return 0;
  } else {
    return -1;
  }
}

// match [
int match_expressionopen(Lexer &lexer, vector<Token> &tokenstream) {
  if (lexer.m_current == '[') {
    tokenstream.push_back(Token{TokenType::Symbol, SymbolType::ExpressionOpen});
    return 0;
  } else {
    return -1;
  }
}

// match ]
int match_expressionclose(Lexer &lexer, vector<Token> &tokenstream) {
  // TODO: If is in an escaped literal, add custom token and begin next
  // string!!!!
  if (lexer.m_current == ']') {
    tokenstream.push_back(
        Token{TokenType::Symbol, SymbolType::ExpressionClose});
    if (lexer.m_state == LexerState::EscapedLiteral) {
      tokenstream.push_back(
          Token{TokenType::Symbol, SymbolType::ConcatLiteral});
      // Boostrap the next part to be parsed as a string
      lexer.insert_next_byte('\"');
    }
    return 0;
  } else {
    return -1;
  }
}

// match {
int match_targetopen(Lexer &lexer, vector<Token> &tokenstream) {
  if (lexer.m_current == '{') {
    tokenstream.push_back(Token{TokenType::Symbol, SymbolType::TargetOpen});
    return 0;
  } else {
    return -1;
  }
}

// match }
int match_targetclose(Lexer &lexer, vector<Token> &tokenstream) {
  if (lexer.m_current == '}') {
    tokenstream.push_back(Token{TokenType::Symbol, SymbolType::TargetClose});
    return 0;
  } else {
    return -1;
  }
}

// match literals
int match_literal(Lexer &lexer, vector<Token> &tokenstream) {
  if (lexer.m_current == '\"') {
    string literal;
    while (lexer.advance_input_byte() != '\"') {
      literal += lexer.m_current;
    }
    tokenstream.push_back(Token{TokenType::Literal, literal});
    return 0;
  } else {
    return -1;
  }
}

// match identifiers
int match_identifier(Lexer &lexer, vector<Token> &tokenstream) {
  if (IS_ALPHABETIC(lexer.m_current)) {
    string identifier;
    do {
      identifier += lexer.m_current;
      cout << "identifier: " << identifier << ", ";
    } while (IS_ALPHABETIC(lexer.advance_input_byte()));
    tokenstream.push_back(Token{TokenType::Identifier, identifier});
    return 0;
  } else {
    return -1;
  }
}
