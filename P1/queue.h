#ifndef _QUEUE_H_
#define _QUEUE_H_
#include "iterator.h"

typedef struct queue Queue;

const Queue *Queue_create(long capacity);

struct queue {
    void *self;
    void (*destroy)(const Queue *q, void (*freeFxn)(void *item));
    int (*enqueue)(const Queue *q, void *item);
    int (*dequeue)(const Queue *q, void **item);
    int (*front)(const Queue *q, void **item);
    long (*size)(const Queue *q);
    int (*isEmpty)(const Queue *q);
    const Iterator *(*itCreate)(const Queue *q);
};
#endif /* _QUEUE_H_ */
