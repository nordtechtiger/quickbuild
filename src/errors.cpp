#include "errors.hpp"

std::vector<ErrorInfo> ErrorHandler::error_stack = {};

// push an error onto the stack so that it can be traced later.
void ErrorHandler::push_error(ErrorContext context, ErrorCode error_code) {
  std::string message = _ERROR_LOOKUP_TABLE.at(error_code);
  ErrorInfo error_info = {
      context,
      error_code,
      message,
  };
  error_lock.lock();
  error_stack.push_back(error_info);
  error_lock.unlock();
}

// push an error and abandon ship.
void ErrorHandler::push_error_throw(ErrorContext context,
                                    ErrorCode error_code) {
  std::string message = _ERROR_LOOKUP_TABLE.at(error_code);
  ErrorInfo error_info = {
      context,
      error_code,
      message,
  };
  error_lock.lock();
  error_stack.push_back(error_info);
  error_lock.unlock();
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

std::optional<ErrorInfo> ErrorHandler::peek_error() {
  if (error_stack.empty())
    return std::nullopt;
  return error_stack.back();
}

ErrorContext::ErrorContext(Origin const &origin) {
  if (std::holds_alternative<InputStreamPos>(origin)) {
    this->stream_pos = std::get<InputStreamPos>(origin);
    this->ref = std::nullopt;
  } else if (std::holds_alternative<ObjectReference>(origin)) {
    this->stream_pos = std::nullopt;
    this->ref = std::get<ObjectReference>(origin);
  } else {
    this->stream_pos = std::nullopt;
    this->ref = std::nullopt;
  }
}

ErrorContext::ErrorContext(InternalNode const &) {
  this->stream_pos = std::nullopt;
  this->ref = std::nullopt;
}

ErrorContext::ErrorContext(ObjectReference const &ref) {
  this->stream_pos = std::nullopt;
  this->ref = ref;
}

ErrorContext::ErrorContext(Origin const &origin, ObjectReference const &ref) {
  if (std::holds_alternative<InputStreamPos>(origin)) {
    this->stream_pos = std::get<InputStreamPos>(origin);
    this->ref = std::nullopt;
  } else if (std::holds_alternative<ObjectReference>(origin)) {
    this->stream_pos = std::nullopt;
    this->ref = std::get<ObjectReference>(origin);
  } else {
    this->stream_pos = std::nullopt;
    this->ref = std::nullopt;
  }
  this->ref = ref;
}
