#include "fs_tools.h"
#include "lexer.h"
#include "stdio.h"
#include "stdlib.h"

#define CONFIG_FILE "quickbuild"

int main(int argc, char **argv) {
  printf("= quickbuild beta\n");

  // get file system tree
  struct FsObject *fs_tree = calloc(1, sizeof(struct FsObject));
  int ret = get_fs_object(".", fs_tree);
  if (0 > ret) {
    printf("= error: failed to get directory tree. this is likely a bug, "
           "consider submitting a bug report.\n");
    return EXIT_FAILURE;
  }

  // load config file into memory (rb = read binary)
  FILE *config_file = fopen(CONFIG_FILE, "rb");
  if (config_file == NULL) {
    printf("= error: couldn't find a quickbuild config file.\n");
    return EXIT_FAILURE;
  }
  fseek(config_file, 0, SEEK_END);
  size_t config_size = ftell(config_file);
  fseek(config_file, 0, SEEK_SET);

  uint8_t *config_bytes = calloc(config_size, 1);
  fread(config_bytes, 1, config_size, config_file);

  fclose(config_file);

  // lex config

  // clean up and exit
  free_fs_object(fs_tree);
  printf("= done\n");
  return EXIT_SUCCESS;
}
