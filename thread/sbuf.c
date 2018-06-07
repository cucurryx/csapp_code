#include "csapp.h"

/**
 *生产者-消费者模型 
 */

typedef struct {
    int *buf;
    int n;
    int front;
    int rear;
    sem_t mutex;
    sem_t slots;
    sem_t items;
} sbuf_t;

void sbuf_init(sbuf_t *sp, int n) {
    sp->n = n;
    sp->buf = Calloc(sizeof(int), n);
    sp->front = 0;
    sp->rear = 0;
    Sem_init(&sp->mutex, 0, 1);
    Sem_init(&sp->slots, 0, n);
    Sem_init(&sp->items, 0, 0);
}

void sbuf_deinit(sbuf_t *sp) {
    Free(sp->buf);
}

void sbuf_insert(sbuf_t *sp, int item) {
    P(&sp->slots); //有空闲空间
    P(&sp->mutex);
    sp->buf[(++sp->rear)%(sp->n)] = item;
    V(&sp->mutex);
    V(&sp->slots);
}

int sbuf_remove(sbuf_t *sp) {
    int item;
    P(&sp->items);
    P(&sp->mutex);
    item = sp->buf[(++sp->front)%(sp->n)];
    V(&sp->mutex);
    V(&sp->items);
    return item;
}