#include "parser.hpp"
#include "lexer.hpp"
#include <iostream>

// Initialize fields
Parser::Parser(std::vector<Token> token_stream) {
  m_token_stream = token_stream;
  m_index = 0;
  m_current = (m_token_stream.size() >= m_index + 1)
                  ? m_token_stream[m_index]
                  : Token{TokenType::Invalid, "__invalid__"};
  m_next = (m_token_stream.size() >= m_index + 2)
               ? m_token_stream[m_index + 1]
               : Token{TokenType::Invalid, "__invalid__"};
}

// Consume a token
Token Parser::consume_token() { return consume_token(1); }

// Consume n tokens
Token Parser::consume_token(int n) {
  m_index += n;
  Token _previous = m_current;
  m_current = (m_token_stream.size() >= m_index + 1)
                  ? m_token_stream[m_index]
                  : Token{TokenType::Invalid, "__invalid__"};
  m_next = (m_token_stream.size() >= m_index + 2)
               ? m_token_stream[m_index + 1]
               : Token{TokenType::Invalid, "__invalid__"};
  return _previous;
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
    throw ParserException("[P001] Unknown statement encountered.");
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
  std::optional<std::tuple<Expression, Identifier>> target_header = parse_target_header();
  if (!target_header)
    return std::nullopt;

  // Fill in the metadata for the target
  Target target;
  target.identifier = std::get<0>(*target_header);
  target.public_name = std::get<1>(*target_header);

  // Populates all the fields
  std::optional<Field> field;
  while ((field = parse_field())) {
    target.fields.push_back(*field);
  }

  // Checks that the target is closed
  if (!check_current(TokenType::TargetClose))
    throw ParserException("[P002] Target not closed");
  consume_token(); // Consume the `}`

  return target;
}

// Returns identifier and public_name
std::optional<std::tuple<Expression, Identifier>> Parser::parse_target_header() {
  if (check_next(TokenType::TargetOpen)) {
    // `object {`
    Expression expression;
    Identifier public_name;
    if (check_current(TokenType::Identifier)) {
      public_name = Identifier{*consume_token().context};
      expression = Expression{public_name};
    }
    else if (check_current(TokenType::Literal)) {
      public_name = Identifier{"__target__"};
      expression = Expression{Literal{*consume_token().context}};
    }
    else {
      throw ParserException("[P003] Unsupported target");
    }
    consume_token(); // Consume the `{`
    return std::make_tuple(expression, public_name);
  } else if (check_next(TokenType::IterateAs)) {
    // `objects as obj {`
    Expression expression;
    Identifier public_name;
    if (check_current(TokenType::Identifier)) {
      expression = Identifier{*consume_token().context};
    }
    else if (check_current(TokenType::Literal)) {
      expression = Expression{Literal{*consume_token().context}};
    }
    else {
      throw ParserException("[P003] Unsupported target");
    }
    consume_token(); // Consume the `as`
    if (check_current(TokenType::Identifier)) {
      public_name = Identifier{*consume_token().context};
    } else {
      throw ParserException("[P008] Unsupported public name");
    }
    consume_token(); // Consume the `{`
    return std::make_tuple(expression, public_name);

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
  field.identifier = Identifier{*consume_token().context};
  consume_token(); // Consume the '='

  // Parse the expression
  auto expression = parse_expression();
  if (!expression)
    throw ParserException("[P004] Invalid expression");
  field.expression = *expression;

  // Check that there is a linestop
  if (!check_current(TokenType::LineStop))
    throw ParserException("[P005] Missing semicolon");
  consume_token(); // Consume the ';'

  return field;
}

// FIXME: Currently always returns an expressios, why std::optional
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
      expression.push_back(Identifier{*consume_token().context});
      continue;
    }
    if (check_current(TokenType::Literal)) {
      expression.push_back(Literal{*consume_token().context});
      continue;
    }

    if (check_current(TokenType::Separator)) {
      consume_token(); // Consume the ','
      continue;
    }

    break; // FIXME: Not optimal?

    // TODO: Finish this
    throw ParserException("[P007] Invalid expression statement");
  }

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
      concatenation.push_back(Identifier{*consume_token().context});
      consume_token(); // Consume the concat token
      continue;
    } else if (check_current(TokenType::Literal)) {
      concatenation.push_back(Literal{*consume_token().context});
      consume_token(); // Consume the concat token
      continue;
    }
  } while (check_next(TokenType::ConcatLiteral));

  // There will still be a token left
  if (check_current(TokenType::Identifier)) {
    concatenation.push_back(Identifier{*consume_token().context});
  } else if (check_current(TokenType::Literal)) {
    concatenation.push_back(Literal{*consume_token().context});
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
  replace.identifier = Identifier{*consume_token().context};
  consume_token(); // Consume ':'
  replace.original = Literal{*consume_token().context};
  if (!check_current(TokenType::Arrow))
    throw ParserException("[P006] Missing iteration arrow");
  consume_token(); // Consume '->'
  replace.replacement = Literal{*consume_token().context};

  return replace;
}
