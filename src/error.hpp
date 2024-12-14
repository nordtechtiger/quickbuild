#ifndef ERROR_H
#define ERROR_H

#include <map>
#include <string>
#include <tuple>
#include <vector>

enum ErrorCode {
  // Parser
  P_NO_ITER_ARROW,
  P_INVALID_ITER_STATEMENT,
  P_EXPECTED_EXPR_CLOSE,
  P_INVALID_EXPR_STATEMENT,
  P_MISSING_SEMICOLON,
  P_INVALID_EXPR,
  P_BAD_PUBLIC_NAME, // This is only here because parser is incomplete
  P_UNSUPPORTED_TARGET,
  P_TARGET_NOT_CLOSED,
  P_UNKNOWN_STATEMENT,

  // Builder
  _B_EXPR_UPGRADE,         // Should *never* happen
  _B_INVALID_EXPR_VARIANT, // Should *never* happen
  B_TARGET_MISSING,
  B_NON_ZERO_PROCESS,
  B_NO_TARGETS_FOUND,
};

struct ErrorInfo {
  size_t origin;
  ErrorCode error_code;
  std::string message;
  std::string description;
};

const std::map<ErrorCode, std::tuple<std::string, std::string>> lookup_table = {
    // Error code, message, description
    {P_NO_ITER_ARROW,
     {"Missing replacement arrow",
      "Incorrect expression: Detected replacement but no arrow found"}},
    {P_INVALID_ITER_STATEMENT, {"Invalid iteration statement", ""}},
    {P_EXPECTED_EXPR_CLOSE},
    {P_INVALID_EXPR_STATEMENT},
    {P_MISSING_SEMICOLON},
    {P_INVALID_EXPR},
    {P_BAD_PUBLIC_NAME},
    {P_UNSUPPORTED_TARGET},
    {P_TARGET_NOT_CLOSED},
    {P_UNKNOWN_STATEMENT},

    {_B_EXPR_UPGRADE},
    {_B_INVALID_EXPR_VARIANT},
    {B_TARGET_MISSING},
    {B_NON_ZERO_PROCESS},
    {B_NO_TARGETS_FOUND},
};

class ErrorHandler {
private:
  static std::vector<ErrorInfo> error_stack;
  static std::map<ErrorCode, std::tuple<std::string, std::string>> lookup_table;

public:
  static void push_error(ErrorCode error_code);
  static void push_error_throw(ErrorCode error_code);
  static ErrorInfo pop_error();
};

std::map<ErrorCode, std::tuple<std::string, std::string>>
    ErrorHandler::lookup_table = {

        {P_NO_ITER_ARROW, {"", ""}}};

#endif
