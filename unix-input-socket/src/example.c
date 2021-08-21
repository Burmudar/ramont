#include <fcntl.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(void) {
  struct uinput_setup usetup;
  struct libevdev *dev;
  struct libevdev_uinput *uidev;

  int i = 50;

  int fd = open("/dev/input/event2", O_WRONLY | O_NONBLOCK);
  int ufd = open("/dev/uinput", O_RDWR);

  libevdev_new_from_fd(fd, &dev);
  libevdev_uinput_create_from_device(dev, ufd, &uidev);

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
  close(ufd);

  return 0;
}
