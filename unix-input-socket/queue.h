#ifndef HEADER_FILE
#define HEADER_FILE
typedef struct node {
  int data;
  struct node *next;
  struct node *prev;
} node_t;

typedef struct queue {
  node_t *head;
  node_t *tail;
  int size;
} queue_t;

queue_t *new_queue();
void enqueue(queue_t *q, int data);
void *dequeue(queue_t *q);
void print_queue(queue_t *q);
#endif
