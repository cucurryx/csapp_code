#include "csapp.h"

void echo(int connfd) {
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        printf("server received %d bytes\n", (int)n);
        Rio_writen(connfd, buf, n);
    }
}

void sigchld_handler(int sig) {
    while (waitpid(-1, 0, WNOHANG) > 0) {
        ;
    }
    return;
}

int main(int argc, char **argv) {
    char *port;
    int listenfd, connfd, sockfd;
    struct sockaddr_storage clientaddr;
    socklen_t client_len;

    if (argc != 2) {
        fprintf(stderr, "Error: wrong arguments.\n");
        exit(0);
    }

    port = argv[1];
    Signal(SIGCHLD, sigchld_handler);
    listenfd = Open_listenfd(port);
    while (1) {
        client_len = sizeof(struct sockaddr_storage);
        //对监听的fd，进行accept，返回一个sockaddr，存储了有关信息
        connfd = Accept(listenfd, (SA*)&clientaddr, &client_len);
        if (Fork() == 0) {
            Close(listenfd); //子进程中不需要再监听，只需要完成这次请求就行
            echo(connfd); //把送来的信息读取出来，然后再又写进去
            Close(connfd);
            exit(0);
        }
        Close(connfd); //每次请求之后，需要关闭这次的connect fd，否则会资源泄漏
    }
}