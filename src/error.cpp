#include "error.hpp"
#include "lexer.hpp"

std::vector<ErrorInfo> ErrorHandler::error_stack = {};

// push an error onto the stack so that it can be traced later.
void ErrorHandler::push_error(Origin origin, ErrorCode error_code) {
  auto [message, description] = _ERROR_LOOKUP_TABLE.at(error_code);
  ErrorInfo error_info = {
      origin,
      error_code,
      message,
      description,
  };
  error_stack.push_back(error_info);
}

// push an error and abandon ship.
void ErrorHandler::push_error_throw(Origin origin, ErrorCode error_code) {
  auto [message, description] = _ERROR_LOOKUP_TABLE.at(error_code);
  ErrorInfo error_info = {
      origin,
      error_code,
      message,
      description,
  };
  error_stack.push_back(error_info);
  throw BuildException(message.c_str());
}

// get the latest error pushed to the stack.
std::optional<ErrorInfo> ErrorHandler::pop_error() {
  if (error_stack.empty())
    return std::nullopt;
  ErrorInfo error_info = error_stack.back();
  error_stack.pop_back();
  return error_info;
}
