#include "parser.hpp"
#include "lexer.hpp"

using namespace std;

int parse_variable(std::vector<Token>);
int parse_child(std::vector<Token>);

const vector<int (*)(vector<Token>)> parsing_rules{
  parse_variable, parse_child,
};

Object Parser::parse_tokens(std::vector<Token> token_stream) {
  
}

int parse_variable(vector<Token> token_stream) {

}

int parse_child(vector<Token> token_stream) {

}
