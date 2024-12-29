#ifndef ERROR_H
#define ERROR_H

#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

enum ErrorCode {
  // Parser
  P_EXPECTED_ITER_ARROW,
  P_EXPECTED_EXPR_CLOSE,
  P_EXPECTED_TARGET_CLOSE,
  P_EXPECTED_SEMICOLON,
  P_INVALID_EXPR_STATEMENT,
  P_NO_MATCH,
  P_BAD_PUBLIC_NAME,
  P_BAD_TARGET, // This is only here beceause parser is incomplete
  _P_NULLOPT_EXPR,

  // Builder
  B_MISSING_TARGET,
  B_NON_ZERO_PROCESS,
  B_NO_TARGETS_FOUND,
  B_INVALID_FIELD,
  _B_EXPR_UPGRADE,         // Should *never* happen
  _B_INVALID_EXPR_VARIANT, // Should *never* happen
};

struct ErrorInfo {
  // size_t origin;
  int origin;
  ErrorCode error_code;
  std::string message;
  std::string description;
};

// Error code: <message, description>
const std::map<ErrorCode, std::tuple<std::string, std::string>>
    _ERROR_LOOKUP_TABLE = {
        {P_EXPECTED_ITER_ARROW,
         {"Missing replacement arrow",
          "Expected a replacement operator but no arrow was found."}},

        {P_EXPECTED_EXPR_CLOSE,
         {"Expected a closing bracket",
          "A bracket was opened but not correctly closed."}},

        {P_INVALID_EXPR_STATEMENT,
         {"Expression malformed",
          "An expression was expected but none was found."}},

        {P_EXPECTED_SEMICOLON,
         {"Missing semicolon", // line break here (clangformat is dumb)
          "Expected a semicolon at the end of a field."}},

        {P_BAD_PUBLIC_NAME,
         {"Unsupported iteration variable",
          "The iteration target is required to be stored in a variable."}},

        {P_BAD_TARGET,
         {"Unsupported target type",
          "Only variables and literals are supported as targets."}},

        {P_EXPECTED_TARGET_CLOSE,
         {"Expected target to be closed",
          "A target was opened but not closed."}},

        {P_NO_MATCH, {"Unknown operation", "The expression failed to parse."}},

        {_P_NULLOPT_EXPR,
         {"<internal> expression returned std::nullopt",
          "This is an internal bug in Quickbuild. Please submit an bug "
          "report."}},

        {_B_EXPR_UPGRADE,
         {"<internal> failed to upgrade expression type",
          "This is an internal bug in Quickbuild. Please submit an bug "
          "report."}},

        {_B_INVALID_EXPR_VARIANT,
         {"<internal> invalid expression variant",
          "This is an internal bug in Quickbuild. Please submit an bug "
          "report."}},

        {B_MISSING_TARGET,
         {"Missing target", "A target was required but did not exist"}},

        {B_NON_ZERO_PROCESS,
         {"Non-zero process return",
          "A command failed and the build was halted."}},

        {B_NO_TARGETS_FOUND,
         {"No targets found", "Couldn't find a target to build."}},

        {B_INVALID_FIELD,
         {"Invalid field referenced",
          "No field exists with this identifier. Are you sure you spelled it "
          "correctly?"}},
};

class ErrorHandler {
private:
  static std::vector<ErrorInfo> error_stack;

public:
  static void push_error(int origin, ErrorCode error_code);
  static void push_error_throw(int origin, ErrorCode error_code);
  static std::optional<ErrorInfo> pop_error();
};

class BuildException : public std::exception {
private:
  const char *details;

public:
  BuildException(const char *details) : details(details) {}
  const char *what() const noexcept override { return details; };
};

#endif
