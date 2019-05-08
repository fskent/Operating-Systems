#include "queue.h"
#include <stdlib.h>

#define incrc(i, n) (((i+1) >= n) ? 0: i+1)
#define DEFAULT_CAPACITY 50L
#define MAX_INIT_CAPACITY 1000L

typedef struct q_data {
    long head;
    long tail;
    long nitems;
    long length;
    void **theArray;
} Q_Data;

static void q_purge(Q_Data *qd, void (*freeFxn)(void *item)) {
    if (freeFxn != NULL) {
        long i, n = qd->nitems;

        for (i = qd->head; n-- > 0; i = incrc(i, qd->length)) {
            (*freeFxn)(qd->theArray[i]);
        }
    }
}

static void q_destroy(const Queue *q, void (*freeFxn)(void *item)) {
    Q_Data *qd = (Q_Data *)q->self;

    q_purge(qd, freeFxn);
    free(qd->theArray);
    free(q->self);
    free((void *)q);
}

static int q_enqueue(const Queue *q, void *item) {
    Q_Data *qd = (Q_Data *)q->self;
    int status = 1;

    if (qd->nitems == qd->length) {
        size_t nbytes = 2 * qd-> length * sizeof(void *);
        void **tmp = (void **)malloc(nbytes);
        if (tmp == NULL) {
            status = 0;
        } else {
            long i, j, n = qd->nitems;

            for (i = qd->head, j = 0; n-- > 0; i = incrc(i, qd->length), j++)
                tmp[j] = qd->theArray[i];
            free(qd->theArray);
            qd->theArray = tmp;
            qd->head = 0;
            qd->tail = qd->nitems;
            qd->length *= 2;
        }
    }
    if (status) {
        qd->theArray[qd->tail] = item;
        qd->tail = incrc(qd->tail, qd->length);
        qd->nitems++;
    }
    return status;
}

static int q_dequeue(const Queue *q, void **item) {
    Q_Data *qd = (Q_Data *)q->self;
    int status = 0;

    if (qd->nitems > 0L) {
        *item = qd->theArray[qd->head];
        qd->head = incrc(qd->head, qd->length);
        qd->nitems--;
        status = 1;
    }
    return status;
}

static int q_front(const Queue *q, void **item) {
    Q_Data *qd = (Q_Data *)q->self;
    int status = 0;

    if (qd->nitems > 0L) {
        *item = qd->theArray[qd->head];
        status = 1;
    }
    return status;
}

static long q_size(const Queue *q) {
    Q_Data *qd = (Q_Data *)q->self;

    return qd->nitems;
}

static int q_isEmpty(const Queue *q) {
    Q_Data *qd = (Q_Data *)q->self;

    return qd->nitems == 0L;
}

static void **q_arrayDupl(Q_Data *qd) {
    void **tmp = NULL;

    if (qd->nitems > 0L) {
        size_t nbytes = qd->nitems * sizeof(void *);
        tmp = (void **)malloc(nbytes);
        if (tmp != NULL) {
            long i, j, n = qd->nitems;

            for (i = qd->head, j = 0; n-- > 0; i = incrc(i, qd->length), j++)
                tmp[j] = qd->theArray[i];
        }
    }
    return tmp;
}

static const Iterator *q_itCreate(const Queue *q) {
    Q_Data *qd = (Q_Data *)q->self;
    const Iterator *it = NULL;
    void **tmp = q_arrayDupl(qd);

    if (tmp != NULL) {
        it = Iterator_create(qd->nitems, tmp);
        if (it == NULL)
            free(tmp);
    }
    return it;
}

static Queue template = {
    NULL, q_destroy, q_enqueue, q_dequeue, q_front, q_size,
    q_isEmpty, q_itCreate
};

const Queue *Queue_create(long capacity) {
    Queue *q = (Queue *)malloc(sizeof(Queue));

    if (q != NULL) {
        Q_Data *qd = (Q_Data *)malloc(sizeof(Q_Data));

        if (qd != NULL) {
            long cap;
            void **array = NULL;

            cap = (capacity <= 0L) ? DEFAULT_CAPACITY : capacity;
            cap = (cap > MAX_INIT_CAPACITY) ? MAX_INIT_CAPACITY : cap;
            array = (void **)malloc(cap * sizeof(void *));
            if (array != NULL) {
                qd->length = cap;
                qd->head = 0L;
                qd->tail = 0L;
                qd->nitems = 0L;
                qd->theArray = array;
                *q = template;
                q->self = qd;
            } else {
                free(qd);
                free(q);
                q = NULL;
            }
        } else {
            free(q);
            q = NULL;
        }
    }
    return q;
}
