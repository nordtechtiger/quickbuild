#include "interpreter.hpp"
#include "filesystem"
#include "format.hpp"

// constructors & casts for internal data types.
QBString::QBString() {
  this->origin = ORIGIN_UNDEFINED;
  this->content = "";
}
QBString::QBString(Token token) {
  if (token.type != TokenType::Literal)
    ErrorHandler::push_error_throw(token.origin,
                                   I_CONSTRUCTOR_EXPECTED_LITERAL);
  this->origin = token.origin;
  this->content = std::get<CTX_STR>(*token.context);
};
QBString::QBString(std::string content, Origin origin) {
  this->origin = origin;
  this->content = content;
}
QBBool::QBBool() {
  this->origin = ORIGIN_UNDEFINED;
  this->content = false;
}
QBBool::QBBool(Token token) {
  if (token.type != TokenType::True && token.type != TokenType::False)
    ErrorHandler::push_error_throw(token.origin, I_CONSTRUCTOR_EXPECTED_BOOL);
  this->origin = token.origin;
  this->content = (token.type == TokenType::True);
}
QBBool::QBBool(bool content, Origin origin) {
  this->origin = origin;
  this->content = content;
}
QBList::QBList() {
  this->origin = ORIGIN_UNDEFINED;
  this->contents = std::vector<QBString>();
}
QBList::QBList(
    std::variant<std::vector<QBString>, std::vector<QBBool>> contents) {
  if (std::holds_alternative<std::vector<QBString>>(contents)) {
    std::vector<QBString> contents_qbstring =
        std::get<std::vector<QBString>>(contents);
    if (contents_qbstring.size() <= 0)
      ErrorHandler::push_error_throw(ORIGIN_UNDEFINED,
                                     I_CONSTRUCTOR_EXPECTED_NONEMPTY);
    this->contents = contents_qbstring;
    this->origin = contents_qbstring[0].origin;
  } else if (std::holds_alternative<std::vector<QBBool>>(contents)) {
    std::vector<QBBool> contents_qbbool =
        std::get<std::vector<QBBool>>(contents);
    if (contents_qbbool.size() <= 0)
      ErrorHandler::push_error_throw(ORIGIN_UNDEFINED,
                                     I_CONSTRUCTOR_EXPECTED_NONEMPTY);
    this->contents = contents_qbbool;
    this->origin = contents_qbbool[0].origin;
  }
}
std::string QBString::toString() { return (this->content); };
QBBool::operator bool() { return (this->content); };
bool QBList::holds_qbstring() { return (this->contents.index() == QBLIST_STR); }
bool QBList::holds_qbbool() { return (this->contents.index() == QBLIST_BOOL); }

// visitor that evaluates an AST object recursively.
struct ASTVisitEvaluate {
  AST ast;
  EvaluationContext context;
  EvaluationResult operator()(Identifier const &identifier);
  EvaluationResult operator()(Literal const &literal);
  EvaluationResult operator()(FormattedLiteral const &formatted_literal);
  EvaluationResult operator()(List const &list);
  EvaluationResult operator()(Boolean const &boolean);
  EvaluationResult operator()(Replace const &replace);
};

EvaluationResult ASTVisitEvaluate::operator()(Identifier const &identifier) {
  // target-specific fields.
  if (context.target_scope)
    for (Field const &field : context.target_scope->fields)
      if (field.identifier.content == identifier.content)
        return std::visit(ASTVisitEvaluate{ast, context}, field.expression);

  // target iteration variable.
  if (context.target_iteration && context.target_scope)
    if (context.target_scope->iterator.content == identifier.content)
      return QBString(*context.target_iteration, context.target_scope->origin);

  // global fields.
  for (Field const &field : ast.fields) {
    if (field.identifier.content == identifier.content)
      return std::visit(
          ASTVisitEvaluate{ast, EvaluationContext{std::nullopt, std::nullopt}},
          field.expression);
  }

  // identifier not found.
  ErrorHandler::push_error_throw(identifier.origin, I_NO_MATCHING_IDENTIFIER);
  __builtin_unreachable();
}

// note: if literal includes a `*`, globbing will be used - this is expensive.
EvaluationResult ASTVisitEvaluate::operator()(Literal const &literal) {
  size_t i_asterisk = literal.content.find('*');
  if (i_asterisk == std::string::npos) // no globbing.
    return QBString(literal.content, literal.origin);

  // globbing is required.
  std::string prefix = literal.content.substr(0, i_asterisk);
  std::string suffix = literal.content.substr(i_asterisk + 1);

  // acts as a string vector.
  QBList matching_paths;
  for (std::filesystem::directory_entry const &dir_entry :
       std::filesystem::recursive_directory_iterator()) {
    std::string dir_path = dir_entry.path().string();
    size_t i_prefix = dir_path.find(prefix);
    size_t i_suffix = dir_path.find(suffix);
    if (prefix.empty() && i_suffix != std::string::npos)
      std::get<QBLIST_STR>(matching_paths.contents)
          .push_back(QBString(dir_path, literal.origin));
    else if (suffix.empty() && i_prefix != std::string::npos)
      std::get<QBLIST_STR>(matching_paths.contents)
          .push_back(QBString(dir_path, literal.origin));
    else if (!prefix.empty() && !suffix.empty() &&
             i_prefix != std::string::npos && i_suffix != std::string::npos &&
             i_prefix < i_suffix)
      std::get<QBLIST_STR>(matching_paths.contents)
          .push_back(QBString(dir_path, literal.origin));
  }

  if (std::get<QBLIST_STR>(matching_paths.contents).size() > 0)
    matching_paths.origin =
        std::get<QBLIST_STR>(matching_paths.contents)[0].origin;

  return matching_paths;
};

EvaluationResult
ASTVisitEvaluate::operator()(FormattedLiteral const &formatted_literal) {
  QBString out;
  for (ASTObject const &ast_obj : formatted_literal.contents) {
    EvaluationResult obj_result =
        std::visit(ASTVisitEvaluate{ast, context}, ast_obj);
    // append a string.
    if (std::holds_alternative<QBString>(obj_result)) {
      if (out.origin == ORIGIN_UNDEFINED)
        out.origin = std::get<QBString>(obj_result).origin;
      out.content += std::get<QBString>(obj_result).content;
    }
    // append a bool.
    else if (std::holds_alternative<QBBool>(obj_result)) {
      if (out.origin == ORIGIN_UNDEFINED)
        out.origin = std::get<QBBool>(obj_result).origin;
      out.content += (std::get<QBBool>(obj_result) ? "true" : "false");
    }
    // append a list of strings.
    else if (std::holds_alternative<QBList>(obj_result) &&
             std::get<QBList>(obj_result).contents.index() == QBLIST_STR) {
      QBList obj_result_list = std::get<QBList>(obj_result);
      for (size_t i = 0;
           i < std::get<QBLIST_STR>(obj_result_list.contents).size(); i++) {
        if (out.origin == ORIGIN_UNDEFINED)
          out.origin = std::get<QBLIST_STR>(obj_result_list.contents)[i].origin;
        out.content +=
            std::get<QBLIST_STR>(obj_result_list.contents)[i].content;
        if (i < std::get<QBLIST_STR>(obj_result_list.contents).size() - 1)
          out.content.append(" ");
      }
    }
    // append a list of bools.
    else if (std::holds_alternative<QBList>(obj_result) &&
             std::get<QBList>(obj_result).contents.index() == QBLIST_BOOL) {
      QBList obj_result_list = std::get<QBList>(obj_result);
      for (size_t i = 0;
           i < std::get<QBLIST_BOOL>(obj_result_list.contents).size(); i++) {
        if (out.origin == ORIGIN_UNDEFINED)
          out.origin =
              std::get<QBLIST_BOOL>(obj_result_list.contents)[i].origin;
        out.content +=
            (std::get<QBLIST_BOOL>(obj_result_list.contents)[i] ? "true"
                                                                : "false");
        if (i < std::get<QBLIST_BOOL>(obj_result_list.contents).size() - 1)
          out.content.append(" ");
      }
    }
  }
  return out;
}

EvaluationResult ASTVisitEvaluate::operator()(List const &list) {
  if (list.contents.size() <= 0)
    ErrorHandler::push_error_throw(ORIGIN_UNDEFINED,
                                   I_EVALUATE_EXPECTED_NONEMPTY);

  QBList out;
  // infer the list type. todo: consider a cleaner solution.
  EvaluationResult _obj_result =
      std::visit(ASTVisitEvaluate{ast, context}, list.contents[0]);
  if (std::holds_alternative<QBString>(_obj_result))
    out.contents = std::vector<QBString>();
  else if (std::holds_alternative<QBBool>(_obj_result))
    out.contents = std::vector<QBBool>();
  else if (std::holds_alternative<QBList>(_obj_result)) {
    QBList __obj_qblist = std::get<QBList>(_obj_result);
    if (std::holds_alternative<std::vector<QBString>>(__obj_qblist.contents))
      out.contents = std::vector<QBString>();
    else if (std::holds_alternative<std::vector<QBBool>>(__obj_qblist.contents))
      out.contents = std::vector<QBBool>();
  }

  for (ASTObject const &ast_obj : list.contents) {
    EvaluationResult obj_result =
        std::visit(ASTVisitEvaluate{ast, context}, ast_obj);
    if (std::holds_alternative<QBString>(obj_result)) {
      if (out.holds_qbstring())
        std::get<QBLIST_STR>(out.contents)
            .push_back(std::get<QBString>(obj_result));
      else
        ErrorHandler::push_error_throw(std::get<QBString>(obj_result).origin,
                                       I_EVALUATE_QBSTRING_IN_QBBOOL_LIST);
    } else if (std::holds_alternative<QBBool>(obj_result)) {
      if (out.holds_qbbool())
        std::get<QBLIST_BOOL>(out.contents)
            .push_back(std::get<QBBool>(obj_result));
      else
        ErrorHandler::push_error_throw(std::get<QBBool>(obj_result).origin,
                                       I_EVALUATE_QBBOOL_IN_QBSTRING_LIST);
    } else if (std::holds_alternative<QBList>(obj_result)) {
      QBList obj_result_qblist = std::get<QBList>(obj_result);
      if (obj_result_qblist.holds_qbstring() && out.holds_qbstring()) {
        std::get<QBLIST_STR>(out.contents)
            .insert(std::get<QBLIST_STR>(out.contents).begin(),
                    std::get<QBLIST_STR>(obj_result_qblist.contents).begin(),
                    std::get<QBLIST_STR>(obj_result_qblist.contents).end());
      } else if (obj_result_qblist.holds_qbbool() && out.holds_qbbool()) {
        std::get<QBLIST_BOOL>(out.contents)
            .insert(std::get<QBLIST_BOOL>(out.contents).begin(),
                    std::get<QBLIST_BOOL>(obj_result_qblist.contents).begin(),
                    std::get<QBLIST_BOOL>(obj_result_qblist.contents).end());

      } else {
        ErrorHandler::push_error_throw(obj_result_qblist.origin,
                                       I_EVALUATE_LIST_TYPE_MISMATCH);
      }
    }
  }
  return out;
};

EvaluationResult ASTVisitEvaluate::operator()(Boolean const &boolean) {
  return QBBool(boolean.content, boolean.origin);
};

// note: this only supports a single wildcard. consider expanding.
EvaluationResult ASTVisitEvaluate::operator()(Replace const &replace) {
  std::cerr << "why u tryna evaluate a replace bruh" << std::endl;
  exit(-1);
};

Interpreter::Interpreter(AST ast, Setup setup) {
  m_ast = ast;
  m_setup = setup;
}

void Interpreter::build() {

  // find the target.
  if (m_ast.targets.empty())
    ErrorHandler::push_error_throw(Origin{0, 0}, I_NO_TARGETS);

  ASTObject foo = Identifier {"sources", ORIGIN_UNDEFINED};
  EvaluationResult result = std::visit(ASTVisitEvaluate {m_ast, EvaluationContext{std::nullopt, std::nullopt}}, foo);
  // result 

  LOG_STANDARD("= building " << GREEN);
}
