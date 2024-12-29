#include "builder.hpp"
#include "error.hpp"
#include "format.hpp"
#include "parser.hpp"
#include "shell.hpp"

#include <filesystem>
#include <string>
#include <variant>
#include <vector>

#include <sys/stat.h>
#ifdef WIN32
#define stat _stat
#endif

#define FIELD_ID_DEPENDENCIES Identifier{"depends", 0}
#define FIELD_ID_EXECUTE Identifier{"run", 0}

Builder::Builder(AST ast, Setup setup) {
  m_ast = ast;
  m_setup = setup;
  // m_cache = {};
}

// Parses a string by matching against *
// NOTE: Assumes that max one asterisk is present
std::vector<std::string> Builder::evaluate_literal(Literal literal) {
  std::vector<std::string> out;
  size_t asterisk_index = literal.literal.find('*');
  if (asterisk_index == std::string::npos) {
    // No processing needs to be done
    return {literal.literal};
  }

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
    ErrorHandler::push_error_throw(-1, _B_EXPR_UPGRADE);
  }
  __builtin_unreachable();
}

std::vector<std::string>
Builder::evaluate_concatenation(Concatenation concatenation,
                                std::optional<Target> ctx) {
  std::string out;
  for (const auto &expression : concatenation) {
    std::vector<std::string> _out =
        evaluate(__upgrade_expression_type(expression), ctx);
    if (_out.size() >= 1) {
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

std::optional<std::vector<std::string>>
Builder::get_cached_expression(Expression expression,
                               std::optional<Target> ctx) {
  for (const auto &i_expression : m_cache) {
    if (std::get<0>(i_expression) == expression &&
        std::get<1>(i_expression) == ctx)
      return std::get<2>(i_expression);
  }
  return std::nullopt;
}

// Evaluates a vector of expressions within a certain context, yielding the
// lowest-level string representation
std::vector<std::string> Builder::evaluate(std::vector<Expression> expressions,
                                           std::optional<Target> ctx,
                                           bool use_cache) {
  std::vector<std::string> out;
  for (const Expression &expression : expressions) {
    std::vector<std::string> _out = evaluate(expression, ctx, use_cache);
    out.insert(out.end(), _out.begin(), _out.end());
  }
  return out;
}

std::vector<std::string> Builder::evaluate(Expression expression,
                                           std::optional<Target> ctx,
                                           bool use_cache) {

  // this is checked *before* the cache because otherwise iteration
  // variables are going to be incorrectly cached
  if (Identifier *identifier = std::get_if<Identifier>(&expression)) {
    // -- identifier
    // need to verify that the field exists...
    std::optional<std::vector<Expression>> field = get_field(ctx, *identifier);
    if (!field)
      ErrorHandler::push_error_throw(-1, B_INVALID_FIELD); // TODO: TRACKING!
    std::vector<std::string> result = evaluate(*field, ctx);
    return result;
  } else if (Concatenation *concatenation =
                 std::get_if<Concatenation>(&expression)) {
    // -- concatenation
    std::vector<std::string> result =
        evaluate_concatenation(*concatenation, ctx);
    return result;
  } else if (Replace *replace = std::get_if<Replace>(&expression)) {
    // -- replace
    std::vector<std::string> result = evaluate_replace(*replace, ctx);
    return result;
  }

  if (use_cache) {
    std::optional<std::vector<std::string>> result =
        get_cached_expression(expression, ctx);
    if (result) {
      // std::cerr << "cache hit for " << ctx->public_name.identifier <<
      // std::endl;
      return *result;
    }
  }

  if (Literal *literal = std::get_if<Literal>(&expression)) {
    // -- literal
    std::vector<std::string> result = evaluate_literal(*literal);
    m_cache.push_back({expression, ctx, result});
    return result;
  } else {
    ErrorHandler::push_error_throw(-1, _B_INVALID_EXPR_VARIANT);
  }
  __builtin_unreachable();
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
  // last effort: check for iterator name
  if (target && target->public_name.identifier == identifier.identifier) {
    // This is a terrible hack but seems to satisfy the compiler for now
    // Also, the origin=0 isn't optimal but works for now
    std::vector<Expression> __out = {Literal{m_target_ref, 0}};
    return __out;
  }
  // ErrorHandler::push_error_throw(-1, B_INVALID_FIELD);
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

// Returns timestamp of latest file modification
// TODO: Verify error handling
int Builder::get_file_date(std::string path) {
  struct stat t_stat;
  stat(path.c_str(), &t_stat);
  time_t t = t_stat.st_ctim.tv_sec;
  return t;
}

bool Builder::is_dirty(Literal literal, std::string dependant) {
  std::optional<Target> target = get_target(literal);
  // If the target is defined, scan the dependencies...
  if (target) {
    std::optional<std::vector<Expression>> dependencies_expression =
        get_field(target, FIELD_ID_DEPENDENCIES);
    std::vector<std::string> dependencies;
    m_target_ref = literal.literal;
    if (dependencies_expression)
      dependencies = evaluate(*dependencies_expression, target);
    else
      return true; // If a custom target has no dependencies, it should always
                   // be triggered
    for (const std::string &dependency : dependencies) {
      if (is_dirty(Literal{dependency, 0}, literal.literal))
        return true;
    }
    return false;
  }
  // If not, compare the file dates - target vs depdencies...
  return get_file_date(literal.literal) >= get_file_date(dependant);
}

void Builder::build_target(Target target, Literal ctx_literal) {
  // Resolve all dependencies
  std::optional<std::vector<Expression>> dependencies_expression =
      get_field(target, FIELD_ID_DEPENDENCIES);
  std::vector<std::string> dependencies;
  m_target_ref = ctx_literal.literal;
  if (dependencies_expression)
    dependencies = evaluate(*dependencies_expression, target);
  for (const auto &dependency : dependencies) {
    std::optional<Target> dep_target = get_target(Literal{dependency, target.origin});
    if (!dep_target)
      continue;
    build_target(*dep_target, Literal{dependency, target.origin});
  }

  // Build final target
  LOG_STANDARD_NO_NEWLINE("  - Building " + ctx_literal.literal + "...");
  if (!is_dirty(ctx_literal, "__root__")) {
    LOG_STANDARD(ITALIC " <no change>" RESET);
    return;
  }
  m_target_ref = ctx_literal.literal;
  std::optional<std::vector<Expression>> cmdline_expression = get_field(target, FIELD_ID_EXECUTE);
  if (!cmdline_expression) {
    LOG_STANDARD(RED " <invalid>" RESET);
    ErrorHandler::push_error_throw(target.origin, B_NO_CMDLINE);
  }
  std::vector<std::string> cmdlines =
      evaluate(*cmdline_expression, target);
  std::string stdout;
  for (const std::string &cmdline : cmdlines) {
    // Execute the command line with the appropriate output (verbose, quiet,
    // etc)
    LOG_VERBOSE_NO_NEWLINE("\n  > " + cmdline);
    if (m_setup.dry_run)
      continue;

    ShellResult result = Shell::execute(cmdline);
    stdout += result.stdout;
    if (result.status) { // Error
      LOG_STANDARD(RED " <failed>" RESET);
      LOG_STANDARD(stdout);
      // FIXME: This should ideally be able to point to the
      // command that failed to execute.
      ErrorHandler::push_error_throw(-1, B_NON_ZERO_PROCESS);
    }
  }
  LOG_STANDARD(GREEN " <ok>" RESET);
  if (!stdout.empty())
    LOG_STANDARD(stdout);
}

void Builder::build() {
  Literal literal;
  if (m_setup.target) {
    literal = Literal{*m_setup.target, 0};
  } else if (m_ast.targets.size() >= 1) {
    literal = std::get<1>(m_ast.targets[0].identifier);
  } else {
    ErrorHandler::push_error_throw(-1, B_NO_TARGETS_FOUND);
  }

  if (m_setup.dry_run) {
    LOG_STANDARD("= " GREEN "Building " CYAN + literal.literal +
                 ITALIC " [dry run]" RESET);
  } else {
    LOG_STANDARD("= " GREEN "Building " CYAN + literal.literal + RESET);
  }

  std::optional<Target> target = get_target(literal);
  if (!target)
    ErrorHandler::push_error_throw(-1, B_MISSING_TARGET);
  build_target(*target, literal);
}
