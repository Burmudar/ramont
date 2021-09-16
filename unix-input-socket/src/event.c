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
    free(e->type);
}

void print_event(Event* e) {
    fprintf(stderr, "\n---- Event ----\ntype: %s\noffsetX: %5.16f\noffsetY: %5.16f\n\n", e->type, e->offsetX, e->offsetY);
}

int parse_event(char* data, Event* dst) {
    cJSON *json = cJSON_Parse(data);

    cJSON *type = cJSON_GetObjectItemCaseSensitive(json, "type");
    if (cJSON_IsString(type) && (type->valuestring != NULL)) {
        dst->type = malloc(sizeof(type->valuestring));
        strcpy(dst->type, type->valuestring);
    }

    cJSON *x = cJSON_GetObjectItemCaseSensitive(json, "offsetX");
    if (cJSON_IsNumber(x) && (x->valuedouble)) {
        dst->offsetX = x->valuedouble;
    }

    cJSON *y = cJSON_GetObjectItemCaseSensitive(json, "offsetY");
    if (cJSON_IsNumber(y) && (y->valuedouble)) {
        dst->offsetY = y->valuedouble;
    }

    return 0;
}
