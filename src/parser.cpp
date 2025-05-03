#include "parser.hpp"
#include "errors.hpp"
#include "lexer.hpp"
#include <memory>

#define ITERATOR_INTERNAL                                                      \
  Identifier {                                                                 \
    "__target__", InternalNode {}                                              \
  }

// equality operators for AST objects.
bool Identifier::operator==(Identifier const &other) const {
  // return this->content == other.content && this->origin == other.origin;
  return this->content == other.content;
}
bool Literal::operator==(Literal const &other) const {
  return this->content == other.content;
}
bool FormattedLiteral::operator==(FormattedLiteral const &other) const {
  return this->contents == other.contents;
}
bool Boolean::operator==(Boolean const &other) const {
  return this->content == other.content;
}
bool Replace::operator==(Replace const &other) const {
  return this->identifier == other.identifier &&
         *(this->original) == *(other.original) &&
         *(this->replacement) == *(other.replacement);
}
bool List::operator==(List const &other) const {
  return this->contents == other.contents;
}

// visitor that simply returns the origin of an AST object.
struct ASTVisitOrigin {
  Origin operator()(Identifier const &identifier) { return identifier.origin; }
  Origin operator()(Literal const &literal) { return literal.origin; }
  Origin operator()(FormattedLiteral const &formatted_literal) {
    return formatted_literal.origin;
  }
  Origin operator()(List const &list) { return list.origin; }
  Origin operator()(Boolean const &boolean) { return boolean.origin; }
  Origin operator()(Replace const &replace) { return replace.origin; }
};

// initialises fields.
Parser::Parser(std::vector<Token> token_stream) : m_ast() {
  m_token_stream = token_stream;
  m_index = 0;
  // the origin should never be read, so we can keep this as an internalnode.
  m_current = (m_token_stream.size() >= m_index + 1)
                  ? m_token_stream[m_index]
                  : Token{TokenType::Invalid, "__invalid__", InternalNode{}};
  m_next = (m_token_stream.size() >= m_index + 2)
               ? m_token_stream[m_index + 1]
               : Token{TokenType::Invalid, "__invalid__", InternalNode{}};
}

// token checking, no side effects.
bool Parser::check_current(TokenType token_type) {
  return m_current.type == token_type;
}

// token checking, no side effects.
bool Parser::check_next(TokenType token_type) {
  return m_next.type == token_type;
}

// consume a token.
Token Parser::consume_token() { return consume_token(1); }

// consume n tokens.
Token Parser::consume_token(int n) {
  m_index += n;
  m_previous = m_current;
  // the origin should never be read, so we can keep it as 0.
  m_current = (m_token_stream.size() >= m_index + 1)
                  ? m_token_stream[m_index]
                  : Token{TokenType::Invalid, "__invalid__", InternalNode{}};
  m_next = (m_token_stream.size() >= m_index + 2)
               ? m_token_stream[m_index + 1]
               : Token{TokenType::Invalid, "__invalid__", InternalNode{}};
  return m_previous;
}

// consume a token if the type matches.
std::optional<Token> Parser::consume_if(TokenType token_type) {
  if (check_current(token_type))
    return consume_token();
  return std::nullopt;
}

// parses the entire token stream.
AST Parser::parse_tokens() {
  AST ast;
  while (!check_current(TokenType::Invalid)) {
    std::optional<Field> field = parse_field();
    if (field) {
      ast.fields.push_back(*field);
      continue;
    }
    std::optional<Target> target = parse_target();
    if (target) {
      ast.targets.push_back(*target);
      continue;
    }
    ErrorHandler::push_error_throw(m_current.origin, P_NO_MATCH);
  }
  return AST(ast);
}

// attempts to parse a field.
std::optional<Field> Parser::parse_field() {
  if (!check_current(TokenType::Identifier) || !check_next(TokenType::Equals))
    return std::nullopt;

  Field field;
  Token identifier_token = consume_token();
  field.identifier = Identifier{std::get<CTX_STR>(*identifier_token.context),
                                identifier_token.origin};
  field.origin = identifier_token.origin;
  consume_token(); // consume the `=`.

  std::optional<ASTObject> ast_object = parse_ast_object();
  if (!ast_object)
    ErrorHandler::push_error_throw(field.origin, P_FIELD_NO_EXPR);

  field.expression = *ast_object;
  if (!consume_if(TokenType::LineStop))
    ErrorHandler::push_error_throw(field.origin, P_FIELD_NO_LINESTOP);
  return field;
}

// attempts to parse a target.
std::optional<Target> Parser::parse_target() {
  std::optional<ASTObject> identifier = parse_ast_object();
  Identifier iterator = ITERATOR_INTERNAL;
  if (!identifier)
    return std::nullopt;
  Origin origin = std::visit(ASTVisitOrigin{}, *identifier);
  // check if an explicit iterator name has been declared.
  if (consume_if(TokenType::IterateAs)) {
    std::optional<Token> iterator_token = consume_if(TokenType::Identifier);
    if (!iterator_token)
      ErrorHandler::push_error_throw(origin, P_TARGET_NO_ITERATOR);
    iterator = Identifier{std::get<CTX_STR>(*iterator_token->context),
                          iterator_token->origin};
  }
  if (!consume_if(TokenType::TargetOpen))
    ErrorHandler::push_error_throw(origin, P_TARGET_NO_OPEN);

  Target target;
  target.identifier = *identifier;
  target.iterator = iterator;
  target.origin = origin;

  std::optional<Field> field;
  while ((field = parse_field()))
    target.fields.push_back(*field);

  if (!consume_if(TokenType::TargetClose))
    ErrorHandler::push_error_throw(origin, P_TARGET_NO_CLOSE);

  return target;
}

/*
 * grammar:
 * ========
 * ASTOBJ -> LIST
 * LIST -> REPLACE ("," REPLACE)*
 * REPLACE -> (PRIMARY ":" PRIMARY "->" PRIMARY) | PRIMARY
 * PRIMARY -> token_literal | token_formattedliteral | token_identifier |
 *            token_true | token_false | ("[" ASTOBJ "]")
 */

std::optional<ASTObject> Parser::parse_ast_object() { return parse_list(); }

// recursive descent parser, see grammar.
std::optional<ASTObject> Parser::parse_list() {
  List list;
  list.origin = InternalNode{};
  std::optional<ASTObject> ast_obj;
  ast_obj = parse_replace();
  while (ast_obj && consume_if(TokenType::Separator)) {
    if (std::holds_alternative<InternalNode>(list.origin))
      list.origin = std::visit(ASTVisitOrigin{}, *ast_obj);
    list.contents.push_back(*ast_obj);
    ast_obj = parse_ast_object();
  }

  if (!ast_obj && !list.contents.empty())
    ErrorHandler::push_error_throw(
      std::visit(ASTVisitOrigin{}, list.contents[0]), P_AST_INVALID_END);
  else if (!ast_obj && list.contents.empty())
    return std::nullopt;

  list.contents.push_back(*ast_obj);
  if (std::holds_alternative<InternalNode>(list.origin))
    list.origin = std::visit(ASTVisitOrigin{}, *ast_obj);

  if (list.contents.size() == 1)
    return list.contents[0];

  return list;
}

// recursive descent parser, see grammar.
std::optional<ASTObject> Parser::parse_replace() {
  std::optional<ASTObject> identifier = parse_primary();
  if (!consume_if(TokenType::Modify))
    return identifier; // not a replace.
  Origin origin = std::visit(ASTVisitOrigin{}, *identifier);

  std::optional<ASTObject> original = parse_primary();
  if (!consume_if(TokenType::Arrow))
    ErrorHandler::push_error_throw(origin, P_AST_NO_ARROW);

  std::optional<ASTObject> replacement = parse_primary();
  if (!identifier || !original || !replacement)
    ErrorHandler::push_error_throw(origin, P_AST_INVALID_REPLACE);

  return Replace{
      std::make_shared<ASTObject>(*identifier),
      std::make_shared<ASTObject>(*original),
      std::make_shared<ASTObject>(*replacement),
      origin,
  };
}

// recursive descent parser, see grammar.
std::optional<ASTObject> Parser::parse_primary() {
  std::optional<Token> token;
  if ((token = consume_if(TokenType::Literal)))
    return Literal{std::get<CTX_STR>(*token->context), token->origin};
  else if ((token = consume_if(TokenType::Identifier)))
    return Identifier{std::get<CTX_STR>(*token->context), token->origin};
  else if ((token = consume_if(TokenType::True)))
    return Boolean{true, token->origin};
  else if ((token = consume_if(TokenType::False)))
    return Boolean{false, token->origin};
  else if ((token = consume_if(TokenType::FormattedLiteral))) {
    FormattedLiteral formattedLiteral;
    std::vector<Token> internal_token_stream =
        std::get<CTX_VEC>(*token->context);
    formattedLiteral.origin = internal_token_stream[0].origin;
    // note: only identifiers and literals may be present.
    for (size_t i = 0; i < internal_token_stream.size(); i++) {
      Token internal_token = internal_token_stream[i];
      if (internal_token.type == TokenType::Literal)
        formattedLiteral.contents.push_back(Literal{
            std::get<CTX_STR>(*internal_token.context), internal_token.origin});
      else if (internal_token.type == TokenType::Identifier)
        formattedLiteral.contents.push_back(Identifier{
            std::get<CTX_STR>(*internal_token.context), internal_token.origin});
      else
        ErrorHandler::push_error_throw(internal_token.origin,
                                       P_AST_INVALID_ESCAPE);
    }
    return formattedLiteral;
  } else if ((token = consume_if(TokenType::ExpressionOpen))) {
    std::optional<ASTObject> ast_object = parse_ast_object();
    if (!consume_if(TokenType::ExpressionClose))
      ErrorHandler::push_error_throw(std::visit(ASTVisitOrigin{}, *ast_object),
                                     P_AST_NO_CLOSE);
    if (!ast_object)
      ErrorHandler::push_error(token->origin, P_EMPTY_EXPRESSION);
      
    return ast_object;
  }

  return std::nullopt;
}
