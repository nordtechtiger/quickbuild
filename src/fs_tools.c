#include "fs_tools.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define FILE 8
#define DIRECTORY 4

#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)

int get_fs_object(const char *path, struct FsObject *fs_object);
struct FsObject *resolve_fs_recursive(DIR *dir_stream);
void free_fs_object(struct FsObject *fs_object);
void append_fs_object(struct FsObject *parent_fs_object,
                      struct FsObject *child_fs_object);
struct FsObject *find_fs_objects(char *pattern, uint32_t max_depth,
                                 struct FsObject *fs_object);
char* str_match(char *base, char *pattern);
int get_path_depth(char *path);

// gets a file system object tree
int get_fs_object(const char *path, struct FsObject *fs_object) {
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
      // allocate and fill out fields
      struct FsObject *child = calloc(1, sizeof(struct FsObject));
      child->name = calloc(strlen(dir_entry->d_name) + 1, sizeof(char));
      strcpy(child->name, dir_entry->d_name);

      child->path =
          calloc(strlen(path) + strlen(dir_entry->d_name) + 2, sizeof(char));
      strcpy(child->path, path);
      strcat(child->path, "/");
      strcat(child->path, dir_entry->d_name);

      // append to root object
      append_fs_object(fs_object, child);

    } else if (dir_entry->d_type == DIRECTORY) {
      // compute the new path (1 level deeper)
      char *child_path =
          calloc(strlen(path) + strlen(dir_entry->d_name) + 2, sizeof(char));
      strcpy(child_path, path);
      strcat(child_path, "/");
      strcat(child_path, dir_entry->d_name);

      // add all files in new path
      get_fs_object(child_path, fs_object);
      free(child_path);

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
  // if fs_object has a child node, free it first recursively
  if (fs_object->child != NULL) {
    free_fs_object(fs_object->child);
    fs_object->child = NULL;
  }

  // free fields
  free(fs_object->name);
  fs_object->name = NULL;
  free(fs_object->path);
  fs_object->path = NULL;

  // free itself
  free(fs_object);
}

void append_fs_object(struct FsObject *parent_fs_object,
                      struct FsObject *child_fs_object) {
  // iterate to find the last child object
  while (parent_fs_object->child != NULL) {
    parent_fs_object = parent_fs_object->child;
  }

  if (parent_fs_object->name == NULL && parent_fs_object->path == NULL) {
    // if parent is null, replace parent by child
    parent_fs_object->name = child_fs_object->name;
    parent_fs_object->path = child_fs_object->path;
    parent_fs_object->child = child_fs_object->child;

    // free or we will leak memory
    free(child_fs_object);

  } else {
    // if parent has content, append child at the end instead
    parent_fs_object->child = child_fs_object;
  }
}

// TODO: make this const char
struct FsObject *find_fs_objects(char *pattern, uint32_t max_depth,
                                 struct FsObject *fs_object) {
  uint32_t matches = 0;
  struct FsObject *results = calloc(1, sizeof(struct FsObject));

  while (1) {

    // if path matches, copy and add to results
    if ((get_path_depth(fs_object->path) <= max_depth || max_depth == 0) &&
        str_match(fs_object->path, pattern)) {
      struct FsObject *match = calloc(1, sizeof(struct FsObject));

      match->name = calloc(strlen(fs_object->name) + 1, sizeof(char));
      strcpy(match->name, fs_object->name);

      match->path = calloc(strlen(fs_object->path) + 1, sizeof(char));
      strcpy(match->path, fs_object->path);

      match->child = NULL;

      matches++;
      append_fs_object(results, match);
    }

    printf("matches: %i, done processing:%s\n", matches, fs_object->path);

    // check if we're reached the final fs_object
    if (!(fs_object = fs_object->child))
      break;
  }
}

// TODO: make this const char
char* str_match(char *base, char *pattern) {
  int32_t match_base = -1;
  int32_t match_top = -1;

  // advance base pointer for base string
  for (int i = 0; base[i] != '\0'; i++) {
    // iterate through pattern
    for (int j = 0; pattern[j] != '\0' && base[j+i] != '\0'; j++) {
      if (pattern[j] == '*') {
        
      } else if (pattern[j] != base[j+i]) {
          return NULL;
      }
    }
  }

  if (match_base != -1 && match_top != -1) {
    char* match = calloc(match_top - match_base, sizeof(char));
    strncpy(match, base, match_top - match_base);
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
