#include "lexer.hpp"
#include <iostream>

using namespace std;

Lexer::Lexer() { this->lex_state = NONE; }

vector<Token> Lexer::lex_bytes(const vector<unsigned char> input_bytes) {

}
