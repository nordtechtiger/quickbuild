#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <variant>
#include <vector>

// Defines what type of token it is
enum class TokenType {
  Identifier, // any text without quotes
  Literal,    // any text in quotes
  Symbol,     // `=`, `:`, `->`,
  Invalid,
};

// Defines the specific operator
enum class SymbolType {
  Equals,          // `=`
  Modify,          // `:`
  LineStop,        // ';`
  Arrow,           // `->`
  IterateAs,       // `as`
  Separator,       // ','
  ExpressionOpen,  // `[`
  ExpressionClose, // `]`
  TargetOpen,      // `{`
  TargetClose,     // `}`
};

// Defines additional data depending on the token type
typedef std::variant<SymbolType, std::string> TokenContext;

// Defines a general token, struct Token {
struct Token {
  TokenType token_type;
  TokenContext token_context;
};

// Work class
class Lexer {
private:
  std::vector<unsigned char> m_input_bytes;
  unsigned long long m_input_index;

public:
  Token get_next_token();
  unsigned char m_peek_next_byte;
  unsigned char m_peek_current_byte;
  unsigned char advance_input_byte();
  Lexer(std::vector<unsigned char> input_bytes);
};

// Exceptions thrown by the lexer
class LexerException : public std::exception {
private:
  const char *details;

public:
  LexerException(const char *details) : details(details) {};
  const char *what() { return details; }
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
