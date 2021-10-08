#ifndef HEADER_FILE
#define HEADER_FILE
#include <sys/queue.h>
typedef struct entry {
  void* data;
  TAILQ_ENTRY(entry) entries;
} entry;


TAILQ_HEAD(tailhead, entry);


typedef struct queue {
    struct tailhead head;
    int size;
} queue;

queue *new_queue();
void enqueue(struct queue *q, void* data);
void* dequeue(struct queue *q);
#endif
