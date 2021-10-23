#include <cjson/cJSON.h>
#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <stdlib.h>

#include "cramont.h"
// Device
Device *cramont_new(char *name) {
  Device *device = malloc(sizeof(Device));
  device->name = name;

  return device;
}

int cramont_init_trackpad(Device *device, int x_dimension[2],
                          int y_dimension[2]) {
  device->type = DEVICE_TRACKPAD;

  struct libevdev *dev = libevdev_new();
  libevdev_set_name(dev, device->name);

  struct input_absinfo x_info;
  // Have to set all the values here others ... stuff just won't work!
  x_info.minimum = x_dimension[0];
  x_info.maximum = x_dimension[1];
  x_info.resolution = 100;
  x_info.flat = 0;
  x_info.fuzz = 0;
  int rc = libevdev_enable_event_code(dev, EV_ABS, ABS_X, &x_info);
  if (rc != 0) {
    fprintf(stderr, "error 1\n");
    return rc;
  }

  struct input_absinfo y_info;

  y_info.minimum = y_dimension[0];
  y_info.maximum = y_dimension[1];
  y_info.resolution = 100;
  y_info.flat = 0;
  y_info.fuzz = 0;
  rc = libevdev_enable_event_code(dev, EV_ABS, ABS_Y, &y_info);
  if (rc != 0) {
    return rc;
  }

  libevdev_enable_event_type(dev, EV_SYN);
  libevdev_enable_event_type(dev, EV_KEY);
  rc = libevdev_enable_event_code(dev, EV_KEY, BTN_TOUCH, NULL);
  if (rc < 0) {
    return rc;
  }
  rc = libevdev_enable_event_code(dev, EV_KEY, BTN_LEFT, NULL);
  if (rc < 0) {
    return rc;
  }
  rc = libevdev_enable_event_code(dev, EV_KEY, BTN_MIDDLE, NULL);
  if (rc < 0) {
    return rc;
  }
  rc = libevdev_enable_event_code(dev, EV_KEY, BTN_RIGHT, NULL);
  if (rc < 0) {
    return rc;
  }

  struct libevdev_uinput *input;
  rc = libevdev_uinput_create_from_device(dev, LIBEVDEV_UINPUT_OPEN_MANAGED,
                                          &input);
  if (rc < 0) {
    fprintf(stderr, "error 7\n");
    return rc;
  }

  device->device = dev;
  device->uinput = input;

  return 0;
}

void cramont_move(Device *device, int x, int y) {
  fprintf(stderr, "trackpad[%s](%d, %d)\n", device->name, x, y);

  if (device->type == DEVICE_TRACKPAD) {
    libevdev_uinput_write_event(device->uinput, EV_ABS, ABS_X, x);
    libevdev_uinput_write_event(device->uinput, EV_ABS, ABS_Y, y);
  }
  libevdev_uinput_write_event(device->uinput, EV_SYN, SYN_REPORT, 0);
}

void cramont_free_device(Device *device) {
  libevdev_uinput_destroy(device->uinput);
  libevdev_free(device->device);
  free(device);
}
// DeviceEvent

DeviceEvent *cramont_new_event() {
  DeviceEvent *e = malloc(sizeof(DeviceEvent));

  return e;
}

void cramont_free_event(DeviceEvent *e) {
  if (e != NULL) {
    return;
  }

  if (e->type != NULL) {
    free(e->type);
  }

  free(e);
}

void cramont_print_event(DeviceEvent *e) {
  fprintf(
      stderr,
      "\n---- DeviceEvent ----\ntype: %s\nunit_x: %5.16f\nunit_y: %5.16f\n\n",
      e->type, e->unit_x, e->unit_y);
}

int cramont_parse_event(char *data, DeviceEvent *dst) {
  cJSON *json = cJSON_Parse(data);

  cJSON *type = cJSON_GetObjectItemCaseSensitive(json, "type");
  if (cJSON_IsString(type) && (type->valuestring != NULL)) {
    dst->type = malloc(sizeof(type->valuestring));
    strcpy(dst->type, type->valuestring);
  }

  cJSON *x = cJSON_GetObjectItemCaseSensitive(json, "unitX");
  if (cJSON_IsNumber(x) && (x->valuedouble)) {
    dst->unit_x = x->valuedouble;
  }

  cJSON *y = cJSON_GetObjectItemCaseSensitive(json, "unitY");
  if (cJSON_IsNumber(y) && (y->valuedouble)) {
    dst->unit_y = y->valuedouble;
  }

  return 0;
}

Coord cramont_translate_event_to_coord(DeviceEvent *event, short width,
                                       short height) {
  Coord coord;
  double dx = event->unit_x * width;
  double dy = event->unit_y * height;

  coord.x = dx;
  coord.y = dy;
  return coord;
}

Coord cramont_delta_coord(Coord main, Coord last) {
  Coord coord;

  coord.x = main.x - last.x;
  coord.y = main.y - last.y;

  return coord;
}

void cramont_print_coord(Coord *coord) {
  fprintf(stderr, "translated: { %5.16f, %5.16f }\n", coord->x, coord->y);
}
