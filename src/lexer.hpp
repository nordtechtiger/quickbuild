#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

// Defines what type of token it is
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

// Defines the specific operator
enum OperatorType {
  Set,     // `=`
  Modify,  // `:`
  Replace, // `->`
  Iterate, // `as`
};

// Defines a general token, with optional data depending on the token type
struct Token {
  TokenType token_type;
  union {
    OperatorType operator_type;
    std::string data;
  } token_context;
};

// Work class
class Lexer {
private:
  enum {
    NONE,
    IN_LITERAL,
    IN_STRING,
  } lex_state;

public:
  std::vector<Token> lex_bytes(std::vector<unsigned char> input_bytes);
};

// Exceptions thrown by the lexer
class LexerException : public std::exception {
private:
  char *details;

public:
  LexerException(char *details) : details(details) {};
  char *what() { return details; }
};

#endif

/* === LEXER NOTES / SPECIFICATIONS ===
 * QuickBuild config:
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
