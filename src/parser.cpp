#include "parser.hpp"
#include "lexer.hpp"
#include <iostream>

Parser::Parser(std::vector<Token> token_stream) {
  m_token_stream = token_stream;
  m_index = 0;
  m_current = (m_token_stream.size() >= m_index + 1)
                  ? m_token_stream[m_index]
                  : Token{TokenType::Invalid, "__invalid__"};
  m_next = (m_token_stream.size() >= m_index + 2)
               ? m_token_stream[m_index]
               : Token{TokenType::Invalid, "__invalid__"};
}

Token Parser::advance_token() {
  m_index++;
  m_current = (m_token_stream.size() >= m_index + 1)
                  ? m_token_stream[m_index]
                  : Token{TokenType::Invalid, "__invalid__"};
  m_next = (m_token_stream.size() >= m_index + 2)
               ? m_token_stream[m_index]
               : Token{TokenType::Invalid, "__invalid__"};
  return m_current;
}

AST Parser::parse_tokens() {
  while (m_current.type != TokenType::Invalid) {
    if (0 == parse_variable())
      continue;
    if (0 == parse_target())
      continue;
    throw new ParserException("[P001] Unknown statement encountered.");
  }
  return m_ast;
}

int Parser::check_current(TokenType token_type) {
  return m_current.type == token_type;
}

int Parser::check_next(TokenType token_type) {
  return m_next.type == token_type;
}

// Retuns 0 if matched, -1 if not
int Parser::parse_target() {
  std::cerr << "niy parse_target" << std::endl;
}

// Retuns 0 if matched, -1 if not
int Parser::parse_variable() {
  std::cerr << "niy parse_variable" << std::endl;
}

Expression Parser::parse_replace() {
  if (check_current(TokenType::Identifier) && check_next(TokenType::Modify)) {

    // TODO
  }
}

//
// int parse_field(vector<Token>, AST &);
// int parse_target(vector<Token>, AST &);
//
// const vector<int (*)(vector<Token>, AST &)> parsing_rules{
//     parse_field,
//     parse_target,
// };
//
// // Tries to match against all rules, returns tokens parsed
// int try_parse(vector<Token> t_stream, AST &ast) {
//   // cout << ">>>> first token being parsed: " << (int)t_stream[0].type <<
//   endl; for (const auto &fn : parsing_rules) {
//     int tokens_parsed = fn(t_stream, ast);
//     if (0 < tokens_parsed) {
//       return tokens_parsed;
//     }
//   }
//   return 0;
// }
//
// AST Parser::parse_tokens(vector<Token> t_stream) {
//   AST ast;
//   for (int i = 0; i < t_stream.size();) {
//     vector<Token> _t_stream = vector(t_stream.begin() + i, t_stream.end());
//     int tokens_parsed = try_parse(_t_stream, ast);
//     if (0 < tokens_parsed) {
//       i += tokens_parsed;
//       continue;
//     }
//     throw ParserException("[P001] No matching parsing rules");
//   }
//   return ast;
// }
//
// // === All parsing logic below ===
// // NOTE: All parsing rules return the number of tokens parsed
//
// // Parses an expression, returns tokens parsed
// int _parse_expressions(vector<Token> t_stream,
//                        vector<Expression> &o_expression) {
//   int i = 0;
//   int concatenations = 0;
//   vector<Expression> _expression_buf;
//   while (true) {
//     // TODO: Match concatenations
//     if (t_stream[i].type == TokenType::Symbol &&
//         get<CTX_SYMBOL>(t_stream[i].context) == SymbolType::ConcatLiteral) {
//       i += 1; // TODO: Not optimal.
//       continue;
//     }
//
//     // Match separators ,
//     if (t_stream[i].type == TokenType::Symbol &&
//         get<CTX_SYMBOL>(t_stream[i].context) == SymbolType::Separator) {
//       if (_expression_buf.size() != 1) {
//         // FIXME: This could be incorrect (concatenations?)
//         return 0; // Incorrect syntax
//       }
//       o_expression.push_back(Expression{_expression_buf[0]});
//       _expression_buf.clear();
//       i += 1;
//       continue;
//     }
//
//     // Match linestop ;
//     if (t_stream[i].type == TokenType::Symbol &&
//         get<CTX_SYMBOL>(t_stream[i].context) == SymbolType::LineStop) {
//       // Everything is parsed up until the linestop
//       // TODO: Ensure that this is the correct return
//       if (_expression_buf.size() < 1) {
//         return 0;
//       }
//       for (auto const &expr : _expression_buf) {
//         // FIXME: This is incorrect! Do NOT push all of these! Only push the
//         1st
//         // element if there is one, and if there are more, they need to be
//         // concatenated
//         o_expression.push_back(expr);
//       }
//       return i + 1;
//     }
//
//     // Match variables
//     if (t_stream.size() >= 3 && t_stream[i].type == TokenType::Symbol &&
//         get<CTX_SYMBOL>(t_stream[i].context) == SymbolType::ExpressionOpen &&
//         t_stream[i + 1].type == TokenType::Identifier &&
//         t_stream[i + 2].type == TokenType::Symbol &&
//         get<CTX_SYMBOL>(t_stream[i + 2].context) ==
//             SymbolType::ExpressionClose) {
//       _expression_buf.push_back(
//           Expression(Variable{get<CTX_STRING>(t_stream[i + 1].context)}));
//       i += 3;
//       continue;
//     }
//
//     // Match replacements/modifies
//     if (t_stream.size() >= 7 && t_stream[i].type == TokenType::Symbol &&
//         get<CTX_SYMBOL>(t_stream[i].context) == SymbolType::ExpressionOpen &&
//         t_stream[i + 1].type == TokenType::Identifier &&
//         t_stream[i + 2].type == TokenType::Symbol &&
//         get<CTX_SYMBOL>(t_stream[i + 2].context) == SymbolType::Modify &&
//         t_stream[i + 3].type == TokenType::Literal &&
//         t_stream[i + 4].type == TokenType::Symbol &&
//         get<CTX_SYMBOL>(t_stream[i + 4].context) == SymbolType::Arrow &&
//         t_stream[i + 5].type == TokenType::Literal &&
//         t_stream[i + 6].type == TokenType::Symbol &&
//         get<CTX_SYMBOL>(t_stream[i + 6].context) ==
//             SymbolType::ExpressionClose) {
//       Variable variable = Variable{get<CTX_STRING>(t_stream[i + 1].context)};
//       Literal original = Literal{get<CTX_STRING>(t_stream[i + 3].context)};
//       Literal replacement = Literal{get<CTX_STRING>(t_stream[i +
//       5].context)}; _expression_buf.push_back(
//           Expression(Replace{variable, original, replacement}));
//       i += 7;
//       continue;
//     }
//
//     // Match simple literal
//     if (t_stream[i].type == TokenType::Literal) {
//       string literal_string = get<CTX_STRING>(t_stream[i].context);
//       _expression_buf.push_back(Expression(Literal{literal_string}));
//       i += 1;
//       continue;
//     }
//
//     if (i >= t_stream.size() - 1) {
//       // No more tokens to parse
//       cerr << "no more tokens to parse" << endl;
//       return 0;
//     }
//
//     // Nothing matched (?)
//     cerr << "nothing matched, o_expression.size() = " << o_expression.size()
//          << endl;
//     return 0;
//   }
// }
//
// // Parses a complete field, returns tokens parsed
// int _parse_field(vector<Token> t_stream, Field &field) {
//   // Verify the basic signature
//   if (t_stream.size() < 4) {
//     return 0;
//   }
//   if (!((t_stream[0].type == TokenType::Identifier) &&
//         (t_stream[1].type == TokenType::Symbol) &&
//         (get<CTX_SYMBOL>(t_stream[1].context) == SymbolType::Equals))) {
//     return 0;
//   }
//
//   vector<Token> _t_stream = vector(t_stream.begin() + 2, t_stream.end());
//
//   field.identifier = get<CTX_STRING>(t_stream[0].context);
//   int expression_length = _parse_expressions(_t_stream, field.value);
//   if (0 >= expression_length) {
//     throw ParserException("[P002] Field expression tokens invalid");
//   }
//
//   return expression_length + 2;
// }
//
// int parse_field(vector<Token> t_stream, AST &ast) {
//   Field field;
//   int parsed_tokens = _parse_field(t_stream, field);
//   if (0 >= parsed_tokens) {
//     return 0;
//   }
//   ast.fields.push_back(field);
//   return parsed_tokens;
// }
//
// // Parses a complete target, returns tokens parsed
// int parse_target(vector<Token> t_stream, AST &ast) {
//   if (t_stream.size() < 3) {
//     return 0;
//   }
//   Expression identifier;
//   string public_name;
//   // This is utter garbage. Terry Davis would be rolling in his grave if he
//   saw
//   // this. TODO: Comma Better identifier parsing needed!
//   // TODO2: This currently can't parse a collection of variables or
//   identifiers
//   // as a target. For instance, `obj1, obj2 as foo {...` wouldn't work.
//   int i = 0;
//   if (t_stream[0].type == TokenType::Identifier &&
//       t_stream[1].type == TokenType::Symbol &&
//       get<CTX_SYMBOL>(t_stream[1].context) == SymbolType::TargetOpen) {
//     identifier = Literal{get<CTX_STRING>(t_stream[0].context)};
//     public_name = get<CTX_STRING>(t_stream[0].context);
//     i = 2;
//   } else if (t_stream[0].type == TokenType::Identifier &&
//              t_stream[1].type == TokenType::Symbol &&
//              get<CTX_SYMBOL>(t_stream[1].context) == SymbolType::IterateAs &&
//              t_stream[2].type ==
//                  TokenType::Identifier && // FIXME: Is techincally literal
//              t_stream[3].type == TokenType::Symbol &&
//              get<CTX_SYMBOL>(t_stream[3].context) == SymbolType::TargetOpen)
//              {
//     identifier = Variable{get<CTX_STRING>(t_stream[0].context)};
//     public_name = get<CTX_STRING>(t_stream[2].context);
//     i = 4;
//   } else {
//     return 0;
//   }
//
//   std::vector<Field> fields;
//   for (; !(t_stream[i].type == TokenType::Symbol &&
//            get<CTX_SYMBOL>(t_stream[i].context) == SymbolType::TargetClose);)
//            {
//     vector<Token> _t_stream = vector(t_stream.begin() + i, t_stream.end());
//     Field field;
//     int parsed_tokens = _parse_field(_t_stream, field);
//     if (0 >= parsed_tokens) {
//       // Couldn't parse field
//       return 0;
//     } else {
//       fields.push_back(field);
//       i += parsed_tokens;
//     }
//
//     if (i >= t_stream.size() - 1) {
//       // No more tokens to parse
//       return 0;
//     }
//   }
//   ast.targets.push_back(Target{
//       identifier,
//       public_name,
//       fields,
//   });
//   return i + 2; // Account for targetclose and linestop
// }

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
