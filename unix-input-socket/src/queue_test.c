#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"
#include "event.h"

void print_int(void *data) {
    int* d = (int *) data;

    printf("%d\n", *d);
}

int main(void) {
    queue *q = new_queue();
    printf("size: %d\n", q->size);

    for(int i=0; i < 10; i++) {
        Event* event = new_event();
        event->type = "test";
        event->unit_x = i * 1.0;
        event->unit_y = i * 1.0;

        enqueue(q, event);
    }

    while(q->size > 0) {
        void* data = dequeue(q);
        Event* val = (Event*)data;
        print_event(val);
        printf("size: %d\n", q->size);
        free_event(val);
    }

    free(q);
}
