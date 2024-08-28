#include "fs_tools.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define FILE 8
#define DIRECTORY 4

int get_fs_object(const char *path, struct FsObject *fs_object);
struct FsObject *resolve_fs_recursive(DIR *dir_stream);
void free_fs_object(struct FsObject *fs_object);
void append_fs_object(struct FsObject *parent_fs_object,
                     struct FsObject *child_fs_object);

// gets a file system object tree
int get_fs_object(const char *path, struct FsObject *fs_object) {
  // printf("Entering path: %s\n", path);

  // initialize
  struct dirent *dir_entry;
  DIR *dir_stream;

  // open directory stream
  dir_stream = opendir(path);
  if (dir_stream == NULL) {
    return -1; // error
  }

  while ((dir_entry = readdir(dir_stream))) {
    // skip . and .. (recursion) and .git
    // TODO: make this configurable
    if (strcmp(dir_entry->d_name, ".") == 0 ||
        strcmp(dir_entry->d_name, "..") == 0 ||
        strcmp(dir_entry->d_name, ".git") == 0) {
      continue;
    }
    if (dir_entry->d_type == FILE) {
      // append file
      struct FsObject *child = calloc(1, sizeof(struct FsObject));
      child->name = calloc(strlen(dir_entry->d_name) + 1, sizeof(char));
      strcpy(child->name, dir_entry->d_name);
      child->path = calloc(strlen(path) + 1, sizeof(char));
      strcpy(child->path, path);
      append_fs_object(fs_object, child);
    } else if (dir_entry->d_type == DIRECTORY) {
      // recursively keep scanning directories
      char *child_path =
          calloc(strlen(path) + strlen(dir_entry->d_name) + 2, sizeof(char));
      strcpy(child_path, path);
      strcat(child_path, "/");
      strcat(child_path, dir_entry->d_name);
      get_fs_object(child_path, fs_object);
    } else {
      // if object found isn't a file or a directory - unsure if possible
      perror("unknown filesystem type encountered");
      return -1; // error
    }
  }

  closedir(dir_stream);

  return 0; // ok
}

void free_fs_object(struct FsObject *fs_object) {
  if (fs_object->child != NULL) {
    free_fs_object(fs_object->child);
    fs_object->child = NULL;
  }
  free(fs_object->name);
  fs_object->name = NULL;
  free(fs_object->path);
  fs_object->path = NULL;
}

void append_fs_object(struct FsObject *parent_fs_object,
                     struct FsObject *child_fs_object) {
  while (parent_fs_object->child != NULL) {
    parent_fs_object = parent_fs_object->child;
  }
  parent_fs_object->child = child_fs_object;
}
