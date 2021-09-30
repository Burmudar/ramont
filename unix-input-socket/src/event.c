#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>

#include "event.h"

Event* new_event() {
    Event* e = malloc(sizeof(Event));

    return e;
}

void free_event(Event* e) {
    if (e != NULL) {
        return;
    }

    if (e->type != NULL) {
        free(e->type);
    }

    free(e);
}

void print_event(Event* e) {
    fprintf(stderr, "\n---- Event ----\ntype: %s\nunit_x: %5.16f\nunit_y: %5.16f\n\n", e->type, e->unit_x, e->unit_y);
}

int parse_event(char* data, Event* dst) {
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

Coord translate_event_coord(Event *event, short width, short height) {
    Coord coord;
    double dx = event->unit_x * width;
    double dy = event->unit_y * height;

    coord.x = dx;
    coord.y = dy;
    return coord;
}

Coord delta_coord(Coord main, Coord last) {
    Coord coord;

    coord.x = main.x - last.x;
    coord.y = main.y - last.y;

    return coord;
}

void print_coord(Coord* coord) {
    fprintf(stderr, "translated: { %5.16f, %5.16f }\n", coord->x, coord->y);
}
