#include <dirent.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>

#include "path.h"

void free_path_list(path_list_t *r) {
  free(r->paths);
  free(r);
}

void free_path_single(path_single_t *r) {
  free(r->path);
  free(r);
}

path_list_t *find_all_in_path(char *path, char *pattern) {
  int length = 10;
  int count = 0;
  char **result = malloc(sizeof(char *) * length);
  regex_t regex;

  int r = regcomp(&regex, pattern, REG_EXTENDED);
  if (r) {
    return NULL;
  }

  DIR *d;
  struct dirent *dir;
  d = opendir(path);
  if (!d) {
    return NULL;
  }

  while ((dir = readdir(d)) != NULL) {

    int reti = regexec(&regex, dir->d_name, 0, NULL, 0);
    if (!reti) {
      result[count] = malloc(sizeof(char) * (strlen(path) + strlen(dir->d_name) + 2));
      // build and assign the full path!
      sprintf(result[count], "%s/%s", path, dir->d_name);

      count++;
    }
  }

  if (count == 0) {
      return NULL;
  }
  closedir(d);
  regfree(&regex);

  path_list_t *p = malloc(sizeof(path_list_t));
  p->paths = result;
  p->length = count;

  return p;
}

path_single_t *find_in_path(char *path, char *pattern) {
  path_single_t *result = malloc(sizeof(path_single_t));
  regex_t regex;

  int r = regcomp(&regex, pattern, REG_EXTENDED);
  if (r) {
    result->error = r;
    return result;
  }

  DIR *d;
  struct dirent *dir;
  d = opendir(path);
  if (!d) {
    result->error = DIR_ERROR;
    return result;
  }

  while ((dir = readdir(d)) != NULL) {
    printf("%s ...\n", dir->d_name);

    int reti = regexec(&regex, dir->d_name, 0, NULL, 0);
    if (!reti) {
      result->path =
          malloc(sizeof(char) * (strlen(path) + strlen(dir->d_name) + 2));
      sprintf(result->path, "%s/%s", path, dir->d_name);
      break;
    }
  }

  if (result->path == NULL) {
    result->error = REGX_NO_MATCH;
  }
  closedir(d);
  regfree(&regex);

  return result;
}
