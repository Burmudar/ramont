#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

void enqueue(struct queue *q, int val) {
    struct entry *n = malloc(sizeof(struct entry));

    n->data = val;

    TAILQ_INSERT_TAIL(&q->head, n, entries);

    q->size++;
}

int dequeue(struct queue *q) {
    struct entry *item = TAILQ_FIRST(&q->head);
    int result = item->data;

    TAILQ_REMOVE(&q->head, item, entries);
    q->size--;

    return result;
}

struct queue* new_queue()  {
    struct queue *q;
    q = malloc(sizeof(struct queue));

    TAILQ_INIT(&q->head);

    q->size = 0;

    return q;
}
