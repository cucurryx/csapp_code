#include "csapp.h"

void echo(int connfd) {
    char buf[MAXLINE];
    rio_t rio;
    size_t n;

    Rio_readinitb(&rio, connfd);
    Rio_readinitb(&rio, connfd);
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        printf("server received %d bytes\n", (int)n);
        Rio_writen(connfd, buf, n);
    }
}

void *thread(void *vargp) {
    int connfd = *(int*)vargp;
    Pthread_detach(pthread_self());
    Free(vargp);
    echo(connfd);
    Close(connfd);
    return NULL;
}

int main(int argc, char **argv) {
    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    if (argc != 2) {
        fprintf(stderr, "Error: usage error!\n");
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);
    while(1) {
        connfdp = Malloc(sizeof(int)); //每次都分配一块新的内存，防止进程间的竞争
        clientlen = sizeof(struct sockaddr_storage);
        *connfdp = Accept(listenfd, (SA*)&clientaddr, &clientlen);
        Pthread_create(&tid, NULL, thread, connfdp);
    }
    exit(0);
}