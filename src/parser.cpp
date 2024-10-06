#include "parser.hpp"
#include "lexer.hpp"
#include <iostream>

using namespace std;

int parse_variable(vector<Token>, AST &);
int parse_target(vector<Token>, AST &);

const vector<int (*)(vector<Token>, AST &)> parsing_rules{
    parse_variable,
    parse_target,
};

int try_parse(vector<Token> token_stream, AST &ast) {
  for (const auto &fn : parsing_rules) {
    int tokens_parsed = fn(token_stream, ast);
    if (0 < tokens_parsed) {
      return tokens_parsed;
    }
  }
  return -1;
}

AST Parser::parse_tokens(vector<Token> token_stream) {
  AST ast;
  for (int index = 0; index < token_stream.size(); index++) {
    vector<Token> _token_stream =
        vector(token_stream.begin() + index, token_stream.end());
    if (0 == try_parse(_token_stream, ast))
      continue;
    throw ParserException("Failed to match tokens");
  }
  return ast;
}

// === All parsing logic below ===

int _parse_expression() {
  
}

int _parse_expressions(vector<Token> token_stream, vector<Expression> &expression) {
}

int parse_variable(vector<Token> token_stream, AST &ast) {
  // Verify the basic signature
  if (token_stream.size() < 4) {
    return -1;
  }
  if (!((token_stream[0].token_type == TokenType::Identifier) &&
        (token_stream[1].token_type == TokenType::Symbol) &&
        (get<0>(token_stream[1].token_context) == SymbolType::Equals))) {
    return -1;
  }

  vector<Token> _token_stream =
      vector(token_stream.begin() + 2, token_stream.end());

  Field field;
  field.identifier = get<1>(token_stream[0].token_context);
  if (0 > _parse_expressions(_token_stream, field.value)) {
  }

  return 0; // TODO
}

int parse_target(vector<Token> token_stream, AST &ast) {
  return 0; // TODO
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
