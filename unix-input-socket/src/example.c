#include <dirent.h>
#include <fcntl.h>
#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "path.h"

int main(int argc, char **argv) {
  char* path = NULL;

  printf("Args: %d\n", argc);

  if (argc == 1) {
    path_list_t *result =
        find_all_in_path("/dev/input/by-path", ".+-event-mouse$");
    if (result->error == 0) {
      fprintf(stderr, "Found stuff!\n");
    } else {
      fprintf(stderr, "Uh oh! Error!");
    }
    for(int i =0; i < result->length; i++) {
        fprintf(stderr, "Path=%s\n", result->paths[i]);
    }
    free_path_list(result);
  } else {
    path = argv[1];
  }

  struct libevdev *dev;
  struct libevdev_uinput *uidev;

  int i = 50;

  int fd = open(path, O_RDWR | O_NONBLOCK);
  if (fd < 0) {
    fprintf(stderr, "File - failed to open '%s': %s", path,
            strerror(fd));
  }

  int rc = libevdev_new_from_fd(fd, &dev);
  if (rc < 0) {
    printf("Mouse: Failed to init libevdev (%s)\n", strerror(-rc));
    exit(1);
  }
  printf("TEST 1\n");

  rc = libevdev_uinput_create_from_device(dev, LIBEVDEV_UINPUT_OPEN_MANAGED,
                                          &uidev);
  if (rc < 0) {
    printf("Mouse: Failed to init evdev-uinput (%s)\n", strerror(-rc));
    exit(1);
  }
  printf("TEST 2\n");

  sleep(1);

  while (i--) {
    libevdev_uinput_write_event(uidev, EV_REL, REL_X, 5);
    libevdev_uinput_write_event(uidev, EV_REL, REL_Y, 5);
    libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 5);
    usleep(15000);
  }

  sleep(1);

  libevdev_uinput_destroy(uidev);
  libevdev_free(dev);
  close(fd);

  return 0;
}
