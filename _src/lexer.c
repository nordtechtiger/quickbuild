#include "lexer.h"
#include <stdio.h>

enum LexState {
  None,
  InIdentifier,
  InString,
};

struct Token *lex_bytes(char *bytes, uint32_t len);

struct Token *lex_bytes(char *bytes, uint32_t len) {
  printf("- lexing config\n");

  struct Token *parent_token;
  enum LexState lex_state = None;
  uint32_t base_ptr = 0;

  for(int i = 0; i < len; i++) {
    switch (lex_state) {
      None:
        break;

      InIdentifier:
        break;

      InString:
        break;
    }
  }
}
