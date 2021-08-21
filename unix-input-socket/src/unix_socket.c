#include <asm-generic/errno-base.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCK_PATH "echo_socket"
#define TRUE 1
#define FALSE 0

char *concat(char *a, char *b) {
  size_t aSize = strlen(a);
  size_t bSize = strlen(b);
  size_t size = sizeof(char) * (aSize + bSize + 1);

  char *c = malloc(size);
  memcpy(c, a, aSize);
  memcpy(c + aSize, b, bSize);
  c[aSize + bSize] = '\0';

  return c;
}

char *int_to_string(int *num) {
  char *format = "%d";
  int length = snprintf(NULL, 0, format, *num);
  // + 1 for null terminating char
  char *data = malloc(length + 1);
  snprintf(data, length + 1, format, *num);

  return data;
}

int main(void) {
  int s, s2, t, len;
  struct sockaddr_un local, remote;
  char str[100];

  if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }

  local.sun_family = AF_UNIX;
  strcpy(local.sun_path, SOCK_PATH);
  unlink(local.sun_path);
  len = strlen(local.sun_path) + sizeof(local.sun_family);
  if (bind(s, (struct sockaddr *)&local, len) == -1) {
    perror("bind");
    exit(1);
  }

  if (listen(s, 5) == -1) {
    perror("listen");
    exit(1);
  }

  for (;;) {
    int done, n;
    printf("Waiting for connection ...\n");
    t = sizeof(remote);
    if ((s2 = accept(s, (struct sockaddr *)&remote, &t)) == -1) {
      perror("accept");
      exit(1);
    }

    printf("Connected.\n");

    done = FALSE;
    do {
      n = recv(s2, str, 100, 0);
      if (n <= 0) {
        if (n < 0)
          perror("recv");
        done = TRUE;
      }

      printf("recv> %s\n", str);

      char *data = concat(str, "\n");

      /*
      int num = strtol(str, NULL, 10);
      if (errno == ERANGE || errno == EINVAL) {
        perror("conversion");
      }
      num++;

      char *val = int_to_string(&num);
      char *data = concat(val, "\n");
      free(val);
      */

      if (!done) {
        if (send(s2, data, sizeof(char) * strlen(data), 0) < 0) {
          perror("send");
          done = TRUE;
        } else {
          printf("sending>'%s'", data);
        }
      }
      free(data);
    } while (!done);

    close(s2);
  }

  return 0;
}
