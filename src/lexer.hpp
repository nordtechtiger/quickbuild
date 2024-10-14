#ifndef LEXER_H
#define LEXER_H

// Token context indexes
#define CTX_SYMBOL 0
#define CTX_STRING 1

#define LEXING_RULES(_MACRO)                                                   \
  _MACRO(skip_whitespace)                                                      \
  _MACRO(skip_comments)                                                        \
  _MACRO(match_equals)                                                         \
  _MACRO(match_modify)                                                         \
  _MACRO(match_linestop)                                                       \
  _MACRO(match_arrow)                                                          \
  _MACRO(match_iterateas)                                                      \
  _MACRO(match_separator)                                                      \
  _MACRO(match_expressionopen)                                                 \
  _MACRO(match_expressionclose)                                                \
  _MACRO(match_targetopen)                                                     \
  _MACRO(match_targetclose)                                                    \
  _MACRO(match_literal)                                                        \
  _MACRO(match_identifier)

#define _FUNCTION_DECLARE(x) int x();
#define FUNCTION_DECLARE_ALL LEXING_RULES(_FUNCTION_DECLARE)

#define _LAMBDA_DECLARE(x) [this]() { return x(); }
#define _LAMBDA_DECLARE_LIST(x) _LAMBDA_DECLARE(x),
#define LAMBDA_DECLARE_ALL LEXING_RULES(_LAMBDA_DECLARE_LIST)

#define STRINGIFY(x) #x
#define STRINGIFY_MACRO(x) STRINGIFY(x)

#include <functional>
#include <string>
#include <variant>
#include <vector>

// Defines what type of token it is
enum class TokenType {
  Identifier, // any text without quotes
  Literal,    // any text in quotes
  Symbol,     // `=`, `:`, `->`,
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

// Defines a general token
struct Token {
  TokenType type;
  TokenContext context;
};

// Work class
class Lexer {
private:
  std::vector<unsigned char> m_input;
  std::vector<Token> m_t_stream;
  LexerState m_state;

  unsigned char m_next;
  unsigned char m_current;
  unsigned long long m_index;
  unsigned char advance_input_byte();
  void insert_next_byte(unsigned char);

  FUNCTION_DECLARE_ALL
  std::vector<std::function<int(void)>> matching_rules{LAMBDA_DECLARE_ALL};

public:
  Lexer(std::vector<unsigned char> input_bytes);
  std::vector<Token> get_token_stream();
};

// Exceptions thrown by the lexer
class LexerException : public std::exception {
private:
  const char *details;

public:
  LexerException(const char *details) : details(details){};
  const char *what() { return details; }
};

#endif
