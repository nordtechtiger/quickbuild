#include "lexer.hpp"
#include <functional>

// Used for determining e.g. variable names
#define IS_ALPHABETIC(x)                                                       \
  (((x >= 'A') && (x <= 'Z')) || ((x >= 'a') && (x <= 'z')) || x == '_' ||     \
   x == '-')

// initializes new lexer
Lexer::Lexer(std::vector<unsigned char> input_bytes) {
  m_index = 0;
  m_offset = 0;
  m_input = input_bytes;
  m_current = (m_input.size() >= m_index + 1) ? m_input[m_index] : '\0';
  m_next = (m_input.size() >= m_index + 2) ? m_input[m_index + 1] : '\0';
  m_state = LexerState::Normal;
}

unsigned char Lexer::advance_input_byte() {
  m_index++;
  m_current = (m_input.size() >= m_index + 1) ? m_input[m_index] : '\0';
  m_next = (m_input.size() >= m_index + 2) ? m_input[m_index + 1] : '\0';
  return m_current;
}

size_t Lexer::get_real_offset() { return m_index - m_offset; }

// TODO: I've been told that this solution is unholy and a disgrace to mankind.
// Consider alternative solutions
void Lexer::insert_next_byte(unsigned char byte) {
  m_offset += 1; // Keep track of original position
  m_input.insert(m_input.begin() + m_index + 1, byte);
}

// gets next token from stream
std::vector<Token> Lexer::get_token_stream() {
  while (m_current != '\0') {
    for (const auto &fn : matching_rules) {
      int result = fn();
      if (0 > result) {
        continue; // Token doesn't match, keep trying
      }
      break; // Token matches, advance input byte
    }
    // TODO: Ideally we need to check whether a rule successfully matched or
    // not, and gracefully error out if an unrecognized character is found
    advance_input_byte();
  }
  return m_t_stream;
}

// == all functions for validating/checking tokens below ==

// skip whitespace
int Lexer::skip_whitespace() {
  // always return false - we just want to increase the iterator until we hit a
  // non-whitespace
  while (m_current == ' ' || m_current == '\n' || m_current == '\t')
    advance_input_byte();
  return -1;
}

// skip any comments (#)
int Lexer::skip_comments() {
  if (m_current != '#') {
    return -1; // No comments
  }
  while (m_current != '\n') {
    advance_input_byte(); // Currently in a comment
  }
  return -1;
}

// match =
int Lexer::match_equals() {
  if (m_current == '=') {
    m_t_stream.push_back(
        Token{TokenType::Equals, std::nullopt, get_real_offset()});
    return 0;
  } else {
    return -1;
  }
}

// match :
int Lexer::match_modify() {
  if (m_current == ':') {
    m_t_stream.push_back(
        Token{TokenType::Modify, std::nullopt, get_real_offset()});
    return 0;
  } else {
    return -1;
  }
}

// match ;
int Lexer::match_linestop() {
  if (m_current == ';') {
    m_t_stream.push_back(
        Token{TokenType::LineStop, std::nullopt, get_real_offset()});
    return 0;
  } else {
    return -1;
  }
}

// match ->
int Lexer::match_arrow() {
  if (m_current == '-' && m_next == '>') {
    // skip an extra byte due to 2-character token
    advance_input_byte();
    m_t_stream.push_back(
        Token{TokenType::Arrow, std::nullopt, get_real_offset()});
    return 0;
  } else {
    return -1;
  }
}

// match as
int Lexer::match_iterateas() {
  if (m_current == 'a' && m_next == 's') {
    // skip an extra byte due to 2-character token
    advance_input_byte();
    m_t_stream.push_back(
        Token{TokenType::IterateAs, std::nullopt, get_real_offset()});
    return 0;
  } else {
    return -1;
  }
}

// match ,
int Lexer::match_separator() {
  if (m_current == ',') {
    m_t_stream.push_back(
        Token{TokenType::Separator, std::nullopt, get_real_offset()});
    return 0;
  } else {
    return -1;
  }
}

// match [
int Lexer::match_expressionopen() {
  if (m_current == '[') {
    m_t_stream.push_back(
        Token{TokenType::ExpressionOpen, std::nullopt, get_real_offset()});
    return 0;
  } else {
    return -1;
  }
}

// match ]
int Lexer::match_expressionclose() {
  if (m_current == ']') {
    if (m_state != LexerState::EscapedLiteral) {
      m_t_stream.push_back(
          Token{TokenType::ExpressionClose, std::nullopt, get_real_offset()});
    } else {
      m_t_stream.push_back(
          Token{TokenType::ConcatLiteral, std::nullopt, get_real_offset()});
      // Boostrap the next part to be parsed as a string
      // NOTE: Again, this is apparantly unorthodox. Consider alternatives.
      insert_next_byte('\"');
      m_state = LexerState::Normal;
    }
    return 0;
  } else {
    return -1;
  }
}

// match {
int Lexer::match_targetopen() {
  if (m_current == '{') {
    m_t_stream.push_back(
        Token{TokenType::TargetOpen, std::nullopt, get_real_offset()});
    return 0;
  } else {
    return -1;
  }
}

// match }
int Lexer::match_targetclose() {
  if (m_current == '}') {
    m_t_stream.push_back(
        Token{TokenType::TargetClose, std::nullopt, get_real_offset()});
    return 0;
  } else {
    return -1;
  }
}

// match literals
// TODO: This is such a mess. Might be able to refactor this
int Lexer::match_literal() {
  if (m_current == '\"') {
    std::string literal;
    advance_input_byte();
    while (m_current != '\"' && m_current != '\0') {
      if (m_current == '[') {
        m_state = LexerState::EscapedLiteral;
        break;
      }
      literal += m_current;
      advance_input_byte();
    }
    m_t_stream.push_back(Token{TokenType::Literal, literal, get_real_offset()});
    if (m_state == LexerState::EscapedLiteral) {
      m_t_stream.push_back(
          Token{TokenType::ConcatLiteral, std::nullopt, get_real_offset()});
    }
    return 0;
  } else {
    return -1;
  }
}

// match identifiers
int Lexer::match_identifier() {
  if (IS_ALPHABETIC(m_current)) {
    std::string identifier;
    identifier += m_current;
    while (IS_ALPHABETIC(m_next)) {
      identifier += m_next;
      advance_input_byte();
    }
    m_t_stream.push_back(
        Token{TokenType::Identifier, identifier, get_real_offset()});
    return 0;
  } else {
    return -1;
  }
}
