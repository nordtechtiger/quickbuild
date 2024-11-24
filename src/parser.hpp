#ifndef PARSER_H
#define PARSER_H
#include "lexer.hpp"
#include <string>
#include <vector>
#include <variant>

// TODO: We need to figure out how to parse [] properly
// == Grammar rules ==
// var -> IDENTIFIER EQUALS expression ";"
// expression -> (LITERAL | IDENTIFIER | replace)","+
// expression -> ((LITERAL | IDENTIFIER | replace) CONCAT)+
// replace -> IDENTIFIER COLON LITERAL ARROW LITERAL
//
// target -> IDENTIFIER TARGETOPEN expression* TARGETCLOSE
// target -> IDENTIFIER AS IDENTIFIER TARGETOPEN expression* TARGETCLOSE
//

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
private:
  std::vector<Token> m_token_stream;
  AST m_ast;

  unsigned long long m_index;
  Token m_current;
  Token m_next;

  Token advance_token();
  int check_current(TokenType token_type);
  int check_next(TokenType token_type);
  int parse_variable();
  int parse_target();
  Expression parse_replace();

public:
  Parser(std::vector<Token> token_stream);
  AST parse_tokens();
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
