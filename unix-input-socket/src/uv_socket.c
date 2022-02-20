#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cramont/cramont.h"
#include "path/path.h"
#include "queue/queue.h"
#include <X11/Xlib.h>
#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <linux/uinput.h>
#include <uv.h>

#define SOCKET_PATH "uv.socket"
#define DEVICES_SCAN_PATH "/dev/input/by-path"
#define TRUE 0
#define FALSE 1

typedef struct lock_queue {
  uv_mutex_t *lock;
  queue *queue;
} lock_queue;

typedef struct task_t {
  uv_cond_t *cond;
  uv_mutex_t *mutex;
  void *data;
  short done;
} task_t;

uv_loop_t *loop;
short dimensions[2];

lock_queue *lqueue;
uv_thread_t event_consumer_thread;
uv_cond_t *q_cond;

Device *device;

lock_queue *new_lock_queue() {
  lock_queue *lq = malloc(sizeof(lock_queue));

  uv_mutex_t lock;

  uv_mutex_init(&lock);

  lq->lock = &lock;
  lq->queue = new_queue();

  return lq;
}

void free_lock_queue(lock_queue *q) {
  uv_mutex_lock(q->lock);
  free(q->queue);
  uv_mutex_unlock(q->lock);
  uv_mutex_destroy(q->lock);
  free(q);
}

void q_event(lock_queue *lq, DeviceEvent *ev) {
  fprintf(stderr, "locking queue");
  uv_mutex_lock(lq->lock);

  enqueue(lq->queue, (void *)ev);

  fprintf(stderr, "unlocked queue");
  uv_mutex_unlock(lq->lock);
}

DeviceEvent *deq_event(lock_queue *lq) {
  fprintf(stderr, "locking queue");
  uv_mutex_lock(lq->lock);

  DeviceEvent *ev = (DeviceEvent *)dequeue(lq->queue);

  fprintf(stderr, "unlocked queue");
  uv_mutex_unlock(lq->lock);

  return ev;
}

void load_dimensions(short dimensions[]) {
  Display *display = XOpenDisplay(NULL);
  Screen *screen = DefaultScreenOfDisplay(display);
  dimensions[0] = screen->width;
  dimensions[1] = screen->height;
}

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

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
  // TODO: Need to look in reusing buffers
  // calloc zeroes out memory, if we use malloc there is left over garbage
  // memory like trailing  '}'
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
    int length = strlen(data) + 1; // plus 1 for \0

    // n is the number of arguments sscanf was able to assign, since we only
    // have one, it should be 0 / 1
    if (n == 0) {
      // sscanf was unable to assign data
      fprintf(stderr, "Failed to read event: '%s'\n", buf->base);
    } else if (length == nread) {

      DeviceEvent *event = cramont_new_event();
      cramont_parse_event(data, event);
      cramont_print_event(event);

      fprintf(stderr, "queueing work: %s\n", data);
      q_event(lqueue, event);

      if (lqueue->queue->size > 2) {
        fprintf(stderr, "signalling queue cond");
        uv_cond_signal(q_cond);
      }
    }
    if (length < nread) {
      // since there are more events, we should read the events here and add the
      // events to an array?
      fprintf(stderr,
              "[WARNING] More than one event in stream - only read first\n");
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

void remove_sock() {
  uv_fs_t req;

  uv_fs_unlink(loop, &req, SOCKET_PATH, NULL);
}

void clean_evdev() {}

void clean(int sig) {
  remove_sock();
  free_lock_queue(lqueue);
  clean_evdev();
  cramont_free_device(device);

  exit(0);
}

// Try to find the device path from the command line args, if not ...
// scan a known device path. We'll return a device path under
// two circumstances:
// * The user gave a path on the command line (argc > 1)
// * We only found one device at `/dev/input/by-path` that matches the regex
//   .*-event-mouse$
//
// If the above conditions don't hold - WHELP, then we show the user all the
// matching paths so that they can make an informed choice
//
// Also - if the conditions above are not met, you can expect a big fat NULL
// back
char *determine_device_path_from_args(char **argv, int argc) {
  if (argc > 1) {
    return argv[1];
  }

  path_list_t *result = find_all_in_path(DEVICES_SCAN_PATH, ".+-event-mouse$");

  if (result->error != 0) {
    fprintf(stderr,
            "Uh oh! Error! Some problem while scanning for devices in %s\n",
            DEVICES_SCAN_PATH);
    return NULL;
  }

  // Maybe we're lucky and only found one device!?
  if (result->length == 1) {
    fprintf(stderr, "Using only device found: %s\n", result->paths[0]);
  }

  // For now, lets show the found paths to the user and let them pick \o/
  // TODO: Maybe this should be read from a config file instead ?
  fprintf(stderr, "\nPick a path as pass it as an argument to uv_socket:\n");
  fprintf(stderr, "uv_socket /dev/input/by-path/example-event-mouse\n");
  fprintf(stderr, "\nAvailable device paths:\n");

  for (int i = 0; i < result->length; i++) {
    fprintf(stderr, "Path=%s\n", result->paths[i]);
  }
  free_path_list(result);

  return NULL;
  ;
}

void process_event(DeviceEvent *e) {

  char *now = time_now();
  fprintf(stderr, "\n[%s] Got event data\n", now);
  cramont_print_event(e);

  Coord mouse_coords =
      cramont_translate_event_to_coord(e, dimensions[0], dimensions[1]);

  cramont_print_coord(&mouse_coords);

  free(now);
  cramont_free_event(e);
}


void event_consumer(void *arg) {
  fprintf(stderr, "starting up event consumer\n");
  task_t *task = ((task_t *)arg);

  lock_queue *lq = (lock_queue *)task->data;

  while (task->done != TRUE) {
    //fprintf(stderr, "locking task mutex\n");
    //uv_mutex_lock(task->mutex);
    fprintf(stderr, "waiting on task condition\n");
    uv_cond_wait(task->cond, task->mutex);

    if (lq->queue->size >= 1) {
      DeviceEvent *e = (DeviceEvent *)deq_event(lq);
      fprintf(stderr, "deq event");

      process_event(e);
    }

    fprintf(stderr, "after waiting on task condition");
    //uv_mutex_unlock(task->mutex);
  }
}

void init_task_t(task_t *t) {
  // Initialize as task
  // The Task has a sync condition and a mutex so that we only process the queue
  // (data) when it gets a signal
  uv_cond_t cond;
  uv_mutex_t mutex;
  int rc = uv_cond_init(&cond);
  if (rc != 0) {
      fprintf(stderr, "failed to init cond_init");
  }
  uv_mutex_init(&mutex);
  if (rc != 0) {
      fprintf(stderr, "failed to init mutex_init");
  }

  t->done = FALSE;
  t->cond = &cond;
  t->mutex = &mutex;

  t->data = (void *)new_lock_queue();
}

int main(int argc, char **argv) {
  // TODO: Check that we're sudo
  load_dimensions(dimensions);
  fprintf(stderr, "\nWidth: %hd Height: %hd\n", dimensions[0], dimensions[1]);

  device = cramont_new("ramont_trackpad");
  int x[] = {0, dimensions[0] / 2};
  int y[] = {0, dimensions[1]};
  int rc = cramont_init_trackpad(device, x, y);
  if (rc < 0) {
    fprintf(stderr, "failed to init fake trackpad: %s", strerror(-rc));
    exit(rc);
  }

  loop = uv_default_loop();

  task_t event_task;
  init_task_t(&event_task);
  fprintf(stderr, "initialized even_task\n");

  q_cond = event_task.cond;

  uv_thread_create(&event_consumer_thread, event_consumer, &event_task);

  uv_pipe_t server;
  uv_pipe_init(loop, &server, 0);

  signal(SIGINT, clean);

  int r;
  if ((r = uv_pipe_bind(&server, SOCKET_PATH))) {
    fprintf(stderr, "Bind error: %s\n", uv_err_name(r));
  }

  if ((r = uv_listen((uv_stream_t *)&server, 128, on_new_connection))) {
    fprintf(stderr, "Listen error: %s\n", uv_err_name(r));
    return 2;
  }

  fprintf(stderr, "Starting loop");

  return uv_run(loop, UV_RUN_DEFAULT);
}
