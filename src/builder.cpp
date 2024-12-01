#include "builder.hpp"
#include "parser.hpp"

#include <iostream>
#include <string>
#include <vector>

#define GREEN "\033[32m"
#define CYAN "\033[36m"
#define RESET "\033[0m"

Builder::Builder(AST ast, Setup setup) {
  m_ast = ast;
  m_setup = setup;
}

std::vector<std::string> Builder::evaluate(Expression expression) {}

void Builder::build_target(Literal target) {
  std::cout << "Building: " << target.literal << std::endl;
}

void Builder::build() {
  Literal target;
  if (m_setup.target) {
    target = Literal{*m_setup.target};
  } else if (m_ast.targets.size() >= 1) {
    target = std::get<1>(m_ast.targets[0].identifier);
  } else {
    throw BuilderException("B001 No targets found");
  }

  std::cout << "=> Initiating build!" << std::endl;
  std::cout << "=> " << GREEN << "Building " << CYAN << target.literal << RESET
            << std::endl;
}
