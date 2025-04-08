#include "interpreter.hpp"
#include "filesystem"
#include "format.hpp"
#include "shell.hpp"

#define IDENTIFIER_DEPENDS                                                     \
  Identifier { "depends", ORIGIN_UNDEFINED }
#define IDENTIFIER_RUN                                                         \
  Identifier { "run", ORIGIN_UNDEFINED }

struct QBVisitOrigin {
  Origin operator()(QBString qbstring) { return qbstring.origin; };
  Origin operator()(QBBool qbbool) { return qbbool.origin; };
  Origin operator()(QBList qblist) { return qblist.origin; };
};

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
std::string QBString::toString() const { return (this->content); };
bool QBString::operator==(QBString const other) const {
  return this->content == other.content;
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
bool QBBool::operator==(QBBool const other) const {
  return this->content == other.content;
}
QBBool::operator bool() const { return (this->content); };

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
bool QBList::holds_qbstring() const {
  return (this->contents.index() == QBLIST_STR);
}
bool QBList::holds_qbbool() const {
  return (this->contents.index() == QBLIST_BOOL);
}
bool QBList::operator==(QBList const other) const {
  return this->contents == other.contents;
}

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

// note: we handle globbing **after** evaluating a formatted literal.
EvaluationResult ASTVisitEvaluate::operator()(Literal const &literal) {
  return QBString(literal.content, literal.origin);
};

// helper method: handles globbing.
EvaluationResult expand_literal(QBString input_qbstring) {
  size_t i_asterisk = input_qbstring.content.find('*');
  if (i_asterisk == std::string::npos) // no globbing.
    return input_qbstring;

  // globbing is required.
  std::string prefix = input_qbstring.content.substr(0, i_asterisk);
  std::string suffix = input_qbstring.content.substr(i_asterisk + 1);

  // acts as a string vector.
  QBList matching_paths;
  for (std::filesystem::directory_entry const &dir_entry :
       std::filesystem::recursive_directory_iterator(".")) {
    std::string dir_path = dir_entry.path().string();
    size_t i_prefix = dir_path.find(prefix);
    size_t i_suffix = dir_path.find(suffix);
    if (prefix.empty() && i_suffix != std::string::npos)
      std::get<QBLIST_STR>(matching_paths.contents)
          .push_back(QBString(dir_path, input_qbstring.origin));
    else if (suffix.empty() && i_prefix != std::string::npos)
      std::get<QBLIST_STR>(matching_paths.contents)
          .push_back(QBString(dir_path, input_qbstring.origin));
    else if (!prefix.empty() && !suffix.empty() &&
             i_prefix != std::string::npos && i_suffix != std::string::npos &&
             i_prefix < i_suffix)
      std::get<QBLIST_STR>(matching_paths.contents)
          .push_back(QBString(dir_path, input_qbstring.origin));
  }

  if (std::get<QBLIST_STR>(matching_paths.contents).size() > 0)
    matching_paths.origin =
        std::get<QBLIST_STR>(matching_paths.contents)[0].origin;

  if (std::get<QBLIST_STR>(matching_paths.contents).size() == 1)
    return std::get<QBLIST_STR>(matching_paths.contents)[0];

  return matching_paths;
}

// note: if literal includes a `*`, globbing will be used - this is expensive.
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

  if (context.use_globbing)
    return expand_literal(out);
  else
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
  out.origin = std::visit(QBVisitOrigin{}, _obj_result);
  if (std::holds_alternative<QBString>(_obj_result)) {
    out.contents = std::vector<QBString>();
  } else if (std::holds_alternative<QBBool>(_obj_result)) {
    out.contents = std::vector<QBBool>();
  } else if (std::holds_alternative<QBList>(_obj_result)) {
    QBList __obj_qblist = std::get<QBList>(_obj_result);
    if (std::holds_alternative<std::vector<QBString>>(__obj_qblist.contents))
      out.contents = std::vector<QBString>();
    else if (std::holds_alternative<std::vector<QBBool>>(__obj_qblist.contents))
      out.contents = std::vector<QBBool>();
  }

  // add the early evaluated first element
  if (std::holds_alternative<QBString>(_obj_result)) {
    std::get<QBLIST_STR>(out.contents)
        .push_back(std::get<QBString>(_obj_result));
  } else if (std::holds_alternative<QBBool>(_obj_result)) {
    std::get<QBLIST_BOOL>(out.contents)
        .push_back(std::get<QBBool>(_obj_result));
  } else if (std::holds_alternative<QBList>(_obj_result)) {
    QBList obj_result_qblist = std::get<QBList>(_obj_result);
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
    }
  }

  for (size_t i = 1; i < list.contents.size(); i++) {
    ASTObject ast_obj = list.contents[i];
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

EvaluationResult ASTVisitEvaluate::operator()(Replace const &replace) {
  // override use_globbing to false, we want to handle the wildcards separately.
  EvaluationContext _context =
      EvaluationContext{context.target_scope, context.target_iteration, false};
  EvaluationResult identifier =
      std::visit(ASTVisitEvaluate{ast, _context}, *replace.identifier);
  EvaluationResult original =
      std::visit(ASTVisitEvaluate{ast, _context}, *replace.original);
  EvaluationResult replacement =
      std::visit(ASTVisitEvaluate{ast, _context}, *replace.replacement);

  if (!std::holds_alternative<QBString>(original) ||
      !std::holds_alternative<QBString>(replacement))
    ErrorHandler::push_error_throw(std::visit(QBVisitOrigin{}, original),
                                   I_EVALUATE_REPLACE_TYPE_ERROR);

  QBList input;
  QBList output;
  if (std::holds_alternative<QBList>(identifier) &&
      std::get<QBList>(identifier).holds_qbstring())
    input = std::get<QBList>(identifier);
  else if (std::holds_alternative<QBString>(identifier))
    std::get<QBLIST_STR>(input.contents)
        .push_back(std::get<QBString>(identifier));
  else
    ErrorHandler::push_error_throw(std::visit(QBVisitOrigin{}, identifier),
                                   I_EVALUATE_REPLACE_TYPE_ERROR);

  // split original and replacement into chunks.
  std::vector<std::string> original_chunked;
  std::stringstream original_ss(std::get<QBString>(original).toString());
  std::string original_buf;
  while (std::getline(original_ss, original_buf, '*'))
    original_chunked.push_back(original_buf);
  std::vector<std::string> replacement_chunked;
  std::stringstream replacement_ss(std::get<QBString>(replacement).toString());
  std::string replacement_buf;
  while (std::getline(replacement_ss, replacement_buf, '*'))
    replacement_chunked.push_back(replacement_buf);

  if (original_chunked.size() < replacement_chunked.size())
    ErrorHandler::push_error_throw(replace.origin,
                                   I_REPLACE_CHUNKS_LENGTH_ERROR);

  // actual string manipulation.
  for (QBString const &qbstring : std::get<QBLIST_STR>(input.contents)) {
    // split input into sections as delimited by the original chunks.
    std::vector<std::string> input_chunked;
    size_t last_token_i = 0;
    for (std::string original_token : original_chunked) {
      size_t token_i = qbstring.toString().find(original_token, last_token_i);
      if (token_i == std::string::npos) {
        std::get<QBLIST_STR>(output.contents).push_back(qbstring);
        last_token_i = std::string::npos;
        break;
      }
      input_chunked.push_back(
          qbstring.toString().substr(last_token_i, (token_i - last_token_i)));
      last_token_i = token_i + original_token.length();
    }
    if (last_token_i == std::string::npos)
      continue;
    // last element.
    input_chunked.push_back(qbstring.toString().substr(last_token_i));

    // reconstruct new element from the chunked input and chunked replacement.
    std::string reconstructed;
    for (size_t i = 0; i < input_chunked.size() - 1; i++) {
      reconstructed += input_chunked[i];
      reconstructed += replacement_chunked[i];
    }
    // last element.
    reconstructed += input_chunked[input_chunked.size() - 1];
    std::get<QBLIST_STR>(output.contents)
        .push_back(QBString(reconstructed, replace.origin));
  }
  // todo: consider returning a single qbstring if list only contains one item.
  return output;
};

Interpreter::Interpreter(AST ast, Setup setup) {
  m_ast = ast;
  m_setup = setup;
}

std::optional<Target> Interpreter::find_target(QBString identifier) {
  for (Target target : m_ast.targets) {
    EvaluationResult target_i =
        std::visit(ASTVisitEvaluate{m_ast, {std::nullopt, std::nullopt}},
                   target.identifier);
    if (std::holds_alternative<QBString>(target_i) &&
        std::get<QBString>(target_i) == identifier) {
      return target;
    } else if (std::holds_alternative<QBList>(target_i) &&
               std::get<QBList>(target_i).holds_qbstring()) {
      for (QBString target_j :
           std::get<QBLIST_STR>(std::get<QBList>(target_i).contents)) {
        if (target_j == identifier)
          return target;
      }
    }
  }
  return std::nullopt;
}

int Interpreter::run_target(Target target, std::string target_iteration) {
  ASTObject _identifier_depends = IDENTIFIER_DEPENDS;
  EvaluationResult dependencies = std::visit(
      ASTVisitEvaluate{m_ast, {target, target_iteration}}, _identifier_depends);

  if (std::holds_alternative<QBString>(dependencies)) {
    QBString _target_iteration = std::get<QBString>(dependencies);
    std::optional<Target> _target = find_target(_target_iteration);
    if (_target) {
      run_target(*_target, _target_iteration.toString());
    }
  } else if (std::holds_alternative<QBList>(dependencies) &&
             std::get<QBList>(dependencies).holds_qbstring()) {
    // todo: handle concurrent execution of tasks.
    for (QBString _target_iteration :
         std::get<QBLIST_STR>(std::get<QBList>(dependencies).contents)) {
      std::optional<Target> _target = find_target(_target_iteration);
      if (!_target) {
        // error out here?
        continue;
      }
      auto _debug1 = *_target;
      auto _debug2 = _target_iteration.toString();

      run_target(*_target, _target_iteration.toString());
    }
  } else {
    // error out here: dependencies are in the wrong type.
  }

  LOG_STANDARD("  " << CYAN << "↪" << RESET << " starting "
                    << target_iteration);

  // todo: handle concurrent execution of tasks.
  ASTObject _identifier_run = IDENTIFIER_RUN;
  EvaluationResult cmdlines = std::visit(
      ASTVisitEvaluate{m_ast, {target, target_iteration}}, _identifier_run);
  if (std::holds_alternative<QBString>(cmdlines)) {
    QBString cmdline = std::get<QBString>(cmdlines);
    ShellResult shell_result = Shell::execute(cmdline.toString());
    if (shell_result.status != 0) {
      // error out here: origin is stored in cmdline.origin
    }
  } else if (std::holds_alternative<QBList>(cmdlines) &&
             std::get<QBList>(cmdlines).holds_qbstring()) {
    for (QBString cmdline :
         std::get<QBLIST_STR>(std::get<QBList>(cmdlines).contents)) {
      ShellResult shell_result = Shell::execute(cmdline.toString());
      if (shell_result.status != 0) {
        // error out here: origin is stored in cmdline.origin
      }
    }
  } else {
    // error out here: run is in the wrong type
  }

  LOG_STANDARD("  " << GREEN << "✓" << RESET << " finished "
                    << target_iteration);
  return 0; // todo: this function probably shouldn't return anything.
}

void Interpreter::build() {

  // find the target.
  if (m_ast.targets.empty())
    ErrorHandler::push_error_throw(ORIGIN_UNDEFINED, I_NO_TARGETS);
  std::optional<Target> target;
  std::string target_iteration;
  if (m_setup.target) {
    target = find_target(QBString(*m_setup.target, ORIGIN_UNDEFINED));
    target_iteration = *m_setup.target;
  } else {
    target = m_ast.targets[0];
    // todo: if this doesn't evaluate nicely, we need to error out properly.
    auto _debug =
        std::visit(ASTVisitEvaluate{m_ast, {std::nullopt, std::nullopt}},
                   m_ast.targets[0].identifier);

    target_iteration =
        std::get<QBString>(
            std::visit(ASTVisitEvaluate{m_ast, {std::nullopt, std::nullopt}},
                       m_ast.targets[0].identifier))
            .toString();
  }

  LOG_STANDARD("⧗ building " << GREEN << target_iteration << RESET);
  // todo: error checking is also required here in case task doesn't exist.
  if (0 == run_target(*target, target_iteration)) {
    LOG_STANDARD("➤ build completed");
  } // else {
  //  LOG_STANDARD(" = one or more tasks " << RED << "failed" << RESET << ";
  //  build halted.");
  //}
}
