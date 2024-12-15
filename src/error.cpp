#include "error.hpp"

std::vector<ErrorInfo> ErrorHandler::error_stack = {};

void ErrorHandler::push_error(size_t origin, ErrorCode error_code) {
  auto [message, description] = _ERROR_LOOKUP_TABLE.at(error_code);
  ErrorInfo error_info = {
      origin,
      error_code,
      message,
      description,
  };
  error_stack.push_back(error_info);
}

void ErrorHandler::push_error_throw(size_t origin, ErrorCode error_code) {
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

std::optional<ErrorInfo> ErrorHandler::pop_error() {
  if (error_stack.empty())
    return std::nullopt;
  ErrorInfo error_info = error_stack.back();
  error_stack.pop_back();
  return error_info;
}
