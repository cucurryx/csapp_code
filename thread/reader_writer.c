#include "csapp.h"

/**
 * 读者-写者模型，读者优先
 */

int read_cnt = 0;
sem_t mutex, w;

void reader() {
    while (1) {
        P(&mutex);
        ++read_cnt;
        if (read_cnt == 1) {
            P(&w); //第一个进来写的进程，把读者给锁上
        }
        V(&mutex);

        //do something

        P(&mutex);
        --read_cnt;
        if (read_cnt == 0) {
            V(&w); //最后一个写完了的进程，给读者解锁
        }
        V(&mutex);  
    }           
}

void writer() {
    while (1) {
        P(&w);
        //do something
        V(&w);
    }
}