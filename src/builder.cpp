#include "builder.hpp"
#include "format.hpp"
#include "parser.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

#define FIELD_ID_DEPENDENCIES                                                  \
  Identifier { "depends" }
#define FIELD_ID_EXECUTE                                                       \
  Identifier { "run" }

Builder::Builder(AST ast, Setup setup) {
  m_ast = ast;
  m_setup = setup;
}

// Parses a string by matching against *
// NOTE: Assumes that max one asterisk is present
std::vector<std::string> Builder::evaluate_literal(Literal literal) {
  std::vector<std::string> out;
  unsigned long long asterisk_index = literal.literal.find('*');
  if (asterisk_index == std::string::npos)
    // No processing needs to be done
    return {literal.literal};

  std::string prefix = literal.literal.substr(0, asterisk_index);
  std::string suffix = literal.literal.substr(asterisk_index + 1);

  // Attempt substitution with paths
  // TODO: This might not be an optimal solution
  for (const auto &dir : std::filesystem::recursive_directory_iterator(".")) {
    std::string path = dir.path();
    if ((prefix.empty() || path.find(prefix) != std::string::npos) &&
        (suffix[0] == '\0' || path.find(suffix) != std::string::npos) &&
        (prefix.empty() || suffix[0] == '\0' ||
         path.find(prefix) < path.find(suffix))) {
      out.push_back(path);
    }
  }

  return out;
}

std::vector<std::string> Builder::evaluate_replace(Replace replace) {
  
}

std::vector<std::string>
Builder::evaluate_concatenation(Concatenation concatenation) {
  for (const auto &expression : concatenation) {
    // not good
    evaluate(expression);
  }
}

// Evaluates a vector of expressions within a certain context, yielding the
// lowest-level string representation
std::vector<std::string> Builder::evaluate(std::vector<Expression> expressions,
                                           std::optional<Target> ctx) {
  std::vector<std::string> out;
  for (const Expression &expression : expressions) {
    std::vector<std::string> _out = evaluate(expression, ctx);
    out.insert(out.end(), _out.begin(), _out.end());
  }
  return out;
}

std::vector<std::string> Builder::evaluate(Expression expression,
                                           std::optional<Target> ctx) {
  if (Literal *literal = std::get_if<Literal>(&expression)) {
    return evaluate_literal(*literal);
  } else if (Identifier *identifier = std::get_if<Identifier>(&expression)) {
    return {evaluate(*get_field(ctx, *identifier), ctx)};
  } else if (Concatenation *concatenation =
                 std::get_if<Concatenation>(&expression)) {
    return evaluate_concatenation(*concatenation);
  } else if (Replace *replace = std::get_if<Replace>(&expression)) {
    return evaluate_replace(*replace);
  } else {
    throw BuilderException(
        "Unable to evaluate expression - invalid expression variant");
  }
  // TODO: Implement all options
}

std::optional<std::vector<Expression>>
Builder::get_field(std::optional<Target> target, Identifier identifier) {
  if (target) {
    for (const Field &field : target->fields) {
      if (field.identifier.identifier == identifier.identifier)
        return field.expression;
    }
  }
  // if no target was specified or field wasn't found, go to global scope
  for (const Field &field : m_ast.fields) {
    if (field.identifier.identifier == identifier.identifier)
      return field.expression;
  }
}

std::optional<Target> Builder::get_target(Literal literal) {
  // Iterate over all targets
  for (const auto &target : m_ast.targets) {
    // Iterate over all matching identifiers
    for (const auto &identifier : evaluate(target.identifier, std::nullopt)) {
      if (identifier == literal.literal)
        return target;
    }
  }
  return std::nullopt;
}

void Builder::build_target(Literal literal) {
  // Verify that it can be built
  std::optional<Target> target = get_target(literal);
  if (!target)
    throw BuilderException("No data for building target");

  // Resolve all dependencies
  std::optional<std::vector<Expression>> dependencies_expression =
      get_field(target, FIELD_ID_DEPENDENCIES);
  std::vector<std::string> dependencies;
  if (dependencies_expression)
    dependencies = evaluate(*dependencies_expression, target);
  for (const auto &dependency : dependencies)
    build_target(Literal{dependency});

  // Build final target
  std::cout << "   -> Building " << literal.literal << "..." << std::endl;
  std::vector<std::string> cmdlines =
      evaluate(*get_field(std::nullopt, FIELD_ID_EXECUTE), target);
  for (const std::string cmdline : cmdlines) {
    std::cout << ">>>>>>>>>> " << cmdline << std::endl;
    // Execute the command line with the appropriate output (verbose, quiet,
    // etc)
  }
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
  std::cout << "=> " GREEN "Building " CYAN << target.literal << RESET
            << std::endl;
  build_target(target);
}
