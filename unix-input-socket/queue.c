#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

node_t *new_node(int data, node_t *next, node_t *prev) {
  node_t *n = NULL;
  n = (node_t *)malloc(sizeof(node_t));

  n->data = data;
  n->next = next;
  n->prev = prev;

  return n;
}

queue_t *new_queue() {
  queue_t *q = NULL;
  q = (queue_t *)malloc(sizeof(queue_t));

  q->head = NULL;
  q->tail = NULL;
  q->size = 0;

  return q;
}

void enqueue(queue_t *q, int data) {
  node_t *tmp = new_node(data, NULL, NULL);

  if (q->head == NULL) {
    q->head = tmp;
  } else {
    tmp->prev = q->tail;
    q->tail->next = tmp;
  }

  q->tail = tmp;
  q->size++;
}

void *dequeue(queue_t *q) {
  node_t *tmp = q->head;
  if (tmp == NULL) {
    return NULL;
  }

  q->head = q->head->next;

  tmp->next = NULL;
  tmp->prev = NULL;
  int result = tmp->data;

  q->size--;

  free(tmp);
  return &result;
}

void print_queue(queue_t *q) {
  node_t *curr = q->head;

  while (curr != NULL) {
    printf("%d\n", curr->data);
    curr = curr->next;
  }
}
