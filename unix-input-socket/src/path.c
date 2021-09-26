#include <dirent.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>

#include "path.h"

void free_path_result(path_result_t *r) {
  free(r->path);
  free(r);
}

path_result_t *find_in_path(char *path, char *pattern) {
  path_result_t *result = malloc(sizeof(path_result_t));
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
