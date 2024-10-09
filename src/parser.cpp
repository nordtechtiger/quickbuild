# include "parser.hpp"
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
int try_parse(vector<Token> t_stream, AST &ast) {
  for (const auto &fn : parsing_rules) {
    int tokens_parsed = fn(t_stream, ast);
    if (0 < tokens_parsed) {
      return tokens_parsed;
    }
  }
  return 0;
}

AST Parser::parse_tokens(vector<Token> t_stream) {
  AST ast;
  for (int index = 0; index < t_stream.size();) {
    vector<Token> _t_stream = vector(t_stream.begin() + index, t_stream.end());
    int tokens_parsed = try_parse(_t_stream, ast);
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
int _parse_expressions(vector<Token> t_stream,
                       vector<Expression> &o_expression) {
  int i = 0;
  while (true) {
    // Match concatenations
    // TODO

    // Match replacements/modifies
    if (t_stream.size() >= 7 && t_stream[i].type == TokenType::Symbol &&
        get<CTX_SYMBOL>(t_stream[i].context) == SymbolType::ExpressionOpen &&
        t_stream[i + 1].type == TokenType::Identifier &&
        t_stream[i + 2].type == TokenType::Symbol &&
        get<CTX_SYMBOL>(t_stream[i + 2].context) == SymbolType::Modify &&
        t_stream[i + 3].type == TokenType::Literal &&
        t_stream[i + 4].type == TokenType::Symbol &&
        get<CTX_SYMBOL>(t_stream[i + 4].context) == SymbolType::Arrow &&
        t_stream[i + 5].type == TokenType::Literal &&
        t_stream[i + 6].type == TokenType::Symbol &&
        get<CTX_SYMBOL>(t_stream[i + 6].context) ==
            SymbolType::ExpressionClose) {
      Variable variable = Variable{get<CTX_STRING>(t_stream[i + 1].context)};
      Literal original = Literal{get<CTX_STRING>(t_stream[i + 3].context)};
      Literal replacement = Literal{get<CTX_STRING>(t_stream[i + 5].context)};
      o_expression.push_back(
          Expression(Replace{variable, original, replacement}));
      i += 7;
      cout << "Matched replacement expression" << endl;
      continue;
    }

    // Match simple literal
    if (t_stream[i].type == TokenType::Literal) {
      string literal_string = get<CTX_STRING>(t_stream[i].context);
      o_expression.push_back(Expression(Literal{literal_string}));
      i += 1;
      cout << "Matched literal expression" << endl;
      continue;
    }

    // Match linestop ;
    if (t_stream[i].type == TokenType::Symbol &&
        get<CTX_SYMBOL>(t_stream[i].context) == SymbolType::LineStop) {
      // Everything is parsed up until the linestop
      // TODO: Ensure that this is the correct return
      return i + 1;
    }

    if (i >= t_stream.size() - 1) {
      // No more tokens to parse
      return 0;
    }
    // Nothing matched (?)
    return 0;
  }
}

// Parses a complete field, returns tokens parsed
int parse_field(vector<Token> t_stream, AST &ast) {
  // Verify the basic signature
  if (t_stream.size() < 4) {
    return -1;
  }
  if (!((t_stream[0].type == TokenType::Identifier) &&
        (t_stream[1].type == TokenType::Symbol) &&
        (get<CTX_SYMBOL>(t_stream[1].context) == SymbolType::Equals))) {
    return -1;
  }

  vector<Token> _t_stream = vector(t_stream.begin() + 2, t_stream.end());

  Field field;
  field.identifier = get<CTX_STRING>(t_stream[0].context);
  int expression_length = _parse_expressions(_t_stream, field.value);
  if (0 >= expression_length) {
    throw ParserException("[P002] Field expression tokens invalid");
  }

  ast.fields.push_back(field);
  return expression_length + 2;
}

// Parses a complete target, returns tokens parsed
int parse_target(vector<Token> t_stream, AST &ast) {
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
