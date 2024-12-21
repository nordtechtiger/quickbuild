#ifndef PARSER_H
#define PARSER_H
#include "lexer.hpp"
#include <string>
#include <variant>
#include <vector>

// Logic: Expressions
struct Identifier {
  std::string identifier;
  size_t origin;
};
struct Literal {
  std::string literal;
  size_t origin;
};
struct Replace {
  Identifier identifier;
  Literal original;
  Literal replacement;
  size_t origin;
};
typedef std::variant<Identifier, Literal, Replace> _expression;
typedef std::vector<_expression> Concatenation;
typedef std::variant<Identifier, Literal, Replace, Concatenation> Expression;

// Config: Fields, targets, AST
struct Field {
  Identifier identifier;
  std::vector<Expression> expression;
  size_t origin;
};
struct Target {
  Expression identifier;
  Identifier public_name;
  std::vector<Field> fields;
  size_t origin;
};
struct AST {
  std::vector<Field> fields;
  std::vector<Target> targets;
};

// Work class
class Parser {
private:
  std::vector<Token> m_token_stream;
  AST m_ast;

  unsigned long long m_index;
  Token m_previous;
  Token m_current;
  Token m_next;

  Token consume_token();
  Token consume_token(int n);
  bool check_current(TokenType token_type);
  bool check_next(TokenType token_type);
  std::optional<Field> parse_field();
  std::optional<Target> parse_target();
  std::optional<std::tuple<Expression, Identifier, size_t>> parse_target_header();
  std::optional<Replace> parse_replace();
  std::optional<std::vector<Expression>> parse_expression();
  std::optional<Concatenation> parse_concatenation();

public:
  Parser(std::vector<Token> token_stream);
  AST parse_tokens();
};

#endif
