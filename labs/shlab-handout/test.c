#include <stdio.h>
#include <unistd.h>

int main() {
    int x = 0;
    if (fork() == 0) {
        x++;
        printf("child pid:%d x:%d address:%p\n\n", getpid(), x, &x);
        exit(0);
    }
    x--;
    printf("parent pid:%d x:%d address:%p\n", getpid(), x, &x);
    exit(0);
}