#include "parser.hpp"
#include "lexer.hpp"
#include <iostream>

using namespace std;

int parse_field(vector<Token>, AST &);
int parse_target(vector<Token>, AST &);

const vector<int (*)(vector<Token>, AST &)> parsing_rules{
    parse_field,
    parse_target,
};

// Tries to match against all rules, returns tokens parsed
int try_parse(vector<Token> token_stream, AST &ast) {
  for (const auto &fn : parsing_rules) {
    int tokens_parsed = fn(token_stream, ast);
    if (0 < tokens_parsed) {
      return tokens_parsed;
    }
  }
  return 0;
}

AST Parser::parse_tokens(vector<Token> token_stream) {
  AST ast;
  for (int index = 0; index < token_stream.size();) {
    vector<Token> _token_stream =
        vector(token_stream.begin() + index, token_stream.end());
    int tokens_parsed = try_parse(_token_stream, ast);
    if (0 < tokens_parsed) {
      index += tokens_parsed;
      continue;
    }
    throw ParserException("[P001] No matching parsing rules");
  }
  return ast;
}

// === All parsing logic below ===
// NOTE: All parsing rules return the number of tokens parsed

// Parses an expression, returns tokens parsed
int _parse_expressions(vector<Token> token_stream,
                       vector<Expression> &expression) {
  int token_index = 0;
  while (true) {
    // Match concatenations
    // TODO

    // Match replacements/modifies
    // TODO

    // Match simple literal
    if (token_stream[token_index].token_type == TokenType::Literal) {
       expression.push_back(Expression(Literal{get<CONTEXT_STRING>(token_stream[token_index].token_context)}));
    }

    // Match linestop ;
    if (token_stream[token_index].token_type == TokenType::Symbol &&
        get<CONTEXT_SYMBOLTYPE>(token_stream[token_index].token_context) ==
            SymbolType::LineStop) {
      // Everything is parsed up until the linestop
      // TODO: Ensure that this is the correct return
      return token_index + 1;
    }

    if (token_index >= token_stream.size() - 1) {
      // No more tokens to parse
      return 0;
    }
    // Nothing matched (?)
    return 0;
  }
}

// Parses a complete field, returns tokens parsed
int parse_field(vector<Token> token_stream, AST &ast) {
  // Verify the basic signature
  if (token_stream.size() < 4) {
    return -1;
  }
  if (!((token_stream[0].token_type == TokenType::Identifier) &&
        (token_stream[1].token_type == TokenType::Symbol) &&
        (get<CONTEXT_SYMBOLTYPE>(token_stream[1].token_context) == SymbolType::Equals))) {
    return -1;
  }

  vector<Token> _token_stream =
      vector(token_stream.begin() + 2, token_stream.end());

  Field field;
  field.identifier = get<CONTEXT_STRING>(token_stream[0].token_context);
  int expression_length = _parse_expressions(_token_stream, field.value);
  if (0 >= expression_length) {
    throw ParserException("[P002] Field expression tokens invalid");
  }

  ast.fields.push_back(field);
  return expression_length + 2;
}

// Parses a complete target, returns tokens parsed
int parse_target(vector<Token> token_stream, AST &ast) {
  return 0; // TODO
}

/* # Matching "rules":
 * field:
 *   identifier, equals, (expression), linestop
 * (expression):
 *   literal
 *   expressionopen, identifier, expressionclose
 *   expressionopen, identifier, modify, literal, arrow, literal,
 * expressionclose
 *   // or any of these combined
 *
 * # Assuming current config, the AST should look like the following:
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
