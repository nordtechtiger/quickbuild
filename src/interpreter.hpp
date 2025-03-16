#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "driver.hpp"
#include "parser.hpp"
#include <variant>
#include <vector>

struct QBString {
  Origin origin;
  std::string content;

  std::string toString();
  QBString();
  QBString(Token);
  QBString(std::string, Origin);
};

struct QBBool {
  Origin origin;
  bool content;
  QBBool();
  QBBool(Token);
  QBBool(bool, Origin);
  operator bool();
};

#define QBLIST_STR 0
#define QBLIST_BOOL 1

struct QBList {
  Origin origin;
  std::variant<std::vector<QBString>, std::vector<QBBool>> contents;
  bool holds_qbstring();
  bool holds_qbbool();
  QBList();
  QBList(std::variant<std::vector<QBString>, std::vector<QBBool>>);
};

using EvaluationResult =
    std::variant<QBString, QBBool, QBList>;

struct EvaluationContext {
  std::optional<Target> target_scope;
  std::optional<std::string> target_iteration;
};

class Interpreter {
private:
  AST m_ast;
  Setup m_setup;

  std::optional<Target> find_target();

public:
  Interpreter(AST ast, Setup setup);
  void build();
};

#endif
