#include "fs_tools.h"
#include "stdio.h"
#include "stdlib.h"

#define CONFIG_FILE "quickbuild"

int main(int argc, char **argv) {
  printf("= quickbuild beta\n");

  // get file system tree
  struct FsObject *fs_tree = calloc(1, sizeof(struct FsObject));
  int ret = get_fs_object(".", fs_tree);
  if (0 > ret) {
    printf("= error: failed to get filesystem information. consider submitting a bug report");
    return EXIT_FAILURE;
  }

  // debug: print all paths
  // while(1) {
  //   printf("path `%s` is file `%s`\n", fs_tree->path, fs_tree->name);
  //   if(!(fs_tree = fs_tree->child)) break;
  // }

  // load config file
  struct FsObject *config_file;
  config_file = find_fs_objects("./makefile", 1, fs_tree);
  

  
  // clean up and exit
  // free_fs_object(fs_object);
  printf("= done\n");
  return EXIT_SUCCESS;
}
