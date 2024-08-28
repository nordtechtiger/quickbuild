#include "fs_tools.h"
#include "stdio.h"
#include "stdlib.h"

#define CONFIG_FILE "quickbuild"

int main(int argc, char **argv) {
  printf("= quickbuild beta\n");

  // get file system tree
  struct FsObject fs_object = {
    .name = "root",
    .path = "path",
    .child = NULL,
  };
  int ret = get_fs_object(".", &fs_object);
  if (0 > ret) {
    printf("= error: failed to get filesystem information. consider submitting a bug report");
    return EXIT_FAILURE;
  }

  printf("- filesystem scan complete\n");
  do {
    printf("File `%s` at `%s`\n", fs_object.name, fs_object.path);
    fs_object = *fs_object.child;
  } while (fs_object.child != NULL);

  // load config file

  // clean up and exit
  printf("- cleaning up\n");
  free_fs_object(&fs_object);

  printf("= done\n");
  return EXIT_SUCCESS;
}
