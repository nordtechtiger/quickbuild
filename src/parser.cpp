#include "parser.hpp"
#include "lexer.hpp"
#include <iostream>

using namespace std;

int parse_variable(std::vector<Token>);
int parse_child(std::vector<Token>);

const vector<int (*)(vector<Token>)> parsing_rules{
    /* parse_variable,
    parse_child, */
};

AST Parser::parse_tokens(std::vector<Token> token_stream) {
}

/* # Assuming current config, the AST should look like the following:
 * 
 * field("compiler", <Literal"gcc">)
 * field("flags", <Literal"-g -o0">)
 * field("sources", <Literal"src/*.c">)
 * field("headers", <Literal"src/*.h">)
 * field("objects", <Replace"sources""src/*.c""obj/*.o">)
 * field("binary", <Literal"bin/quickbuild">)
 * target(Literal"quickbuild", "quickbuild", <
 *   field("depends", <Variable"objects", Variable"headers">)
 *   field("run", <Concatenation<Variable"compiler",Literal" ", ...etc>>)
 * >)
 * target(Variable"objects", "obj", <
 *   field("depends", <Replace"obj"obj/*.o""src/*.c">)
 *   field("run", <Concatenation<Variable"compiler",Literal" ", ...etc>>)
 * >)
 */
