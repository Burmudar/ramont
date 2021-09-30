#pragma once
#include <stdio.h>
#include <stdlib.h>

#include <cjson/cJSON.h>

typedef struct Event {
    char* type;
    double unit_x;
    double unit_y;
} Event;

typedef struct Coord {
    double x;
    double y;
} Coord;

typedef struct MouseEvent {
    char* type;
} MouseEvent;

Event* new_event();

void free_event(Event* e);
void print_event(Event* e);
int parse_event(char* data, Event* dst);

Coord translate_event_coord(Event* event, short width, short height);
Coord delta_coord(Coord value, Coord last);
void print_coord(Coord* coord);
