#ifndef PARSER_H
#define PARSER_H
#include "lexer.hpp"
#include <string>
#include <vector>

// Logic: Expressions

typedef std::string Variable;
typedef std::string Literal;
struct Replace {
  Variable variable;
  Literal original;
  Literal replacement;
};
typedef std::variant<Variable, Literal, Replace> Expression;
// typedef std::vector<_expression> Concatenation;
// typedef _expression Expression;

// Config: Fields, targets, AST

struct Field {
  std::string identifier;
  std::vector<Expression> value;
};
struct Target {
  Expression identifier;
  std::string public_name;
  std::vector<Field> fields;
};
struct AST {
  std::vector<Field> fields;
  std::vector<Target> targets;
};

// Work class

class Parser {
public:
  AST parse_tokens(std::vector<Token> token_stream);
};

#endif
