#include "parser.hpp"
#include "lexer.hpp"

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
 * field("objects", <
 */
