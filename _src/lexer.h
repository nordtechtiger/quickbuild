#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

enum TokenType {
  Identifier,      // any text without quotes
  Literal,         // any text in quotes
  Operation,       // `=`, `:`, `->`,
  Separator,       // ','
  LineStop,        // ';`
  ExpressionOpen,  // `[`
  ExpressionClose, // `]`
  TargetOpen,      // `{`
  TargetClose,     // `}`
};

enum OperatorType {
  Set,     // `=`
  Modify,  // `:`
  Replace, // `->`
  Iterate, // `as`
};

struct Token {
  enum TokenType token_type;
  union TokenData {
    enum OperatorType operator_type;
    char *string;
  } token_data;
  struct Token *child;
};

struct Token *lex_bytes(char *bytes, uint32_t len);

#endif

/* QuickBuild config:
 *
 * source_dir = "src";
 * build_dir = "bin";
 * sources = "[source_dir]/*.c";
 * objects = [
 *   sources: "[source_dir]/*.c" -> "[build_dir]/*.o"
 * ];
 * /

/* Lexer output:
 *
 * (identifier, str) (operator, =) (literal, str) (linestop, )
 * (identifier, str) (operator, =) (literal, str) (linestop, )
 * (identifier, str) (operator, =) (expressionopen, ) (identifier, str)
(expressionclose, ) (literal, str) (linestop, )
 * (identifier, str) (operator, =) (expressionopen, ) (identifier, str)
(operation, :) (expressionopen, ) (identifier, str) (expressionclose) (literal,
str) (operator, ->) (expressionopen, ) (identifier, str) (expressionclose, )
(literal, str) (expressionclose, ), (linestop, 0)
 */
