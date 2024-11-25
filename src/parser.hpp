#ifndef PARSER_H
#define PARSER_H
#include "lexer.hpp"
#include <string>
#include <vector>
#include <variant>

// TODO: Parse precedence correctly []
// == Grammar rules ==
// field -> IDENTIFIER "=" expression ";"
// expression -> (replace | LITERAL | IDENTIFIER)","+
// expression -> ((replace | LITERAL | IDENTIFIER)CONCAT)+
// replace -> IDENTIFIER ":" LITERAL "->" LITERAL
//
// target -> expression TARGETOPEN expression* TARGETCLOSE
// target -> expression AS IDENTIFIER TARGETOPEN expression* TARGETCLOSE
//

// Logic: Expressions
struct Identifier {
  std::string identifier;
};
struct Literal {
  std::string literal;
};
struct Replace {
  Identifier variable;
  Literal original;
  Literal replacement;
};
typedef std::variant<Identifier, Literal, Replace> _expression;
typedef std::vector<_expression> Concatenation;
typedef std::variant<Identifier, Literal, Replace, Concatenation> Expression;

// Config: Fields, targets, AST
struct Field {
  Identifier identifier;
  std::vector<Expression> expression;
};
struct Target {
  Expression identifier;
  Identifier public_name;
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
  Token advance_token(int n);
  bool check_current(TokenType token_type);
  bool check_next(TokenType token_type);
  std::optional<Field> parse_field();
  std::optional<Target> parse_target();
  std::optional<Replace> parse_replace();
  std::optional<std::vector<Expression>> parse_expression();
  std::optional<Concatenation> parse_concatenation();

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
