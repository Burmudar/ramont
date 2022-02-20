#pragma once
#include <dirent.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIR_ERROR 1
#define REGX_NO_MATCH 2

typedef struct path_single_t {
  char *path;
  int error;
} path_single_t;

typedef struct path_list_t {
  char **paths;
  int length;
  int error;
} path_list_t;

void free_path_single(path_single_t *r);
void free_path_list(path_list_t *r);
path_single_t *find_in_path(char *path, char *pattern);
path_list_t *find_all_in_path(char *path, char *pattern);
