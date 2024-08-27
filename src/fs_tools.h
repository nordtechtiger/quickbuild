#ifndef FS_TOOLS_H
#define FS_TOOLS_H
#include <dirent.h>
#include <stdint.h>

// represents a file system object
struct FsObject {
  char *name;
  char *path;
  uint32_t child_len;
  struct FsObject *child;
};

int get_fs_object(const char *path, struct FsObject *fs_object);
void free_fs_object(struct FsObject *fs_object);
int append_fs_object(struct FsObject *parent_fs_object,
                     struct FsObject *child_fs_object);

#endif
