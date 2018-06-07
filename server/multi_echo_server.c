#include "csapp.h"

void echo(int connfd) {
    char buf[MAXLINE];
    size_t n;
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        printf("server received %d bytes\n", (int)n);
        Rio_writen(connfd, buf, n);
    }
}

void command() {
    char buf[MAXLINE];
    if (!Fgets(buf, MAXLINE, stdin)) {
        exit(0);
    }
    printf("%s", buf);
}

int main(int argc, char **argv) {
    fd_set fdset, ready_set;
    struct sockaddr_storage clientaddr;
    int listenfd, connfd;

    if (argc != 2) {
        fprintf(stderr, "Error: usage error!\n");
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);
    FD_ZERO(&fdset);
    FD_SET(STDIN_FILENO, &fdset);
    FD_SET(listenfd, &fdset);
    
    while (1) {
        ready_set = fdset;
        Select(listenfd+1, &ready_set, NULL, NULL, NULL);
        if (FD_ISSET(STDIN_FILENO, &ready_set)) {
            command();
        }
        if (FD_ISSET(listenfd, &ready_set)) {
            socklen_t clientlen = sizeof(struct sockaddr_storage);
            connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
            echo(connfd);
            Close(connfd);
        }
    }
}