#include "lexer.hpp"
#include <iostream>

using namespace std;

#define IS_ALPHABETIC(x)                                                       \
  (((x >= 'A') && (x <= 'Z')) || ((x >= 'a') && (x <= 'z')))

Token match_literal(Lexer &lexer);

const vector<Token (*)(Lexer &)> match_tokens{
    match_literal,
};

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

Token Lexer::get_next_token() {
  Token token;

  for (const auto &fn : match_tokens) {
    fn(*this);
  }

  advance_input_byte();
  return token;
}

Token match_literal(Lexer &lexer) {
  cout << "Hello from literal matching function, current state: "
       << lexer.m_peek_current_byte << endl;
  return Token{TokenType::Literal, "huh"};
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
