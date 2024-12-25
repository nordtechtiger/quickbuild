#ifndef BUILDER_H
#define BUILDER_H

#include "driver.hpp"
#include "parser.hpp"

#include <optional>
#include <string>
#include <tuple>
#include <vector>

class Builder {
private:
  // Source ast and setup
  AST m_ast;
  Setup m_setup;
  // Refers to the identifier of the current target
  std::string m_target_ref;
  // Current field cache to speed up build
  // TODO - Consider using a hashmap
  std::vector<
      std::tuple<Expression, std::optional<Target>, std::vector<std::string>>>
      m_cache;
  std::optional<std::vector<std::string>>
      get_cached_expression(Expression, std::optional<Target>);

  void build_target(Target target, Literal ctx_literal);
  int get_file_date(std::string path);
  bool is_dirty(Literal literal, std::string dependant);
  std::optional<Target> get_target(Literal literal);
  std::optional<std::vector<Expression>> get_field(std::optional<Target> target,
                                                   Identifier identifier);
  std::vector<std::string> evaluate_literal(Literal literal);
  std::vector<std::string> evaluate_replace(Replace replace,
                                            std::optional<Target> ctx);
  std::vector<std::string> evaluate_concatenation(Concatenation concatenation,
                                                  std::optional<Target> ctx);
  std::vector<std::string> evaluate(std::vector<Expression> expressions,
                                    std::optional<Target> ctx,
                                    bool use_cache = true);
  std::vector<std::string> evaluate(Expression expression,
                                    std::optional<Target> ctx,
                                    bool use_cache = true);

public:
  Builder(AST ast, Setup setup);
  void build();
};

#endif
