#include "parser.hpp"
#include "error.hpp"
#include "lexer.hpp"
#include <iostream>

// Initialize fields
Parser::Parser(std::vector<Token> token_stream) {
  m_token_stream = token_stream;
  m_index = 0;
  // the origin should never be read, so we can keep it as 0
  m_previous = Token{TokenType::Invalid, "__invalid", 0};
  m_current = (m_token_stream.size() >= m_index + 1)
                  ? m_token_stream[m_index]
                  : Token{TokenType::Invalid, "__invalid__", 0};
  m_next = (m_token_stream.size() >= m_index + 2)
               ? m_token_stream[m_index + 1]
               : Token{TokenType::Invalid, "__invalid__", 0};
}

// Consume a token
Token Parser::consume_token() { return consume_token(1); }

// Consume n tokens
Token Parser::consume_token(int n) {
  m_index += n;
  m_previous = m_current;
  // the origin should never be read, so we can keep it as 0
  m_current = (m_token_stream.size() >= m_index + 1)
                  ? m_token_stream[m_index]
                  : Token{TokenType::Invalid, "__invalid__", 0};
  m_next = (m_token_stream.size() >= m_index + 2)
               ? m_token_stream[m_index + 1]
               : Token{TokenType::Invalid, "__invalid__", 0};
  return m_previous;
}

AST Parser::parse_tokens() {
  while (m_current.type != TokenType::Invalid) {
    // Try to parse field
    auto field = parse_field();
    if (field) {
      m_ast.fields.push_back(*field);
      continue;
    }
    // Try to parse target
    auto target = parse_target();
    if (target) {
      m_ast.targets.push_back(*target);
      continue;
    }
    // Unknown sequence of tokens
    ErrorHandler::push_error_throw(m_current.origin, P_NO_MATCH);
  }
  return m_ast;
}

// Token checking, no side effects
bool Parser::check_current(TokenType token_type) {
  return m_current.type == token_type;
}

// Token checking, no side effects
bool Parser::check_next(TokenType token_type) {
  return m_next.type == token_type;
}

// FIXME: Currently only identifiers and literals can be used as targets
// FIXME: Also currently chokes on `objects as obj {` ...
// Attempts to parse a target
std::optional<Target> Parser::parse_target() {
  // Doesn't match
  std::optional<std::tuple<Expression, Identifier, size_t>> target_header =
      parse_target_header();
  if (!target_header)
    return std::nullopt;

  // Fill in the metadata for the target
  Target target;
  target.identifier = std::get<0>(*target_header);
  target.public_name = std::get<1>(*target_header);
  target.origin = std::get<2>(*target_header);

  // Populates all the fields
  std::optional<Field> field;
  while ((field = parse_field())) {
    target.fields.push_back(*field);
  }

  // Checks that the target is closed
  if (!check_current(TokenType::TargetClose))
    ErrorHandler::push_error_throw(m_current.origin, P_EXPECTED_TARGET_CLOSE);
  consume_token(); // Consume the `}`

  return target;
}

// Returns identifier and public_name
std::optional<std::tuple<Expression, Identifier, size_t>>
Parser::parse_target_header() {
  Expression expression;
  Identifier public_name;
  size_t origin;
  if (check_next(TokenType::TargetOpen)) {
    // `object {`
    origin = m_current.origin;
    if (check_current(TokenType::Identifier)) {
      Token identifier_t = consume_token();
      public_name = Identifier{*identifier_t.context, identifier_t.origin};
      expression = Expression{public_name};
    } else if (check_current(TokenType::Literal)) {
      public_name = Identifier{"__target__", m_current.origin};
      Token literal_t = consume_token();
      expression = Expression{Literal{*literal_t.context, literal_t.origin}};
    } else {
      ErrorHandler::push_error_throw(m_current.origin, P_BAD_TARGET);
    }
    consume_token(); // Consume the `{`
    return std::make_tuple(expression, public_name, origin);
  } else if (check_next(TokenType::IterateAs)) {
    // `objects as obj {`
    origin = m_current.origin;
    if (check_current(TokenType::Identifier)) {
      Token identifier_t = consume_token();
      expression = Identifier{*identifier_t.context, identifier_t.origin};
    } else if (check_current(TokenType::Literal)) {
      Token literal_t = consume_token();
      expression = Expression{Literal{*literal_t.context, literal_t.origin}};
    } else {
      ErrorHandler::push_error_throw(m_current.origin, P_BAD_TARGET);
    }
    consume_token(); // Consume the `as`
    if (check_current(TokenType::Identifier)) {
      Token identifier_t = consume_token();
      public_name = Identifier{*identifier_t.context, identifier_t.origin};
    } else {
      ErrorHandler::push_error_throw(m_current.origin, P_BAD_PUBLIC_NAME);
    }
    consume_token(); // Consume the `{`
    return std::make_tuple(expression, public_name, origin);
  } else {
    return std::nullopt;
  }
}

// Attempts to parse a singular field
std::optional<Field> Parser::parse_field() {
  // Doesn't match
  if (!check_current(TokenType::Identifier) || !check_next(TokenType::Equals))
    return std::nullopt;

  // Get the identifier
  Field field;
  field.origin = m_current.origin;
  Token identifier_t = consume_token();
  field.identifier = Identifier{*identifier_t.context, identifier_t.origin};
  consume_token(); // Consume the '='

  // Parse the expression
  auto expression = parse_expression();
  if (!expression)
    // unreachable as long as parse_expression() doesn't return a nullopt
    ErrorHandler::push_error_throw(m_current.origin, _P_NULLOPT_EXPR);
  field.expression = *expression;

  // Check that there is a linestop
  if (!check_current(TokenType::LineStop))
    ErrorHandler::push_error_throw(field.origin, P_EXPECTED_SEMICOLON);
  consume_token(); // Consume the ';'

  return field;
}

// FIXME: Currently always returns an expression, why std::optional
// Attempts to parse a singular expression
std::optional<std::vector<Expression>> Parser::parse_expression() {
  std::vector<Expression> expression;
  while (true) {
    // TODO: Add parsing for ',' and breaking out of the loop
    auto concatenation = parse_concatenation();
    if (concatenation) {
      expression.push_back(*concatenation);
      continue;
    }
    auto replace = parse_replace();
    if (replace) {
      expression.push_back(*replace);
      continue;
    }
    if (check_current(TokenType::Identifier)) {
      expression.push_back(Identifier{*m_current.context, m_current.origin});
      consume_token();
      continue;
    }
    if (check_current(TokenType::Literal)) {
      expression.push_back(Literal{*m_current.context, m_current.origin});
      consume_token();
      continue;
    }
    if (check_current(TokenType::ExpressionOpen)) {
      consume_token(); // Consume the '['
      std::optional<std::vector<Expression>> _expression = parse_expression();
      if (_expression) {
        expression.insert(expression.end(), _expression->begin(),
                          _expression->end());
      } else {
        // unreachable as long as parse_expression() doesn't return a nullopt
        ErrorHandler::push_error_throw(m_current.origin, _P_NULLOPT_EXPR);
      }
      if (!check_current(TokenType::ExpressionClose))
        ErrorHandler::push_error_throw(m_current.origin, P_EXPECTED_EXPR_CLOSE);
      consume_token(); // Consume the `]`
    }

    if (check_current(TokenType::Separator)) {
      consume_token(); // Consume the ','
      continue;
    }

    break; // FIXME: Not optimal?
  }

  if (0 >= expression.size())
    ErrorHandler::push_error_throw(m_current.origin, P_INVALID_EXPR_STATEMENT);

  return expression;
}

// FIXME: Currently only identifiers and literals can be escaped
// Attempts to parse a concatenation
std::optional<Concatenation> Parser::parse_concatenation() {
  if (!check_next(TokenType::ConcatLiteral))
    return std::nullopt;

  // TODO: Finish this
  Concatenation concatenation;
  do {
    if (check_current(TokenType::Identifier)) {
      Token identifier_t = consume_token();
      concatenation.push_back(
          Identifier{*identifier_t.context, identifier_t.origin});
      consume_token(); // Consume the concat token
      continue;
    } else if (check_current(TokenType::Literal)) {
      Token literal_t = consume_token();
      concatenation.push_back(Literal{*literal_t.context, literal_t.origin});
      consume_token(); // Consume the concat token
      continue;
    }
  } while (check_next(TokenType::ConcatLiteral));

  // There will still be a token left
  if (check_current(TokenType::Identifier)) {
    Token identifier_t = consume_token();
    concatenation.push_back(
        Identifier{*identifier_t.context, identifier_t.origin});
  } else if (check_current(TokenType::Literal)) {
    Token literal_t = consume_token();
    concatenation.push_back(Literal{*literal_t.context, literal_t.origin});
  }

  return concatenation;
}

// Attempts to parse a replacement
std::optional<Replace> Parser::parse_replace() {
  // Doesn't match
  if (!check_next(TokenType::Modify))
    return std::nullopt;

  // Construct replace type
  Replace replace;
  replace.origin = m_current.origin;
  Token identifier_t = consume_token();
  replace.identifier = Identifier{*identifier_t.context, identifier_t.origin};
  consume_token(); // Consume ':'
  Token literal_t_original = consume_token();
  replace.original =
      Literal{*literal_t_original.context, literal_t_original.origin};
  if (!check_current(TokenType::Arrow)) {
    std::cerr << "foo" << std::endl;
    ErrorHandler::push_error_throw(m_current.origin, P_EXPECTED_ITER_ARROW);
  }
  consume_token(); // Consume '->'
  Token literal_t_replacement = consume_token();
  replace.replacement =
      Literal{*literal_t_replacement.context, literal_t_replacement.origin};

  return replace;
}
