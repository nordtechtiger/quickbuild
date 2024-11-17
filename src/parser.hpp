#ifndef PARSER_H
#define PARSER_H
#include "lexer.hpp"
#include <string>
#include <vector>

// Grammar rules
// target -> IDENTIFIER "=" expression ";"
// expression -> (LITERAL | IDENTIFIER | replace "."*)+
// expression -> ((LITERAL | IDENTIFIER | replace) CONCAT)*
// replace -> IDENTIFIER COLON LITERAL ARROW LITERAL
//
// target -> IDENTIFIER TARGETOPEN expression* TARGETCLOSE
// target -> IDENTIFIER AS IDENTIFIER TARGETOPEN expression* TARGETCLOSE

// Logic: Expressions
struct Variable {
  std::string identifier;
};
struct Literal {
  std::string content;
};
struct Replace {
  Variable variable;
  Literal original;
  Literal replacement;
};
typedef std::variant<Variable, Literal, Replace> _expression;
typedef std::vector<_expression> Concatenation;
typedef std::variant<Variable, Literal, Replace, Concatenation> Expression;

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

// Exceptions thrown by parser
class ParserException : public std::exception {
private:
  const char *details;

public:
  ParserException(const char *details) : details(details){};
  const char *what() { return details; }
};

#endif
