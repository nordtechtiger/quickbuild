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
  size_t asterisk_index = literal.literal.find('*');
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

std::vector<std::string> Builder::evaluate_replace(Replace replace,
                                                   std::optional<Target> ctx) {
  std::string original = replace.original.literal;
  size_t old_asterisk_index = original.find('*');
  std::string old_prefix = original.substr(0, old_asterisk_index);
  std::string old_suffix = original.substr(old_asterisk_index + 1);

  std::string replacement = replace.replacement.literal;
  size_t new_asterisk_index = replacement.find('*');
  std::string new_prefix = replacement.substr(0, new_asterisk_index);
  std::string new_suffix = replacement.substr(new_asterisk_index + 1);

  std::vector<std::string> out;
  for (const std::string &str : evaluate(replace.identifier, ctx)) {
    if (str.find(old_prefix) != std::string::npos &&
        str.find(old_suffix) != std::string::npos &&
        str.find(old_prefix) < str.find(old_suffix)) {
      out.push_back(new_prefix +
                    str.substr(str.find(old_prefix) + old_prefix.size(),
                               str.find(old_suffix) - old_prefix.size() -
                                   str.find(old_prefix)) +
                    new_suffix);
    }
  }
  return out;
}

Expression __upgrade_expression_type(_expression _expr) {
  if (Identifier *identifier = std::get_if<Identifier>(&_expr)) {
    return Identifier{*identifier};
  } else if (Literal *literal = std::get_if<Literal>(&_expr)) {
    return Literal{*literal};
  } else if (Replace *replace = std::get_if<Replace>(&_expr)) {
    return Replace{*replace};
  } else {
    throw BuilderException("[B004] Internal builder error: Failed to upgrade "
                           "expression type. This is a bug.");
  }
}

std::vector<std::string>
Builder::evaluate_concatenation(Concatenation concatenation,
                                std::optional<Target> ctx) {
  std::string out;
  for (const auto &expression : concatenation) {
    std::vector<std::string> _out =
        evaluate(__upgrade_expression_type(expression), ctx);
    if (_out.size() == 1) {
      out += _out[0];
    }
    if (_out.size() > 1) {
      for (size_t i = 1; i < _out.size(); i++) {
        out += " " + _out[i];
      }
    }
  }
  return {out};
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
    return evaluate_concatenation(*concatenation, ctx);
  } else if (Replace *replace = std::get_if<Replace>(&expression)) {
    return evaluate_replace(*replace, ctx);
  } else {
    throw BuilderException(
        "Unable to evaluate expression - invalid expression variant");
  }
}

std::optional<std::vector<Expression>>
Builder::get_field(std::optional<Target> target, Identifier identifier) {
  if (target) {
    for (const Field &field : target->fields) {
      if (field.identifier.identifier == identifier.identifier)
        return field.expression;
    }
    if (target->public_name.identifier == identifier.identifier) {
      // This is a terrible hack but seems to satisfy the compiler for now
      std::vector<Expression> __out = {target->identifier};
      return __out;
    }
  }
  // if no target was specified or field wasn't found, go to global scope
  for (const Field &field : m_ast.fields) {
    if (field.identifier.identifier == identifier.identifier)
      return field.expression;
  }
  return std::nullopt;
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
  for (const auto &dependency : dependencies) {
    if (!get_target(Literal{dependency}))
      continue;
    build_target(Literal{dependency});
  }

  // Build final target
  LOG_STANDARD("   -> Building " + literal.literal + "...");
  // TODO: This crashes the entire build if "run" isn't present. Proper error
  // recovery needed
  std::vector<std::string> cmdlines =
      evaluate(*get_field(target, FIELD_ID_EXECUTE), target);
  for (const std::string cmdline : cmdlines) {
    LOG_VERBOSE("      > " + cmdline);
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

  LOG_STANDARD("=> Initiating build!");
  LOG_STANDARD("=> " GREEN "Building " CYAN + target.literal + RESET);
  build_target(target);
}
