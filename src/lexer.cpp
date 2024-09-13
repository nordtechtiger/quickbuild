#include "lexer.hpp"
#include <iostream>

using namespace std;

#define IS_ALPHABETIC(x)                                                       \
  (((x >= 'A') && (x <= 'Z')) || ((x >= 'a') && (x <= 'z')))

tuple<bool, Token> match_literal(Lexer &lexer);

// contains all tokens to match against
const vector<tuple<bool, Token> (*)(Lexer &)> match_tokens{
    match_literal,
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

  // end of byte stream
  if (m_index >= m_input.size()) {
    m_current = '\0';
    m_next = '\0';
    return m_current;
  }
  // last byte
  if (m_index >= (m_input.size() - 1)) {
    m_current = m_input[m_index];
    m_next = '\0';
    return m_current;
  }
  // more than 2 bytes left
  m_current = m_input[m_index];
  m_next = m_input[m_index + 1];
  return m_current;
}

// gets next token from stream
Token Lexer::get_next_token() {
  Token token = {TokenType::Invalid, ""};

  for (const auto &fn : match_tokens) {
    tuple<bool, Token> result = fn(*this);
    if (get<0>(result)) {
      token = get<1>(result);
      break;
    }
  }

  advance_input_byte();
  return token;
}

// == all functions for validating/checking tokens below ==
tuple<bool, Token> match_literal(Lexer &lexer) {
  cout << "Hello from literal matching function, current state: "
       << lexer.m_current << endl;
  return make_tuple(false, Token{TokenType::Literal, "huh"});
}
