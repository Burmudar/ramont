#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <linux/uinput.h>
#include <cjson/cJSON.h>

#define DEVICE_TRACKPAD 1

typedef struct Device {
    char* name;
    struct libevdev *device;
    struct libevdev_uinput *uinput;
    int type;
} Device;

typedef struct DeviceEvent {
    char* type;
    double unit_x;
    double unit_y;
} DeviceEvent;

typedef struct Coord {
    double x;
    double y;
} Coord;

Device* cramont_new(char* name);
int cramont_init_trackpad(Device* device, int x_dimension[2], int y_dimension[2]);
void cramont_action(Device* device, DeviceEvent* event);
void cramont_move(Device* device, int x, int y);
void cramont_free_device(Device *device);

DeviceEvent* cramont_new_event();

void cramont_free_event(DeviceEvent* e);
void cramont_print_event(DeviceEvent* e);
int cramont_parse_event(char* data, DeviceEvent* dst);

Coord cramont_translate_event_to_coord(DeviceEvent* event, short width, short height);
Coord cramont_delta_coord(Coord value, Coord last);
void cramont_print_coord(Coord* coord);

