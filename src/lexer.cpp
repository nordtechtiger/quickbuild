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
  m_input_index = 0;
  m_input_bytes = input_bytes;
  if (m_input_bytes.size() >= 2) {
    m_peek_current_byte = m_input_bytes[m_input_index];
    m_peek_next_byte = m_input_bytes[m_input_index + 1];
  } else if (m_input_bytes.size() == 1) {
    m_peek_current_byte = m_input_bytes[m_input_index];
    m_peek_next_byte = '\0';
  } else {
    m_peek_current_byte = '\0';
    m_peek_next_byte = '\0';
  }
}

unsigned char Lexer::advance_input_byte() {
  m_input_index++;

  // end of byte stream
  if (m_input_index >= m_input_bytes.size()) {
    m_peek_current_byte = '\0';
    m_peek_next_byte = '\0';
    return m_peek_current_byte;
  }
  // last byte
  if (m_input_index >= (m_input_bytes.size() - 1)) {
    m_peek_current_byte = m_input_bytes[m_input_index];
    m_peek_next_byte = '\0';
    return m_peek_current_byte;
  }
  // more than 2 bytes left
  m_peek_current_byte = m_input_bytes[m_input_index];
  m_peek_next_byte = m_input_bytes[m_input_index + 1];
  return m_peek_current_byte;
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
       << lexer.m_peek_current_byte << endl;
  return make_tuple(false, Token{TokenType::Literal, "huh"});
}

