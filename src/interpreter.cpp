#include "interpreter.hpp"
#include "filesystem"
#include "format.hpp"
#include "oslayer.hpp"
#include <thread>
#include <atomic>

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
  AST ast;
  EvaluationContext context;
  std::shared_ptr<EvaluationState> state;
  QBValue operator()(Identifier const &identifier);
  QBValue operator()(Literal const &literal);
  QBValue operator()(FormattedLiteral const &formatted_literal);
  QBValue operator()(List const &list);
  QBValue operator()(Boolean const &boolean);
  QBValue operator()(Replace const &replace);
};

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
      return {QBString(*context.target_iteration, context.target_scope->origin), false};

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
  ErrorHandler::push_error_throw(identifier.origin, I_NO_MATCHING_IDENTIFIER);
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
QBValue
ASTEvaluate::operator()(FormattedLiteral const &formatted_literal) {
  QBString out;
  bool immutable = true;
  for (ASTObject const &ast_obj : formatted_literal.contents) {
    ASTEvaluate ast_visitor = {ast, context, state};
    QBValue obj_result = std::visit(ast_visitor, ast_obj);
    // append a string.
    if (std::holds_alternative<QBString>(obj_result.value)) {
      if (out.origin == ORIGIN_UNDEFINED)
        out.origin = std::get<QBString>(obj_result.value).origin;
      out.content += std::get<QBString>(obj_result.value).content;
      immutable &= obj_result.immutable;
    }
    // append a bool.
    else if (std::holds_alternative<QBBool>(obj_result.value)) {
      if (out.origin == ORIGIN_UNDEFINED)
        out.origin = std::get<QBBool>(obj_result.value).origin;
      out.content += (std::get<QBBool>(obj_result.value) ? "true" : "false");
      immutable &= obj_result.immutable;
    }
    // append a list of strings.
    else if (std::holds_alternative<QBList>(obj_result.value) &&
             std::get<QBList>(obj_result.value).contents.index() == QBLIST_STR) {
      QBList obj_result_list = std::get<QBList>(obj_result.value);
      for (size_t i = 0;
           i < std::get<QBLIST_STR>(obj_result_list.contents).size(); i++) {
        if (out.origin == ORIGIN_UNDEFINED)
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
             std::get<QBList>(obj_result.value).contents.index() == QBLIST_BOOL) {
      QBList obj_result_list = std::get<QBList>(obj_result.value);
      for (size_t i = 0;
           i < std::get<QBLIST_BOOL>(obj_result_list.contents).size(); i++) {
        if (out.origin == ORIGIN_UNDEFINED)
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
    ErrorHandler::push_error_throw(ORIGIN_UNDEFINED,
                                   I_EVALUATE_EXPECTED_NONEMPTY);

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

  // add the early evaluated first element
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
      }
      else
        ErrorHandler::push_error_throw(std::get<QBString>(obj_result.value).origin,
                                       I_EVALUATE_QBSTRING_IN_QBBOOL_LIST);
    } else if (std::holds_alternative<QBBool>(obj_result.value)) {
      if (out.holds_qbbool()) {
        std::get<QBLIST_BOOL>(out.contents)
            .push_back(std::get<QBBool>(obj_result.value));
        immutable &= _obj_result.immutable;
      }
      else
        ErrorHandler::push_error_throw(std::get<QBBool>(obj_result.value).origin,
                                       I_EVALUATE_QBBOOL_IN_QBSTRING_LIST);
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
    ErrorHandler::push_error_throw(std::visit(QBVisitOrigin{}, identifier.value),
                                   I_EVALUATE_REPLACE_TYPE_ERROR);

  // split original and replacement into chunks.
  std::vector<std::string> original_chunked;
  std::stringstream original_ss(std::get<QBString>(original.value).toString());
  std::string original_buf;
  while (std::getline(original_ss, original_buf, '*'))
    original_chunked.push_back(original_buf);
  std::vector<std::string> replacement_chunked;
  std::stringstream replacement_ss(std::get<QBString>(replacement.value).toString());
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

Interpreter::Interpreter(AST ast, Setup setup) {
  m_ast = ast;
  m_setup = setup;
}

std::optional<Target> Interpreter::find_target(QBString identifier) {
  for (Target target : m_ast.targets) {
    ASTEvaluate ast_visitor = {m_ast, {std::nullopt, std::nullopt}, state};
    QBValue target_i = std::visit(ast_visitor, target.identifier);
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

// void Interpreter::_run_target(Target target, std::string target_iteration, std::atomic<bool> &error) {
void Interpreter::_run_target(Target target, std::string target_iteration) {
  if (0 > run_target(target, target_iteration))
    return;
  // error = true;
}

int Interpreter::_solve_dependencies_parallel(QBValue dependencies) {
  if (std::holds_alternative<QBString>(dependencies.value)) {
    // only one dependency - no reason to use a separate thread.
    QBString _target_iteration = std::get<QBString>(dependencies.value);
    std::optional<Target> _target = find_target(_target_iteration);
    if (_target) {
      return run_target(*_target, _target_iteration.toString());
    }
    // check state of file?
    return 0;
  }
  if (!std::holds_alternative<QBList>(dependencies.value) ||
      !std::get<QBList>(dependencies.value).holds_qbstring()) {
    // error out here.
  }

  std::vector<std::thread> pool;
  std::atomic<bool> error = false;
  QBList dependencies_list = std::get<QBList>(dependencies.value);
    for (QBString _target_iteration :
         std::get<QBLIST_STR>(std::get<QBList>(dependencies.value).contents)) {
      std::optional<Target> _target = find_target(_target_iteration);
      if (!_target) {
        // check state of file
        continue;
      }
      // pool.push_back(std::thread(&Interpreter::_run_target, this, *_target, _target_iteration.toString(), error));
      pool.push_back(std::thread(&Interpreter::_run_target, this, *_target, _target_iteration.toString()));
    }

  for (std::thread &thread : pool) {
    thread.join();
  }

  if (error)
    return -1;
  else
    return 0;
}

int Interpreter::_solve_dependencies_sync(QBValue dependencies) {
  if (std::holds_alternative<QBString>(dependencies.value)) {
    QBString _target_iteration = std::get<QBString>(dependencies.value);
    std::optional<Target> _target = find_target(_target_iteration);
    if (_target) {
      return run_target(*_target, _target_iteration.toString());
    }
    // check state of file
    return 0;
  } else if (std::holds_alternative<QBList>(dependencies.value) &&
             std::get<QBList>(dependencies.value).holds_qbstring()) {
    // todo: handle concurrent execution of tasks.
    for (QBString _target_iteration :
         std::get<QBLIST_STR>(std::get<QBList>(dependencies.value).contents)) {
      std::optional<Target> _target = find_target(_target_iteration);
      if (!_target) {
        // check state of file
        continue;
      }
      if (0 > run_target(*_target, _target_iteration.toString())) {
        return -1;
      }
    }
    return 0;
  } else {
    // error out here: dependencies are in the wrong type.
    return -1;
  }
}

int Interpreter::solve_dependencies(QBValue dependencies, bool parallel) {
  if (parallel)
    return _solve_dependencies_parallel(dependencies);
  else
    return _solve_dependencies_sync(dependencies);
}

int Interpreter::run_target(Target target, std::string target_iteration) {
  // ASTObject _identifier_depends = IDENTIFIER_DEPENDS;
  std::optional<Field> field_depends = find_field(DEPENDS, target);
  if (field_depends) {
    ASTEvaluate ast_visitor = {m_ast, {target, target_iteration}, state};
    QBValue dependencies =
        std::visit(ast_visitor, field_depends->expression);
    bool parallel = false;
    std::optional<Field> field_depends_parallel = find_field(DEPENDS_PARALLEL, target);
    if (field_depends_parallel) {
      QBValue depends_parallel = std::visit(ast_visitor, field_depends_parallel->expression);
      if (std::holds_alternative<QBBool>(depends_parallel.value))
        parallel = std::get<QBBool>(depends_parallel.value);
      else {
        // error out here
      }
    }
    solve_dependencies(dependencies, parallel);
  }

  LOG_STANDARD("  " << CYAN << "↪" << RESET << " starting "
                    << target_iteration);

  // todo: handle concurrent execution of tasks.
  std::optional<Field> field_run = find_field(RUN, target);
  if (!field_run) {
    // throw proper error here...
  }

  ASTEvaluate ast_visitor = {m_ast, {target, target_iteration}, state};
  QBValue command_expr =
      std::visit(ast_visitor, field_run->expression);

  bool parallel = false;
  std::optional<Field> field_run_parallel = find_field(RUN_PARALLEL, target);
  if (field_run_parallel) {
    QBValue run_parallel = std::visit(ast_visitor, field_run_parallel->expression);
    if (std::holds_alternative<QBBool>(run_parallel.value))
        parallel = std::get<QBBool>(run_parallel.value);
    else {
      // error out here
    }
  }

  if (std::holds_alternative<QBString>(command_expr.value)) {
    // single command
    QBString command = std::get<QBString>(command_expr.value);
    OSLayer os_layer(parallel, false);
    os_layer.queue_command(command.toString());
    if (os_layer.execute_queue()) {
      std::cerr << "huh, something failed here too!" << std::endl;
      exit(-1);
    }
  } else if (std::holds_alternative<QBList>(command_expr.value) &&
             std::get<QBList>(command_expr.value).holds_qbstring()) {
    // multiple commands
    OSLayer os_layer(parallel, false);
    for (QBString command :
         std::get<QBLIST_STR>(std::get<QBList>(command_expr.value).contents)) {
      os_layer.queue_command(command.toString());
    }
    if (os_layer.execute_queue()) {
      std::cerr << "something went wrong here!" << std::endl;
      exit(-1);
    }
  } else {
    // error out here: run is in the wrong type
  }

  LOG_STANDARD("  " << GREEN << "✓" << RESET << " finished "
                    << target_iteration);
  return 0;
}

void Interpreter::build() {

  this->state = std::make_shared<EvaluationState>();

  // find the target.
  if (m_ast.targets.empty())
    ErrorHandler::push_error_throw(ORIGIN_UNDEFINED, I_NO_TARGETS);
  std::optional<Target> target;
  std::string target_iteration;
  if (m_setup.target) {
    target = find_target(QBString(*m_setup.target, ORIGIN_UNDEFINED));
    target_iteration = *m_setup.target;
    if (!target) {
      // error out here.
    }
  } else {
    target = m_ast.targets[0];
    // todo: if this doesn't evaluate nicely, we need to error out properly.
    ASTEvaluate ast_visitor = {m_ast, {std::nullopt, std::nullopt}, state};
    target_iteration =
        std::get<QBString>(std::visit(ast_visitor, m_ast.targets[0].identifier).value)
            .toString();
  }

  LOG_STANDARD("⧗ building " << GREEN << target_iteration << RESET);
  // todo: error checking is also required here in case task doesn't exist.
  if (0 == run_target(*target, target_iteration)) {
    LOG_STANDARD("➤ build completed");
  } else {
    LOG_STANDARD("➤ build " << RED << "failed" << RESET);
  }
}
