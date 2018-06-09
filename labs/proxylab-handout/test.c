#include "csapp.h"
#define HOSTNAME_MAX_LEN 63
#define HEADER_NAME_MAX_LEN 32
#define HEADER_VALUE_MAX_LEN 64

/**
 * 
 */
typedef struct {
    char hostname[HOSTNAME_MAX_LEN];
    char path[MAXLINE];
} RequestLine;

typedef struct {
    char name[HEADER_NAME_MAX_LEN];
    char value[HEADER_VALUE_MAX_LEN];
} RequestHeader;

RequestHeader parse_header(char *line) {
    RequestHeader header;
    char *c = strstr(line, ": ");
    *c = '\0';

    strncpy(header.name, line, c-line);
    strcpy(header.value, c+2);

    printf("pre: %s %s\n", header.name, line);
    printf("after cut: %s\n", header.value);
    return header;
}

void parse_uri(char *uri, RequestLine *linep) {
    if (strstr(uri, "http://") != uri) {
        fprintf(stderr, "Error: invalid uri!\n");
        exit(0);
    }
    uri += 7;
    char *c = strstr(uri, "/");
    *c = '\0';
    strcpy(linep->hostname, uri);
    *c = '/';
    strcpy(linep->path, c);
}

int main() {
    char sts[10][100] = {
      //  "Host: localhost:7778",
      //  "User-Agent: curl/7.55.1",
      //  "Accept: */*",
        "Proxy-Connection: Keep-Alive"
    };

    for (int i = 0; i < 1; ++i) {
        RequestHeader header = parse_header(sts[i]);
        printf("%s -- %s\n", header.name, header.value);
    }

    char uri[10][100] = {
        "http://localhost:7778/home.html"
    };

    for (int i = 0; i < 1; ++i) {
        RequestLine line;
        parse_uri(uri[i], &line);
        printf("hostname:%s\npath:%s\n", line.hostname, line.path);
    }

}