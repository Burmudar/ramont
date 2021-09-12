#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <uv.h>

#define SOCKET_PATH "uv.socket"

uv_loop_t *loop;
uv_async_t async;

long current_millis() {
  struct timeval time;
  gettimeofday(&time, NULL);

  return time.tv_sec * 1000 + time.tv_usec / 1000;
}

char *time_now() {
  char *buff = malloc(sizeof(char) * 80);
  char b[26];
  time_t t;
  time(&t);
  strftime(b, 26, "%Y-%m-%d %H:%M:%S", localtime(&t));
  sprintf(buff, "%s.%0.3ld", b, current_millis());
  return buff;
}

void update_mouse(uv_work_t *req) {
  char *data = ((char *)req->data);

  char *now = time_now();
  fprintf(stderr, "[%s] Got mouse data: %s\n", now, data);

  /*double points[2] = {1.0, 2.0};
  async.data = (void *)points;
  uv_async_send(&async);*/
  free(now);
}

void print_mouse_change(uv_async_t *handle) {
  double *point = ((double *)handle->data);
  fprintf(stderr, "Moved mouse - x: %f y: %f\n", *point, *(point + 1));
}

void cleanup(uv_work_t *req, int status) {
  fprintf(stderr, "cleaning up after mouse change");
  char *data = ((char *)req->data);
  free(data);
  // we should probably not clean async up here since multiple work requests
  // will use this async ? uv_close((uv_handle_t *)&async, NULL);
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
  // TODO: Need to look in reusing buffers
  // calloc zeroes out memory, if we use malloc there is left over garbage memory like trailing  '}'
  buf->base = calloc(suggested_size, sizeof(char));
  buf->len = suggested_size;
}

void process_data(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
  fprintf(stderr, "buff size: %ld buff len: %zu nread: %zd\n",
          sizeof(buf->base), buf->len, nread);
  if (nread > 0) {

    uv_work_t req;

    char *data = malloc(sizeof(char) * nread);
    // events are supposed to be separated by '\n'
    int n = sscanf(buf->base, "%[^\n]", data);
    int length = strlen(data);

    // n is the number of arguments sscanf was able to assign, since we only have one, it should be 0 / 1
    if (n == 0) {
      //sscanf was unable to assign data
      fprintf(stderr, "Failed to read event: '%s'\n", buf->base);
    } else if (length + 1 == nread) {
      req.data = (void *)data;
      fprintf(stderr, "queueing work: %s\n", data);
      int r = uv_queue_work(loop, &req, update_mouse, cleanup);
      if (r != 0)
        fprintf(stderr, "failed to queue work: %d", r);
    }
    if (length < nread) {
      // since there are more events, we should read the events here and add the events to an array?
      fprintf(stderr, "[WARNING] More than one event in stream - only read first\n");
      fprintf(stderr, "Data: '%s'\n", buf->base);
    }
  }

  if (nread < 0) {
    if (nread != UV_EOF) {
      fprintf(stderr, "Read error %s\n", uv_err_name(nread));
    }
    uv_close((uv_handle_t *)client, NULL);
  }

  if (buf->base) {
    free(buf->base);
  }
}

void on_new_connection(uv_stream_t *server, int status) {
  if (status == -1) {
    return;
  }

  uv_pipe_t *client = (uv_pipe_t *)malloc(sizeof(uv_pipe_t));
  uv_pipe_init(loop, client, 0);

  if (uv_accept(server, (uv_stream_t *)client) == 0) {
    uv_read_start((uv_stream_t *)client, alloc_buffer, process_data);
  } else {
    uv_close((uv_handle_t *)client, NULL);
  }
}

void remove_sock(int sig) {
  uv_fs_t req;

  uv_fs_unlink(loop, &req, SOCKET_PATH, NULL);
  exit(0);
}

int main() {

  loop = uv_default_loop();

  uv_async_init(loop, &async, print_mouse_change);

  uv_pipe_t server;
  uv_pipe_init(loop, &server, 0);

  signal(SIGINT, remove_sock);

  int r;
  if ((r = uv_pipe_bind(&server, SOCKET_PATH))) {
    fprintf(stderr, "Bind error: %s\n", uv_err_name(r));
  }

  if ((r = uv_listen((uv_stream_t *)&server, 128, on_new_connection))) {
    fprintf(stderr, "Listen error: %s\n", uv_err_name(r));
    return 2;
  }

  return uv_run(loop, UV_RUN_DEFAULT);
}
