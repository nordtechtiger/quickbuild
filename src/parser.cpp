#include "parser.hpp"
#include "lexer.hpp"
#include <iostream>

using namespace std;

int parse_variable(std::vector<Token>, AST&);
int parse_child(std::vector<Token>, AST&);

const vector<int (*)(vector<Token>, AST&)> parsing_rules{
    /* parse_variable,
    parse_child, */
};

AST Parser::parse_tokens(std::vector<Token> token_stream) {
  AST ast;
  for (int token_index = 0; token_index < token_stream.size(); token_index++) {
    token_stream = vector(token_stream.begin() + 1, token_stream.end());
    if (0 == parse_variable(token_stream, ast)) {
      continue;
    }
    if (0 == parse_child(token_stream, ast)) {
      continue;
    }
    throw ParserException("Failed to match tokens");
  }
}

/* # Assuming current config, the AST should look like the following:
 *
 * field("compiler", <Literal"gcc">)
 * field("flags", <Literal"-g -o0">)
 * field("sources", <Literal"src/*.c">)
 * field("headers", <Literal"src/*.h">)
 * field("objects", <Replace"sources""src/*.c""obj/*.o">)
 * field("binary", <Literal"bin/quickbuild">)
 * target(Literal"quickbuild", "quickbuild", <
 *   field("depends", <Variable"objects", Variable"headers">)
 *   field("run", <Concatenation<Variable"compiler",Literal" ", ...etc>>)
 * >)
 * target(Variable"objects", "obj", <
 *   field("depends", <Replace"obj"obj/*.o""src/*.c">)
 *   field("run", <Concatenation<Variable"compiler",Literal" ", ...etc>>)
 * >)
 */
