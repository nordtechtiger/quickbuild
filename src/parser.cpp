#include "parser.hpp"
#include "lexer.hpp"

using namespace std;

int parse_variable(std::vector<Token>);
int parse_child(std::vector<Token>);

const vector<int (*)(vector<Token>)> parsing_rules{
  parse_variable,
  parse_child,
};

AST Parser::parse_tokens(std::vector<Token> token_stream) {
  
}

/* # Assuming current config, the AST should look like the following:
 *
 * variable("compiler", <STR"gcc">)
 * variable("flags", <STR"-g -o0">)
 * variable("sources", <STR"src/*.c">)
 * variable("headers", <STR"src/*.h">)
 * variable("objects", <replace(VAR"sources", STR"src/*.c", STR"obj/*.o")>)
 * variable("binary", <STR"bin/quickbuild">)
 * target(STR"quickbuild", STR"quickbuild", <
 *   variable("depends", <VAR"objects", VAR"headers">)
 *   variable("run", <CAT<STR"",VAR"compiler",STR" ", etc..>>)
 * >
 * target(VAR"objects", STR"obj", <
 *
 * >
 */
