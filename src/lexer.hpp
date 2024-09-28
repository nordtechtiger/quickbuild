#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <variant>
#include <vector>

#define TOKEN_INVALID                                                          \
  Token { TokenType::Invalid, SymbolType::LineStop }

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
  ConcatLiteral,   // internal token for escaped expressions inside literals
};

enum class LexerState {
  Normal,
  EscapedLiteral,
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
  std::vector<unsigned char> m_input;

public:
  Lexer(std::vector<unsigned char> input_bytes);
  std::vector<Token> get_token_stream();
  unsigned char m_next;
  unsigned char m_current;
  unsigned long long m_index;
  unsigned char advance_input_byte();
  void insert_next_byte(unsigned char);
  LexerState m_state;
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
