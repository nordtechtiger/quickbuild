#ifndef BUILDER_H
#define BUILDER_H

#include "driver.hpp"
#include "parser.hpp"

#include <optional>
#include <string>
#include <vector>

class Builder {
private:
  AST m_ast;
  Setup m_setup;
  void build_target(Literal target);
  std::optional<Target> get_target(Literal literal);
  // if std::nullopt, use global vars
  std::optional<std::vector<Expression>> get_field(std::optional<Target> target,
                                                   Identifier identifier);
  std::vector<std::string> evaluate(Expression expression);
  std::vector<std::string> evaluate(std::vector<Expression> expressions);

public:
  Builder(AST ast, Setup setup);
  void build();
};

class BuilderException : std::exception {
private:
  const char *details;

public:
  BuilderException(const char *details) : details(details){};
  const char *what() { return details; }
};

#endif