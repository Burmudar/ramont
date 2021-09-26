#include <dirent.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct path_result_t {
  char *path;
  int error;
} path_result_t;

void free_path_result(path_result_t *r);
path_result_t *find_in_path(char *path, char *pattern);
