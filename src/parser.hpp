#ifndef PARSER_H
#define PARSER_H
#include "lexer.hpp"
#include <memory>
#include <string>
#include <variant>
#include <vector>

struct Identifier;
struct Literal;
struct FormattedLiteral;
struct List;
struct Boolean;
struct Replace;
using ASTObject = std::variant<Identifier, Literal, FormattedLiteral, List, Boolean,
                               Replace>;

// Logic: Expressions
struct Identifier {
  std::string content;
  Origin origin;

  bool operator==(Identifier const &other) const;
};
struct Literal {
  std::string content;
  Origin origin;

  bool operator==(Literal const &other) const;
};
struct FormattedLiteral {
  std::vector<ASTObject> contents;
  Origin origin;

  bool operator==(FormattedLiteral const &other) const;
};
struct Boolean {
  bool content;
  Origin origin;

  bool operator==(Boolean const &other) const;
};
struct List {
  std::vector<ASTObject> contents;
  Origin origin;
  
  bool operator==(List const &other) const;
};
struct Replace {
  std::shared_ptr<ASTObject> identifier;
  std::shared_ptr<ASTObject> original;
  std::shared_ptr<ASTObject> replacement;
  Origin origin;

  bool operator==(Replace const &other) const;
};

// Config: Fields, targets, AST
struct Field {
  Identifier identifier;
  ASTObject expression;
  Origin origin;

  auto operator==(Field const &other) const {
    return this->identifier == other.identifier &&
           this->expression == other.expression && this->origin == other.origin;
  }
};
struct Target {
  ASTObject identifier;
  Identifier iterator;
  std::vector<Field> fields;
  Origin origin;

  auto operator==(Target const &other) const {
    return this->identifier == other.identifier &&
           this->iterator == other.iterator && this->fields == other.fields &&
           this->origin == other.origin;
  }
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

  size_t m_index;
  Token m_previous;
  Token m_current;
  Token m_next;

  Token consume_token();
  Token consume_token(int n);
  std::optional<Token> consume_if(TokenType token_type);
  bool check_current(TokenType token_type);
  bool check_next(TokenType token_type);
  std::optional<ASTObject> parse_ast_object();
  std::optional<ASTObject> parse_list();
  std::optional<ASTObject> parse_replace();
  std::optional<ASTObject> parse_primary();
  std::optional<Field> parse_field();
  std::optional<Target> parse_target();

public:
  Parser(std::vector<Token> token_stream);
  AST parse_tokens();
};

#endif
