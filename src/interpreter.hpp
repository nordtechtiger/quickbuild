#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "driver.hpp"
#include "parser.hpp"
#include <variant>
#include <vector>

struct QBString {
  Origin origin;
  std::string content;

  std::string toString() const;
  QBString();
  QBString(Token);
  QBString(std::string, Origin);
  bool operator==(QBString const other) const;
};

struct QBBool {
  Origin origin;
  bool content;
  QBBool();
  QBBool(Token);
  QBBool(bool, Origin);
  operator bool() const;
  bool operator==(QBBool const other) const;
};

#define QBLIST_STR 0
#define QBLIST_BOOL 1

struct QBList {
  Origin origin;
  std::variant<std::vector<QBString>, std::vector<QBBool>> contents;
  bool holds_qbstring() const;
  bool holds_qbbool() const;
  QBList();
  QBList(std::variant<std::vector<QBString>, std::vector<QBBool>>);
  bool operator==(QBList const other) const;
};

struct QBValue {
  std::variant<QBString, QBBool, QBList> value;
  bool immutable = true;
};

struct EvaluationContext {
  std::optional<Target> target_scope;
  std::optional<std::string> target_iteration;
  bool use_globbing = true;
  bool context_verify(EvaluationContext const) const;
};

struct ValueInstance {
  Identifier identifier;
  EvaluationContext context;
  QBValue result;
};

struct EvaluationState {
  std::vector<ValueInstance> values;
};

class Interpreter {
private:
  AST m_ast;
  Setup m_setup;
  std::shared_ptr<EvaluationState> state;

  std::optional<Target> find_target(QBString identifier);
  std::optional<Field> find_field(std::string identifier, std::optional<Target> target);
  // void _run_target(Target target, std::string target_iteration, std::atomic<bool> &error);
  void _run_target(Target target, std::string target_iteration);
  int run_target(Target target, std::string target_iteration);
  int _solve_dependencies_parallel(QBValue dependencies);
  int _solve_dependencies_sync(QBValue dependencies);
  int solve_dependencies(QBValue dependencies, bool parallel);

public:
  Interpreter(AST ast, Setup setup);
  void build();
};

#endif
