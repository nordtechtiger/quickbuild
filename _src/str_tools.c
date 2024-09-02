#include "str_tools.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


char *str_match(char *base, char *pattern);
int get_path_depth(char *path);

// TODO: make this const char
// TODO: tidy up this solution, possibly using recursion?
char *str_match(char *base, char *pattern) {
  int32_t match_base = -1;
  int32_t match_top = -1;

  // advance base pointer for base string
  for (int i = 0; base[i] != '\0'; i++) {
    // iterate through pattern
    for (int j = 0; pattern[j] != '\0' && base[j + i] != '\0'; j++) {
      if (pattern[j] == '*') {
        match_base = i + j;
        // try to find substring that matches at the end
        for (int k = 0; base[i + j + k] != '\0'; k++) {
          for (int l = 0;
               base[i + j + k + l] != '\0' && pattern[j + l + 1] != '\0'; l++) {
            if (pattern[j + l + 1] != base[i + j + k + l]) {
              break;
            } else if (pattern[j + l + 2] == '\0') {
              match_top = i + j + k;
              goto exit_loop;
            }
          }
        }
      } else if (pattern[j] != base[j + i]) {
        // retry different base (outer loop)
        break;
      }
    }
  }

exit_loop:

  if (match_base != -1 && match_top != -1) {
    char *match = calloc(match_top - match_base, sizeof(char));
    strncpy(match, &base[match_base], match_top - match_base);
    return match;
  } else {
    return NULL;
  }
}

int get_path_depth(char *path) {
  uint32_t depth = 0;
  for (int i = 0; path[i] != '\0'; i++) {
    if (path[i] == '/') {
      depth++;
    }
  }
  return depth;
}
