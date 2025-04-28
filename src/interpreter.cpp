#include "interpreter.hpp"
#include "filesystem"
#include "format.hpp"
#include "oslayer.hpp"
#include <atomic>
#include <filesystem>
#include <memory>
#include <thread>

#define DEPENDS "depends"
#define DEPENDS_PARALLEL "depends_parallel"
#define RUN "run"
#define RUN_PARALLEL "run_parallel"

struct QBVisitOrigin {
  Origin operator()(QBString qbstring) { return qbstring.origin; };
  Origin operator()(QBBool qbbool) { return qbbool.origin; };
  Origin operator()(QBList qblist) { return qblist.origin; };
};

// constructors & casts for internal data types.
QBString::QBString() {
  this->origin = InternalNode{};
  this->content = "";
}
QBString::QBString(Token token) {
  if (token.type != TokenType::Literal)
    ErrorHandler::push_error_throw(token.origin,
                                   _I_CONSTRUCTOR_EXPECTED_LITERAL);
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
  this->origin = InternalNode{};
  this->content = false;
}
QBBool::QBBool(Token token) {
  if (token.type != TokenType::True && token.type != TokenType::False)
    ErrorHandler::push_error_throw(token.origin, _I_CONSTRUCTOR_EXPECTED_BOOL);
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
  this->origin = InternalNode{};
  this->contents = std::vector<QBString>();
}
QBList::QBList(
    std::variant<std::vector<QBString>, std::vector<QBBool>> contents) {
  if (std::holds_alternative<std::vector<QBString>>(contents)) {
    std::vector<QBString> contents_qbstring =
        std::get<std::vector<QBString>>(contents);
    if (contents_qbstring.size() <= 0)
      ErrorHandler::push_error_throw(InternalNode{},
                                     _I_CONSTRUCTOR_EXPECTED_NONEMPTY);
    this->contents = contents_qbstring;
    this->origin = contents_qbstring[0].origin;
  } else if (std::holds_alternative<std::vector<QBBool>>(contents)) {
    std::vector<QBBool> contents_qbbool =
        std::get<std::vector<QBBool>>(contents);
    if (contents_qbbool.size() <= 0)
      ErrorHandler::push_error_throw(InternalNode{},
                                     _I_CONSTRUCTOR_EXPECTED_NONEMPTY);
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

// returns true if, and only if, the passed context can reach the caller.
bool EvaluationContext::context_verify(EvaluationContext const other) const {
  if (this->target_scope == other.target_scope &&
      this->use_globbing == other.use_globbing)
    return true;
  if (this->target_scope == std::nullopt &&
      this->use_globbing == other.use_globbing)
    return true;
  return false;
}

// visitor that evaluates an AST object recursively.
struct ASTEvaluate {
  AST &ast;
  EvaluationContext context;
  std::shared_ptr<EvaluationState> state;
  QBValue operator()(Identifier const &identifier);
  QBValue operator()(Literal const &literal);
  QBValue operator()(FormattedLiteral const &formatted_literal);
  QBValue operator()(List const &list);
  QBValue operator()(Boolean const &boolean);
  QBValue operator()(Replace const &replace);
};

QBValue
Interpreter::evaluate_ast_object(ASTObject ast_object, AST &ast,
                                 EvaluationContext context,
                                 std::shared_ptr<EvaluationState> state) {
  evaluation_lock.lock();
  // evaluation visitor can amend shared data in the state.
  QBValue value = std::visit(ASTEvaluate{ast, context, state}, ast_object);
  evaluation_lock.unlock();
  return value;
}

QBValue ASTEvaluate::operator()(Identifier const &identifier) {
  // check for any cached values.
  for (ValueInstance value : state->values) {
    if (value.identifier == identifier &&
        value.context.context_verify(context)) {
      return value.result;
    }
  }

  // target-specific fields.
  if (context.target_scope) {
    for (Field const &field : context.target_scope->fields) {
      if (field.identifier.content == identifier.content) {
        ASTEvaluate ast_visitor = {ast, context, state};
        QBValue result = std::visit(ast_visitor, field.expression);
        if (result.immutable)
          state->values.push_back(ValueInstance{identifier, context, result});
        return result;
      }
    }
  }

  // target iteration variable - this isn't cached for obvious reasons.
  if (context.target_iteration && context.target_scope)
    if (context.target_scope->iterator.content == identifier.content)
      return {QBString(*context.target_iteration, context.target_scope->origin),
              false};

  // global fields.
  for (Field const &field : ast.fields) {
    if (field.identifier.content == identifier.content) {
      ASTEvaluate ast_visitor = {
          ast, EvaluationContext{std::nullopt, std::nullopt}, state};
      QBValue result = std::visit(ast_visitor, field.expression);
      EvaluationContext _context =
          EvaluationContext{std::nullopt, std::nullopt, context.use_globbing};
      if (result.immutable)
        state->values.push_back(ValueInstance{identifier, _context, result});
      return result;
    }
  }

  // identifier not found.
  ErrorHandler::push_error_throw({identifier.origin, identifier.content},
                                 I_NO_MATCHING_IDENTIFIER);
  __builtin_unreachable();
}

// note: we handle globbing **after** evaluating a formatted literal.
QBValue ASTEvaluate::operator()(Literal const &literal) {
  return {QBString(literal.content, literal.origin)};
};

// helper method: handles globbing.
QBValue expand_literal(QBString input_qbstring, bool immutable) {
  size_t i_asterisk = input_qbstring.content.find('*');
  if (i_asterisk == std::string::npos) // no globbing.
    return {input_qbstring};

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
    return {std::get<QBLIST_STR>(matching_paths.contents)[0]};

  return {matching_paths, immutable};
}

// note: if literal includes a `*`, globbing will be used - this is
// expensive.
QBValue ASTEvaluate::operator()(FormattedLiteral const &formatted_literal) {
  QBString out;
  bool immutable = true;
  for (ASTObject const &ast_obj : formatted_literal.contents) {
    ASTEvaluate ast_visitor = {ast, context, state};
    QBValue obj_result = std::visit(ast_visitor, ast_obj);
    // append a string.
    if (std::holds_alternative<QBString>(obj_result.value)) {
      if (std::holds_alternative<InternalNode>(out.origin))
        out.origin = std::get<QBString>(obj_result.value).origin;
      out.content += std::get<QBString>(obj_result.value).content;
      immutable &= obj_result.immutable;
    }
    // append a bool.
    else if (std::holds_alternative<QBBool>(obj_result.value)) {
      if (std::holds_alternative<InternalNode>(out.origin))
        out.origin = std::get<QBBool>(obj_result.value).origin;
      out.content += (std::get<QBBool>(obj_result.value) ? "true" : "false");
      immutable &= obj_result.immutable;
    }
    // append a list of strings.
    else if (std::holds_alternative<QBList>(obj_result.value) &&
             std::get<QBList>(obj_result.value).contents.index() ==
                 QBLIST_STR) {
      QBList obj_result_list = std::get<QBList>(obj_result.value);
      for (size_t i = 0;
           i < std::get<QBLIST_STR>(obj_result_list.contents).size(); i++) {
        if (std::holds_alternative<InternalNode>(out.origin))
          out.origin = std::get<QBLIST_STR>(obj_result_list.contents)[i].origin;
        out.content +=
            std::get<QBLIST_STR>(obj_result_list.contents)[i].content;
        immutable &= obj_result.immutable;
        if (i < std::get<QBLIST_STR>(obj_result_list.contents).size() - 1)
          out.content.append(" ");
      }
    }
    // append a list of bools.
    else if (std::holds_alternative<QBList>(obj_result.value) &&
             std::get<QBList>(obj_result.value).contents.index() ==
                 QBLIST_BOOL) {
      QBList obj_result_list = std::get<QBList>(obj_result.value);
      for (size_t i = 0;
           i < std::get<QBLIST_BOOL>(obj_result_list.contents).size(); i++) {
        if (std::holds_alternative<InternalNode>(out.origin))
          out.origin =
              std::get<QBLIST_BOOL>(obj_result_list.contents)[i].origin;
        out.content +=
            (std::get<QBLIST_BOOL>(obj_result_list.contents)[i] ? "true"
                                                                : "false");
        immutable &= obj_result.immutable;
        if (i < std::get<QBLIST_BOOL>(obj_result_list.contents).size() - 1)
          out.content.append(" ");
      }
    }
  }

  if (context.use_globbing)
    return expand_literal(out, immutable);
  else
    return {out, immutable};
}

QBValue ASTEvaluate::operator()(List const &list) {
  if (list.contents.size() <= 0)
    ErrorHandler::push_error_throw(InternalNode{},
                                   _I_EVALUATE_EXPECTED_NONEMPTY);

  QBList out;
  bool immutable = true;
  // infer the list type. todo: consider a cleaner solution.
  ASTEvaluate ast_visitor = {ast, context, state};
  QBValue _obj_result = std::visit(ast_visitor, list.contents[0]);
  out.origin = std::visit(QBVisitOrigin{}, _obj_result.value);
  if (std::holds_alternative<QBString>(_obj_result.value)) {
    out.contents = std::vector<QBString>();
  } else if (std::holds_alternative<QBBool>(_obj_result.value)) {
    out.contents = std::vector<QBBool>();
  } else if (std::holds_alternative<QBList>(_obj_result.value)) {
    QBList __obj_qblist = std::get<QBList>(_obj_result.value);
    if (std::holds_alternative<std::vector<QBString>>(__obj_qblist.contents))
      out.contents = std::vector<QBString>();
    else if (std::holds_alternative<std::vector<QBBool>>(__obj_qblist.contents))
      out.contents = std::vector<QBBool>();
  }

  // add the early evaluated first element.
  if (std::holds_alternative<QBString>(_obj_result.value)) {
    std::get<QBLIST_STR>(out.contents)
        .push_back(std::get<QBString>(_obj_result.value));
    immutable &= _obj_result.immutable;
  } else if (std::holds_alternative<QBBool>(_obj_result.value)) {
    std::get<QBLIST_BOOL>(out.contents)
        .push_back(std::get<QBBool>(_obj_result.value));
    immutable &= _obj_result.immutable;
  } else if (std::holds_alternative<QBList>(_obj_result.value)) {
    QBList obj_result_qblist = std::get<QBList>(_obj_result.value);
    if (obj_result_qblist.holds_qbstring() && out.holds_qbstring()) {
      std::get<QBLIST_STR>(out.contents)
          .insert(std::get<QBLIST_STR>(out.contents).begin(),
                  std::get<QBLIST_STR>(obj_result_qblist.contents).begin(),
                  std::get<QBLIST_STR>(obj_result_qblist.contents).end());
      immutable &= _obj_result.immutable;
    } else if (obj_result_qblist.holds_qbbool() && out.holds_qbbool()) {
      std::get<QBLIST_BOOL>(out.contents)
          .insert(std::get<QBLIST_BOOL>(out.contents).begin(),
                  std::get<QBLIST_BOOL>(obj_result_qblist.contents).begin(),
                  std::get<QBLIST_BOOL>(obj_result_qblist.contents).end());
      immutable &= _obj_result.immutable;
    }
  }

  for (size_t i = 1; i < list.contents.size(); i++) {
    ASTObject ast_obj = list.contents[i];
    ASTEvaluate ast_visitor = {ast, context, state};
    QBValue obj_result = std::visit(ast_visitor, ast_obj);
    if (std::holds_alternative<QBString>(obj_result.value)) {
      if (out.holds_qbstring()) {
        std::get<QBLIST_STR>(out.contents)
            .push_back(std::get<QBString>(obj_result.value));
        immutable &= _obj_result.immutable;
      } else
        ErrorHandler::push_error_throw(
            std::get<QBString>(obj_result.value).origin,
            I_EVALUATE_LIST_TYPE_MISMATCH);
    } else if (std::holds_alternative<QBBool>(obj_result.value)) {
      if (out.holds_qbbool()) {
        std::get<QBLIST_BOOL>(out.contents)
            .push_back(std::get<QBBool>(obj_result.value));
        immutable &= _obj_result.immutable;
      } else
        ErrorHandler::push_error_throw(
            std::get<QBBool>(obj_result.value).origin,
            I_EVALUATE_LIST_TYPE_MISMATCH);
    } else if (std::holds_alternative<QBList>(obj_result.value)) {
      QBList obj_result_qblist = std::get<QBList>(obj_result.value);
      if (obj_result_qblist.holds_qbstring() && out.holds_qbstring()) {
        std::get<QBLIST_STR>(out.contents)
            .insert(std::get<QBLIST_STR>(out.contents).end(),
                    std::get<QBLIST_STR>(obj_result_qblist.contents).begin(),
                    std::get<QBLIST_STR>(obj_result_qblist.contents).end());
        immutable &= _obj_result.immutable;
      } else if (obj_result_qblist.holds_qbbool() && out.holds_qbbool()) {
        std::get<QBLIST_BOOL>(out.contents)
            .insert(std::get<QBLIST_BOOL>(out.contents).end(),
                    std::get<QBLIST_BOOL>(obj_result_qblist.contents).begin(),
                    std::get<QBLIST_BOOL>(obj_result_qblist.contents).end());
        immutable &= _obj_result.immutable;
      } else {
        ErrorHandler::push_error_throw(obj_result_qblist.origin,
                                       I_EVALUATE_LIST_TYPE_MISMATCH);
      }
    }
  }

  return {out, immutable};
};

QBValue ASTEvaluate::operator()(Boolean const &boolean) {
  return {QBBool(boolean.content, boolean.origin)};
};

QBValue ASTEvaluate::operator()(Replace const &replace) {
  // override use_globbing to false, we want to handle the wildcards
  // separately.
  EvaluationContext _context =
      EvaluationContext{context.target_scope, context.target_iteration, false};
  ASTEvaluate ast_visitor = {ast, _context, state};
  QBValue identifier = std::visit(ast_visitor, *replace.identifier);
  QBValue original = std::visit(ast_visitor, *replace.original);
  QBValue replacement = std::visit(ast_visitor, *replace.replacement);

  bool immutable = true;
  immutable &= identifier.immutable;
  immutable &= original.immutable;
  immutable &= replacement.immutable;

  if (!std::holds_alternative<QBString>(original.value) ||
      !std::holds_alternative<QBString>(replacement.value))
    ErrorHandler::push_error_throw(std::visit(QBVisitOrigin{}, original.value),
                                   I_EVALUATE_REPLACE_TYPE_ERROR);

  QBList input;
  QBList output;
  if (std::holds_alternative<QBList>(identifier.value) &&
      std::get<QBList>(identifier.value).holds_qbstring())
    input = std::get<QBList>(identifier.value);
  else if (std::holds_alternative<QBString>(identifier.value))
    std::get<QBLIST_STR>(input.contents)
        .push_back(std::get<QBString>(identifier.value));
  else
    ErrorHandler::push_error_throw(
        std::visit(QBVisitOrigin{}, identifier.value),
        I_EVALUATE_REPLACE_TYPE_ERROR);

  // split original and replacement into chunks.
  std::vector<std::string> original_chunked;
  std::stringstream original_ss(std::get<QBString>(original.value).toString());
  std::string original_buf;
  while (std::getline(original_ss, original_buf, '*'))
    original_chunked.push_back(original_buf);
  std::vector<std::string> replacement_chunked;
  std::stringstream replacement_ss(
      std::get<QBString>(replacement.value).toString());
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

    // reconstruct new element from the chunked input and chunked
    // replacement.
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
  // todo: consider returning a single qbstring if list only contains one
  // item.
  return {output, immutable};
};

Interpreter::Interpreter(AST &ast, Setup setup) : m_ast(ast) {
  m_setup = setup;
}

std::optional<Target> Interpreter::find_target(QBString identifier) {
  for (Target target : m_ast.targets) {
    QBValue target_i = evaluate_ast_object(target.identifier, m_ast,
                                           {std::nullopt, std::nullopt}, state);
    if (std::holds_alternative<QBString>(target_i.value) &&
        std::get<QBString>(target_i.value) == identifier) {
      return target;
    } else if (std::holds_alternative<QBList>(target_i.value) &&
               std::get<QBList>(target_i.value).holds_qbstring()) {
      for (QBString target_j :
           std::get<QBLIST_STR>(std::get<QBList>(target_i.value).contents)) {
        if (target_j == identifier)
          return target;
      }
    }
  }
  return std::nullopt;
}

std::optional<Field> Interpreter::find_field(std::string identifier,
                                             std::optional<Target> target) {
  // target-specific fields.
  if (target)
    for (Field const &field : target->fields)
      if (field.identifier.content == identifier)
        return field;

  // global fields.
  for (Field const &field : m_ast.fields) {
    if (field.identifier.content == identifier) {
      return field;
    }
  }
  return std::nullopt;
}

QBValue
Interpreter::evaluate_field_default(std::string identifier,
                                    EvaluationContext context,
                                    std::shared_ptr<EvaluationState> state,
                                    std::optional<QBValue> default_value) {
  std::optional<Field> field = find_field(identifier, context.target_scope);
  if (!field) {
    if (!default_value) {
      ErrorHandler::push_error_throw(ObjectReference(identifier),
                                     I_NO_FIELD_NOR_DEFAULT);
    }
    return *default_value;
  }
  return evaluate_ast_object(field->expression, m_ast, context, state);
}

std::optional<QBValue>
Interpreter::evaluate_field_optional(std::string identifier,
                                     EvaluationContext context,
                                     std::shared_ptr<EvaluationState> state) {
  std::optional<Field> field = find_field(identifier, context.target_scope);
  if (!field)
    return std::nullopt;
  return evaluate_ast_object(field->expression, m_ast, context, state);
}

void Interpreter::_run_target(Target target, std::string target_iteration,
                              std::shared_ptr<std::atomic<bool>> error) {
  if (0 > run_target(target, target_iteration))
    *error = true;
}

DependencyStatus
Interpreter::_solve_dependencies_parallel(QBValue dependencies) {
  if (std::holds_alternative<QBString>(dependencies.value)) {
    // only one dependency - no reason to use a separate thread.
    QBString target_iteration = std::get<QBString>(dependencies.value);
    std::optional<Target> _target = find_target(target_iteration);
    std::optional<size_t> modified =
        OSLayer::get_file_timestamp(target_iteration.toString());
    if (!_target) {
      return {true, modified};
    }
    int run_status = run_target(*_target, target_iteration.toString());
    if (0 > run_status)
      ErrorHandler::push_error(_target->origin, I_DEPENDENCY_FAILED);
    return {0 == run_status, modified};
  }
  if (!std::holds_alternative<QBList>(dependencies.value) ||
      !std::get<QBList>(dependencies.value).holds_qbstring()) {
    ErrorHandler::push_error_throw(
        std::visit(QBVisitOrigin{}, dependencies.value), I_TYPE_DEPENDENCIES);
    __builtin_unreachable();
  }

  std::vector<std::thread> pool;
  QBList dependencies_list = std::get<QBList>(dependencies.value);
  std::shared_ptr<std::atomic<bool>> error =
      std::make_shared<std::atomic<bool>>();
  *error = false;
  std::optional<size_t> modified;

  for (QBString target_iteration :
       std::get<QBLIST_STR>(std::get<QBList>(dependencies.value).contents)) {
    std::optional<Target> _target = find_target(target_iteration);
    std::optional<size_t> modified_i =
        OSLayer::get_file_timestamp(target_iteration.toString());
    if (!modified || (modified_i && modified < modified_i))
      modified = modified_i;
    if (!_target) {
      continue;
    }
    pool.push_back(std::thread(&Interpreter::_run_target, this, *_target,
                               target_iteration.toString(), error));
  }

  for (std::thread &thread : pool) {
    thread.join();
  }

  if (*error) {
    ErrorHandler::push_error(InternalNode{}, I_DEPENDENCY_FAILED);
    return {false, modified};
  } else
    return {true, modified};
}

DependencyStatus Interpreter::_solve_dependencies_sync(QBValue dependencies) {
  if (std::holds_alternative<QBString>(dependencies.value)) {
    QBString target_iteration = std::get<QBString>(dependencies.value);
    std::optional<Target> _target = find_target(target_iteration);
    std::optional<size_t> modified =
        OSLayer::get_file_timestamp(target_iteration.toString());
    if (_target) {
      int run_status = run_target(*_target, target_iteration.toString());
      if (0 > run_status)
        ErrorHandler::push_error(_target->origin, I_DEPENDENCY_FAILED);
      return {0 == run_status, modified};
    }
    return {true, modified};
  } else if (std::holds_alternative<QBList>(dependencies.value) &&
             std::get<QBList>(dependencies.value).holds_qbstring()) {
    std::optional<size_t> modified;
    for (QBString target_iteration :
         std::get<QBLIST_STR>(std::get<QBList>(dependencies.value).contents)) {
      std::optional<Target> _target = find_target(target_iteration);
      std::optional<size_t> modified_i =
          OSLayer::get_file_timestamp(target_iteration.toString());
      if (!modified || (modified_i && modified < modified_i))
        modified = modified_i;
      if (!_target) {
        continue;
      }
      int run_status = run_target(*_target, target_iteration.toString());
      if (0 > run_status)
        ErrorHandler::push_error(_target->origin, I_DEPENDENCY_FAILED);
      if (0 > run_status) {
        return {false, modified};
      }
    }
    return {true, modified};
  } else {
    ErrorHandler::push_error_throw(
        std::visit(QBVisitOrigin{}, dependencies.value), I_TYPE_DEPENDENCIES);
    __builtin_unreachable();
  }
}

DependencyStatus Interpreter::solve_dependencies(QBValue dependencies,
                                                 bool parallel) {
  if (parallel)
    return _solve_dependencies_parallel(dependencies);
  else
    return _solve_dependencies_sync(dependencies);
}

int Interpreter::run_target(Target target, std::string target_iteration) {
  // solve dependencies.
  std::optional<QBValue> dependencies =
      evaluate_field_optional(DEPENDS, {target, target_iteration}, this->state);
  std::optional<size_t> dep_modified;
  if (dependencies) {
    QBValue parallel_default = {QBBool(false, InternalNode{}), true};
    QBValue parallel =
        evaluate_field_default(DEPENDS_PARALLEL, {target, target_iteration},
                               this->state, parallel_default);
    if (!std::holds_alternative<QBBool>(parallel.value)) {
      ErrorHandler::push_error_throw(
          std::visit(QBVisitOrigin{}, parallel.value), I_TYPE_PARALLEL);
    }
    DependencyStatus dep_stat =
        solve_dependencies(*dependencies, std::get<QBBool>(parallel.value));
    dep_modified = dep_stat.modified;
    if (!dep_stat.success)
      return -1;
  }

  // check for changes.
  std::optional<size_t> this_modified =
      OSLayer::get_file_timestamp(target_iteration);
  if (this_modified && dep_modified && *this_modified >= *dep_modified) {
    LOG_STANDARD("  " << "•" << RESET << " skipped " << target_iteration);
    return 0;
  }

  // execution related fields.
  std::optional<QBValue> command_expr =
      evaluate_field_optional(RUN, {target, target_iteration}, this->state);
  if (!command_expr) {
    return 0; // abstract task.
  }
  QBValue run_parallel_default = {QBBool(false, InternalNode{}), true};
  QBValue run_parallel =
      evaluate_field_default(RUN_PARALLEL, {target, target_iteration},
                             this->state, run_parallel_default);
  if (!std::holds_alternative<QBBool>(run_parallel.value)) {
    ErrorHandler::push_error_throw(
        std::visit(QBVisitOrigin{}, run_parallel.value), I_TYPE_PARALLEL);
  }
  if (!std::holds_alternative<QBString>(command_expr->value) &&
      !(std::holds_alternative<QBList>(command_expr->value) &&
       std::get<QBList>(command_expr->value).holds_qbstring())) {
    ErrorHandler::push_error_throw(
        std::visit(QBVisitOrigin{}, command_expr->value), I_TYPE_RUN);
  }

  // execute task.
  LOG_STANDARD("  " << CYAN << "»" << RESET << " starting "
                    << target_iteration);
  if (std::holds_alternative<QBString>(command_expr->value)) {
    // single command
    QBString cmdline = std::get<QBString>(command_expr->value);
    OSLayer os_layer(std::get<QBBool>(run_parallel.value), false);
    os_layer.queue_command({cmdline.toString(), cmdline.origin});
    os_layer.execute_queue();
    if (!os_layer.get_errors().empty()) {
      for (ErrorContext const &e_ctx : os_layer.get_errors()) {
        ErrorHandler::push_error(e_ctx, I_NONZERO_PROCESS);
      }
      return -1;
    }
  } else if (std::holds_alternative<QBList>(command_expr->value) &&
             std::get<QBList>(command_expr->value).holds_qbstring()) {
    // multiple commands
    OSLayer os_layer(std::get<QBBool>(run_parallel.value), false);
    for (QBString cmdline :
         std::get<QBLIST_STR>(std::get<QBList>(command_expr->value).contents)) {
      os_layer.queue_command({cmdline.toString(), cmdline.origin});
    }
    os_layer.execute_queue();
    if (!os_layer.get_errors().empty()) {
      for (ErrorContext const &e_ctx : os_layer.get_errors()) {
        ErrorHandler::push_error(e_ctx, I_NONZERO_PROCESS);
      }
      return -1;
    }
  }

  LOG_STANDARD("  " << GREEN << "✓" << RESET << " finished "
                    << target_iteration);
  return 0;
}

int Interpreter::build() {

  this->state = std::make_shared<EvaluationState>();

  // find the target.
  if (m_ast.targets.empty())
    ErrorHandler::push_error_throw(InternalNode{}, I_NO_TARGETS);
  std::optional<Target> target;
  std::string target_iteration;
  if (m_setup.target) {
    target = find_target(QBString(*m_setup.target, InternalNode{}));
    target_iteration = *m_setup.target;
    if (!target) {
      ErrorHandler::push_error_throw(ObjectReference(*m_setup.target),
                                     I_SPECIFIED_TARGET_NOT_FOUND);
    }
  } else if (m_ast.targets.size() > 0) {
    target = m_ast.targets[0];
    QBValue target_iteration_qbvalue =
        evaluate_ast_object(m_ast.targets[0].identifier, m_ast,
                            {std::nullopt, std::nullopt}, state);
    if (!std::holds_alternative<QBString>(target_iteration_qbvalue.value)) {
      ErrorHandler::push_error_throw(
          std::visit(QBVisitOrigin{}, target_iteration_qbvalue.value),
          I_MULTIPLE_TARGETS);
    }
    target_iteration =
        std::get<QBString>(
            evaluate_ast_object(m_ast.targets[0].identifier, m_ast,
                                {std::nullopt, std::nullopt}, state)
                .value)
            .toString();
  } else {
    ErrorHandler::push_error_throw(InternalNode{}, I_NO_TARGETS);
  }

  LOG_STANDARD("⧗ building " << CYAN << target_iteration << RESET);
  // todo: error checking is also required here in case task doesn't exist.
  if (0 == run_target(*target, target_iteration)) {
    return 0;
  } else {
    ErrorHandler::push_error_throw(target->origin, I_BUILD_FAILED);
    __builtin_unreachable();
  }
}
