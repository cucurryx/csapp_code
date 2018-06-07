#include "csapp.h"

typedef struct {
    int maxfd;
    fd_set read_set;
    fd_set ready_set;
    int nready;
    int maxi;
    int clientfd[FD_SETSIZE];
    rio_t clientrio[FD_SETSIZE]; //1024
} pool;

int byte_cnt = 0;

void init_pool(int listenfd, pool *pool);
void add_client(int connfd, pool *pool);
void check_clients(pool *pool);

int main(int argc, char **argv) {
    fd_set fdset, ready_set;
    struct sockaddr_storage clientaddr;
    int listenfd, connfd;
    socklen_t clientlen;
    static pool pool;

    if (argc != 2) {
        fprintf(stderr, "Error: usage error!\n");
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);
    init_pool(listenfd, &pool);
    
    while (1) {
        pool.ready_set = pool.read_set;
        pool.nready = Select(pool.maxfd+1, &pool.ready_set, NULL, NULL, NULL);

        if (FD_ISSET(listenfd, &pool.ready_set)) {
            clientlen = sizeof(struct sockaddr_storage);
            connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
            add_client(connfd, &pool);
        }
        check_clients(&pool);

    }
}

void init_pool(int listenfd, pool *pool) {
    pool->maxi = -1;
    for (int i = 0; i < FD_SETSIZE; ++i) {
        pool->clientfd[i] = -1;
    }
    pool->maxfd = listenfd;
    FD_ZERO(&pool->read_set);
    FD_SET(listenfd, &pool->read_set);
}

void add_client(int connfd, pool *pool) {
    pool->nready--;
    for (int i = 0; i < FD_SETSIZE; ++i) {
        if (pool->clientfd[i] < 0) {
            pool->clientfd[i] = connfd;
            Rio_readinitb(&pool->clientrio[i], connfd);
            FD_SET(connfd, &pool->read_set);

            if (connfd > pool->maxfd) {
                pool->maxfd = connfd;
            }
            if (i > pool->maxi) {
                pool->maxi = i;
            }
            break;
        }
        if (i == FD_SETSIZE) {
            app_error("add_client error: Too many clients");
        }
    }
}

void check_clients(pool *pool) {
    int connfd, n;
    char buf[MAXLINE];
    rio_t rio;

    for (int i = 0; (i <= pool->maxi) && (pool->nready > 0); ++i) {
        connfd = pool->clientfd[i];
        rio = pool->clientrio[i];

        if ((connfd > 0) && (FD_ISSET(connfd, &pool->ready_set))) {
            pool->nready--;
            if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
                byte_cnt += n;
                printf("server received %d (%d total) bytes on fd %d\n", n, byte_cnt, connfd);
                Rio_writen(connfd, buf, n);
            } else {
                Close(connfd);
                FD_CLR(connfd, &pool->read_set);
                pool->clientfd[i] = -1;
            }
        }
    }
}