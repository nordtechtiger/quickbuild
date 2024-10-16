#ifndef VARIABLE_RESOLVE_H
#define VARIABLE_RESOLVE_H

#include "parser.hpp"

class VariableResolver {
private:
  AST m_ast;

public:
  VariableResolver(AST);
};

#endif
