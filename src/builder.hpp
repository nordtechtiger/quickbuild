#ifndef BUILDER_H
#define BUILDER_H

#include "driver.hpp"
#include "parser.hpp"

class Builder {
private:
  AST m_ast;
  Setup m_setup;

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
