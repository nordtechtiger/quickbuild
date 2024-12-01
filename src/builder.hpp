#ifndef BUILDER_H
#define BUILDER_H

#include "driver.hpp"
#include "parser.hpp"

#include <string>
#include <vector>

class Builder {
private:
  AST m_ast;
  Setup m_setup;
  void build_target(Literal target);
  std::vector<std::string> evaluate(Expression expression);

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
