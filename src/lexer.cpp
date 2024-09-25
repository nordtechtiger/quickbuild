#include "lexer.hpp"
#include <iostream>

using namespace std;

#define IS_ALPHABETIC(x)                                                       \
  (((x >= 'A') && (x <= 'Z')) || ((x >= 'a') && (x <= 'z')))

tuple<bool, Token> debug_lexer_state(Lexer &lexer);
tuple<bool, Token> skip_whitespace(Lexer &lexer);
tuple<bool, Token> match_equals(Lexer &lexer);
tuple<bool, Token> match_modify(Lexer &lexer);
tuple<bool, Token> match_linestop(Lexer &lexer);
tuple<bool, Token> match_arrow(Lexer &lexer);
tuple<bool, Token> match_iterateas(Lexer &lexer);
tuple<bool, Token> match_separator(Lexer &lexer);
tuple<bool, Token> match_expressionopen(Lexer &lexer);
tuple<bool, Token> match_expressionclose(Lexer &lexer);
tuple<bool, Token> match_targetopen(Lexer &lexer);
tuple<bool, Token> match_targetclose(Lexer &lexer);

// contains all tokens to match against
const vector<tuple<bool, Token> (*)(Lexer &)> match_tokens{
    debug_lexer_state,
    skip_whitespace,
    match_equals,
    match_modify,
    match_linestop,
    match_arrow,
    match_iterateas,
    /* match_separator,
    match_expressionopen,
    match_expressionclose,
    match_targetopen,
    match_targetclose, */
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

// gets next token from stream
Token Lexer::get_next_token() {
  Token token = TOKEN_INVALID;

  while (token.token_type == TokenType::Invalid && m_current != '\0') {
    for (const auto &fn : match_tokens) {
      tuple<bool, Token> result = fn(*this);
      if (get<0>(result)) {
        token = get<1>(result);
        break;
      }
    }
    advance_input_byte();
  }

  return token;
}

// == all functions for validating/checking tokens below ==

// debugging
tuple<bool, Token> debug_lexer_state(Lexer &lexer) {
  cout << "Hello from literal matching function, current state: "
       << lexer.m_current << endl;
  return make_tuple(false, TOKEN_INVALID);
}

// skip whitespace
tuple<bool, Token> skip_whitespace(Lexer &lexer) {
  // always return false - we just want to increase the iterator until we hit a
  // non-whitespace
  while (lexer.m_current == ' ')
    lexer.advance_input_byte();
  return make_tuple(false, TOKEN_INVALID);
}

// match =
tuple<bool, Token> match_equals(Lexer &lexer) {
  if (lexer.m_current == '=') {
    return make_tuple(true, Token{TokenType::Symbol, SymbolType::Equals});
  } else {
    return make_tuple(false, TOKEN_INVALID);
  }
}

// match :
tuple<bool, Token> match_modify(Lexer &lexer) {
  if (lexer.m_current == ':') {
    return make_tuple(true, Token{TokenType::Symbol, SymbolType::Modify});
  } else {
    return make_tuple(false, TOKEN_INVALID);
  }
}

// match ;
tuple<bool, Token> match_linestop(Lexer &lexer) {
  if (lexer.m_current == ';') {
    return make_tuple(true, Token{TokenType::Symbol, SymbolType::LineStop});
  } else {
    return make_tuple(false, TOKEN_INVALID);
  }
}

// match ->
tuple<bool, Token> match_arrow(Lexer &lexer) {
  if (lexer.m_current == '-' && lexer.m_next == '>') {
    // skip an extra byte due to 2-character token
    lexer.advance_input_byte();
    return make_tuple(true, Token{TokenType::Symbol, SymbolType::Arrow});
  } else {
    return make_tuple(false, TOKEN_INVALID);
  }
}

// match as
tuple<bool, Token> match_iterateas(Lexer &lexer) {
  if (lexer.m_current == 'a' && lexer.m_next == 's') {
    // skip an extra byte due to 2-character token
    lexer.advance_input_byte();
    return make_tuple(true, Token{TokenType::Symbol, SymbolType::IterateAs});
  } else {
    return make_tuple(false, TOKEN_INVALID);
  }
}
