#ifndef PARSER_H
#define PARSER_H
#include "lexer.hpp"
#include <string>
#include <vector>

struct Field {
  std::string identifier;
  std::string value;
};

struct Object {
  std::vector<Field> fields;
  std::vector<Object> children;
};

// Work class
class Parser {
public:
  Object parse_tokens(std::vector<Token> token_stream);
};

#endif
