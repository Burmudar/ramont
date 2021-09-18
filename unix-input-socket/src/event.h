#pragma once
#include <stdio.h>
#include <stdlib.h>

#include <cjson/cJSON.h>

typedef struct Event {
    char* type;
    double offsetX;
    double offsetY;
} Event;

Event* new_event();
void free_event(Event* e);
void print_event(Event* e);
int parse_event(char* data, Event* dst);
