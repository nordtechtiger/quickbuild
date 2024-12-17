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
#include <optional>
#include <string>
#include <vector>

// Defines what type of token it is
enum class TokenType {
  Identifier,      // any text without quotes
  Literal,         // any text in quotes
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
  Invalid,         // internal return type in parser
};

enum class LexerState {
  Normal,
  EscapedLiteral,
};

// Defines a general token
struct Token {
  TokenType type;
  std::optional<std::string> context;
  size_t origin; // Index in original ascii stream
};

// Work class
class Lexer {
private:
  std::vector<unsigned char> m_input;
  std::vector<Token> m_t_stream;
  LexerState m_state;

  unsigned char m_current;
  unsigned char m_next;
  size_t m_index;
  size_t m_offset; // When bytes are added to input, append this so that the
                   // origin can be correctly calculated
  unsigned char advance_input_byte();
  size_t get_real_offset();
  void insert_next_byte(unsigned char);

  FUNCTION_DECLARE_ALL
  std::vector<std::function<int(void)>> matching_rules{LAMBDA_DECLARE_ALL};

public:
  Lexer(std::vector<unsigned char> input_bytes);
  std::vector<Token> get_token_stream();
};

#endif
