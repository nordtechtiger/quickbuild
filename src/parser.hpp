#ifndef PARSER_H
#define PARSER_H
#include "lexer.hpp"
#include <string>
#include <vector>

enum class FieldType {
  Literal,
  Expression,
};

struct Field {
  FieldType field_type;
};

struct Object {
  std::vector<Field> fields;
  std::vector<Object> children;
};

// Work class
class Parser {
  Object parse_tokens(std::vector<Token> token_stream);
};

#endif
