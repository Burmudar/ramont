#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

void print_int(void *data) {
    int* d = (int *) data;

    printf("%d\n", *d);
}

int main(void) {
    queue_t *q = new_queue();
    printf("size: %d\n", q->size);

    for(int i=0; i < 10; i++) {
        int d = i;

        enqueue(q, d);
    }

    while(q->size > 0) {
        int *val = (int *)dequeue(q);
        if (val == NULL) break;
        printf("size: %d\nval: %d\n", q->size, *val);
    }

    free(q);
}
