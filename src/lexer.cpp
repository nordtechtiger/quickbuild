#include "lexer.hpp"
#include "errors.hpp"
#include <functional>
#include <variant>

// used for determining e.g. variable names.
inline bool is_alphabetic(char x) {
  return ((x >= 'A') && (x <= 'Z')) || ((x >= 'a') && (x <= 'z')) || x == '_' ||
         x == '-' || (x >= '0' && x <= '9');
}

// initializes new lexer.
Lexer::Lexer(std::vector<unsigned char> input_bytes) {
  m_index = 0;
  _m_line = 1;
  m_input = input_bytes;
  m_current = (m_input.size() >= m_index + 1) ? m_input[m_index] : '\0';
  m_next = (m_input.size() >= m_index + 2) ? m_input[m_index + 1] : '\0';
}

unsigned char Lexer::consume_byte() { return consume_byte(1); }

unsigned char Lexer::consume_byte(int n) {
  size_t original_index = m_index;
  // loop to scan for newlines.
  for (; m_index < original_index + n;) {
    m_index++;
    m_current = (m_input.size() >= m_index + 1) ? m_input[m_index] : '\0';
    m_next = (m_input.size() >= m_index + 2) ? m_input[m_index + 1] : '\0';
    if (m_current == '\n')
      _m_line++;
  }
  return m_current;
}

// turn the current state into an origin.
Origin Lexer::get_local_origin() { return InputStreamPos{m_index, _m_line}; }

// gets next token from stream.
std::vector<Token> Lexer::get_token_stream() {
  while (m_current != '\0') {
    bool match = false;
    for (const auto &fn : matching_rules) {
      std::optional<Token> token = fn();
      if (token) {
        m_token_stream.push_back(*token);
        match = true;
        break;
      }
    }
    if (m_current == '\0')
      break;
    if (!match) {
      ErrorHandler::push_error_throw(get_local_origin(), L_INVALID_SYMBOL);
    }
  }
  return m_token_stream;
}

// skip all whitespace characters and comments.
std::optional<Token> Lexer::skip_whitespace_comments() {
  while (m_current == ' ' || m_current == '\n' || m_current == '\t' ||
         m_current == '#') {
    while (m_current == ' ' || m_current == '\n' || m_current == '\t')
      consume_byte();
    if (m_current != '#')
      return std::nullopt; // not a comment.
    while (m_current != '\n') {
      consume_byte(); // currently in a comment.
    }
  }
  return std::nullopt;
}

// match =
std::optional<Token> Lexer::match_equals() {
  if (m_current != '=')
    return std::nullopt;
  consume_byte();
  return Token{TokenType::Equals, std::nullopt, get_local_origin()};
}

// match :
std::optional<Token> Lexer::match_modify() {
  if (m_current != ':')
    return std::nullopt;
  consume_byte();
  return Token{TokenType::Modify, std::nullopt, get_local_origin()};
}

// match ;
std::optional<Token> Lexer::match_linestop() {
  if (m_current != ';')
    return std::nullopt;
  consume_byte();
  return Token{TokenType::LineStop, std::nullopt, get_local_origin()};
}

// match ->
std::optional<Token> Lexer::match_arrow() {
  if (!(m_current == '-' && m_next == '>'))
    return std::nullopt;
  consume_byte(2);
  return Token{TokenType::Arrow, std::nullopt, get_local_origin()};
}

// match ,
std::optional<Token> Lexer::match_separator() {
  if (m_current != ',')
    return std::nullopt;
  consume_byte();
  return Token{TokenType::Separator, std::nullopt, get_local_origin()};
}

// match [
std::optional<Token> Lexer::match_expressionopen() {
  if (m_current != '[')
    return std::nullopt;
  consume_byte();
  return Token{TokenType::ExpressionOpen, std::nullopt, get_local_origin()};
}

// match ]
std::optional<Token> Lexer::match_expressionclose() {
  if (m_current != ']')
    return std::nullopt;
  consume_byte();
  return Token{TokenType::ExpressionClose, std::nullopt, get_local_origin()};
}

// match {
std::optional<Token> Lexer::match_targetopen() {
  if (m_current != '{')
    return std::nullopt;
  consume_byte();
  return Token{TokenType::TargetOpen, std::nullopt, get_local_origin()};
}

// match }
std::optional<Token> Lexer::match_targetclose() {
  if (m_current != '}')
    return std::nullopt;
  consume_byte();
  return Token{TokenType::TargetClose, std::nullopt, get_local_origin()};
}

// match literals
std::optional<Token> Lexer::match_literal() {
  if (m_current != '\"')
    return std::nullopt;
  consume_byte();
  Token formatted_literal = Token{
      TokenType::FormattedLiteral,
      std::vector<Token>{},
      get_local_origin(),
  };
  std::string substr;
  while (m_current != '\"') {
    if (m_current == '[') {
      consume_byte(); // consume the `[`.
      std::get<CTX_VEC>(*formatted_literal.context)
          .push_back(Token{TokenType::Literal, substr, get_local_origin()});
      substr = "";
      // lex escaped expression.
      while (m_current != ']') {

        std::optional<Token> inner_token;
        skip_whitespace_comments();
        // note: the parser only supports escaped identifiers
        if ((inner_token = match_modify())) {
        } else if ((inner_token = match_arrow())) {
        } else if ((inner_token = match_separator())) {
        } else if ((inner_token = match_identifier())) {
        }

        if (!inner_token)
          ErrorHandler::push_error_throw(get_local_origin(),
                                         L_INVALID_LITERAL);

        std::get<CTX_VEC>(*formatted_literal.context).push_back(*inner_token);
      }
      consume_byte(); // consume the `]`
      continue;
    }

    // lex "pure" literal:
    substr += m_current;
    consume_byte();
  }
  consume_byte(); // consume the `"`
  std::get<CTX_VEC>(*formatted_literal.context)
      .push_back(Token{TokenType::Literal, substr, get_local_origin()});
  return formatted_literal;
}

// match identifiers
std::optional<Token> Lexer::match_identifier() {
  if (!is_alphabetic(m_current))
    return std::nullopt;
  std::string identifier;
  while (is_alphabetic(m_current)) {
    identifier += m_current;
    consume_byte();
  }

  if (identifier == "as")
    return Token{TokenType::IterateAs, std::nullopt, get_local_origin()};
  else if (identifier == "true")
    return Token{TokenType::True, std::nullopt, get_local_origin()};
  else if (identifier == "false")
    return Token{TokenType::False, std::nullopt, get_local_origin()};
  else
    return Token{TokenType::Identifier, identifier, get_local_origin()};
}
