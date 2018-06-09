# CSAPP Lab -- Proxy Lab

## 准备
这是最后一个lab，要求实现一个并发Web代理。相比本书前半部分的lab，这个任务很容易完成，不需要苦思冥想。读过最后三章的内容，就可以开始了。需要的知识点有：
- client-server模型
- 简单的socket编程
- Web服务器基础和HTTP协议基础
- 多线程并发编程，读者-写者模型
- Cache相关知识(可选)

先去[官网](http://csapp.cs.cmu.edu/3e/labs.html)下载所需的材料，一定要下载**Writeup**，它会提供很大的帮助。阅读Writeup，可以了解到一些实验相关的信息：
- Web代理是一个Web服务器和浏览器之间的中间程序。浏览器和代理连接，然后代理再将请求转发给服务器。服务器的相应，也由代理返回给浏览器。
- `port-for-user.pl`用来自动生成可用端口(没啥用，可以自己随便选个比较大的，基本都能用)
- 附赠一个tiny web服务器，用来进行测试
- 实验分为三步。第一步实现基本的转发，第二步实现并发，第三步加入cache功能
- 可以用`curl`这个工具来进行测试，例如:` curl -v --proxy http://localhost:15214 http://localhost:15213/home.html`就是向代理`http://localhost:15214`发送请求，得到后面那个uri的资源

弄清实验要求后，就可以构思大致的思路了。类似于web服务器，只是接受client的request后，解析一下，然后将信息转发给对应的服务器，等服务器响应后，将responce信息从服务器对应的描述符中读取出来，再写入和client对应的描述符就完成了。

## Part1: 简单实现
HTTP request中包含请求行和请求头，所以我们可以设计对应的数据结构，方便存储解析后的数据:
```C
typedef struct {
    char host[HOSTNAME_MAX_LEN];
    char port[PORT_MAX_LEN];
    char path[MAXLINE];
} ReqLine;

typedef struct {
    char name[HEADER_NAME_MAX_LEN];
    char value[HEADER_VALUE_MAX_LEN];
} ReqHeader;
```
请求行实际上是一个`map`，但是C语言实现比较麻烦，就直接用了一个数组来保存。

回想一下web服务器的实现，`main()`函数里面`bind`套接字、`listen`一个端口、`accept`等待连接，然后再处理这个描述符。这个过程完成就和代理一样，可以直接套用，不同的只是处理函数:
```C
int main(int argc, char **argv) 
{
    int listenfd, *connfd;
    pthread_t tid;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if (argc != 2) {
	    fprintf(stderr, "usage: %s <port>\n", argv[0]);
	    exit(1);
    }

    init_cache();
    listenfd = Open_listenfd(argv[1]);
    while (1) {
	    clientlen = sizeof(clientaddr);
        connfd = Malloc(sizeof(int));
	    *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        Pthread_create(&tid, NULL, thread, connfd); //处理函数，这个是create了一个新线程
    }
}
```

在创建新线程后，它会自动去执行`thread()`函数，然后`thread()`函数会去调用主要的处理函数`doit()`。如果不看并发实现部分的代码，`doit()`函数中的逻辑大概就是:`parse_request`得到解析后的请求行和请求头，然后`send_to_server()`，让他去连接对应的服务器，向服务器发送请求。和服务器建立连接后，返回的信息会在描述符`connfd`中，也就是`send_to_server()`函数的返回值。然后再把信息从`connfd`中读取出来，直接写进客户端对应的描述符`fd`就行了。

```C
void doit(int fd) {
    char buf[MAXLINE], uri[MAXLINE], object_buf[MAX_OBJECT_SIZE];
    int total_size, connfd;
    rio_t rio;
    ReqLine request_line;
    ReqHeader headers[20];
    int num_hds, n;
    parse_request(fd, &request_line, headers, &num_hds);

    strcpy(uri, request_line.host);
    strcpy(uri+strlen(uri), request_line.path);
    if (reader(fd, uri)) {
        fprintf(stdout, "%s from cache\n", uri);
        fflush(stdout);
        return;
    }

    total_size = 0;
    connfd = send_to_server(&request_line, headers, num_hds);
    Rio_readinitb(&rio, connfd);
    while ((n = Rio_readlineb(&rio, buf, MAXLINE))) {
        Rio_writen(fd, buf, n);
        strcpy(object_buf + total_size, buf);
        total_size += n;
    }
    if (total_size < MAX_OBJECT_SIZE) {
        writer(uri, object_buf);
    }
    Close(connfd);
}
```

解析部分的代码很简单，就不解释了:
```C
void parse_request(int fd, ReqLine *linep, ReqHeader *headers, int *num_hds) {
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))  
        return;

    //parse request line
    sscanf(buf, "%s %s %s", method, uri, version);
    parse_uri(uri, linep);

    //parse request headers
    *num_hds = 0;
	Rio_readlineb(&rio, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {
	    headers[(*num_hds)++] = parse_header(buf);
	    Rio_readlineb(&rio, buf, MAXLINE);
    }
}

void parse_uri(char *uri, ReqLine *linep) {
    if (strstr(uri, "http://") != uri) {
        fprintf(stderr, "Error: invalid uri!\n");
        exit(0);
    }
    uri += strlen("http://");
    char *c = strstr(uri, ":");
    *c = '\0';
    strcpy(linep->host, uri);
    uri = c + 1;
    c = strstr(uri, "/");
    *c = '\0';
    strcpy(linep->port, uri);
    *c = '/';
    strcpy(linep->path, c);
}

ReqHeader parse_header(char *line) {
    ReqHeader header;
    char *c = strstr(line, ": ");
    if (c == NULL) {
        fprintf(stderr, "Error: invalid header: %s\n", line);
        exit(0);
    }
    *c = '\0';
    strcpy(header.name, line);
    strcpy(header.value, c+2);
    return header;
}
```

解析完后，向服务器发送请求：
```C
int send_to_server(ReqLine *line, ReqHeader *headers, int num_hds) {
    int clientfd;
    char buf[MAXLINE], *buf_head = buf;
    rio_t rio;
    
    clientfd = Open_clientfd(line->host, line->port);
    Rio_readinitb(&rio, clientfd);
    sprintf(buf_head, "GET %s HTTP/1.0\r\n", line->path);
    buf_head = buf + strlen(buf);
    for (int i = 0; i < num_hds; ++i) {
        sprintf(buf_head, "%s: %s", headers[i].name, headers[i].value);
        buf_head = buf + strlen(buf);
    }
    sprintf(buf_head, "\r\n");
    Rio_writen(clientfd, buf, MAXLINE);
    
    return clientfd;
}
```

这样，就实现web代理的基本功能了。实际上，web代理就是同时担任客户端和服务端，然后中间加上个信息的处理就行了。

## Part2: 并发
实现并发有很多方法，比如用多进程、IO多路复用、多线程、预线程化（利用生产者-消费者模型）。我采用的是多线程的方法，因为实现起来比较简单。只需要每次`accept`之后，创建一个新的线程来处理这个连接就行:
```C
void *thread(void *vargp) {
    int connfd = *((int*)vargp);
    Pthread_detach(pthread_self()); //将线程分离，让这个线程计数结束后自己回收资源
    Free(vargp);
    doit(connfd);
    Close(connfd);
    return NULL;
}
```

基本就是将`doit()`函数包在`thread()`里面，然后注意将线程分离出去，避免内存泄露。

## Part3: Cache
在server和client之间加入代理的好处之一，就可以实现cache化。因为，经常有很多对同一个资源多次请求的情况，如果每次都从服务端获取，那样服务器会很累。如果可以在代理部分就实现一个cache，将最近客户端请求过的数据给存储起来，那样就不需要每次都要从服务器请求了，进而提高服务器的效率。

做到这个部分的时候，最先想到的是cache lab里面实现的cache模拟器。所以，可以参考那个思路，先定义cache的数据结构:
```C
typedef struct {
    char *name;
    char *object;
} CacheLine;

typedef struct {
    int used_cnt;
    CacheLine* objects;
} Cache;
```
这个做法实际不是很好，因为不能充分利用内存空间，但是这样比较简单粗暴。这里有很多选择，比如cache可以放在硬盘中，也可以放在内存中。我选择了放在内存中，原因还是实现起来比较简单。可以实现LRU，但是这里没有LRU。原因是因为，我在这里用了读者-写者模型，可以让多个线程同时来读。但是，如果用LRU的话，在每次读之后，都需要更新时间戳，就违背了只读不写的原则。

```C
void init_cache() {
    Sem_init(&mutex, 0, 1);
    Sem_init(&w, 0, 1);
    readcnt = 0;
    cache.objects = (CacheLine*)Malloc(sizeof(CacheLine) * 10);
    cache.used_cnt = 0;
    for (int i = 0; i < 10; ++i) {
        cache.objects[i].name = Malloc(sizeof(char) * MAXLINE);
        cache.objects[i].object = Malloc(sizeof(char) * MAX_OBJECT_SIZE);
    }
}
```
所以，最后确定的方案就是，将1MiB内存分为十块，也就是可以缓存十个web object。然后，采用读者-写者模型。再考虑实现，大概就是每次接受请求并解析后，先去cache里看看有没有对应的web object，如果有就直接返回给客户端，如果没有就只能老老实实向服务端请求，然后再加入到缓存，再返回给客户端了。这个逻辑都在`doit()`函数里面实现了。

还没有提到的就是读者-写者模型:
```C
Cache cache;
int readcnt; //用来记录读者的个数
sem_t mutex, w; //mutex用来给readcnt加锁，w用来给写操作加锁

int reader(int fd, char *uri) {
    int in_cache= 0;
    P(&mutex);
    readcnt++;
    if (readcnt == 1) {
        P(&w);
    }
    V(&mutex);

    for (int i = 0; i < 10; ++i) {
        if (!strcmp(cache.objects[i].name, uri)) {
            Rio_writen(fd, cache.objects[i].object, MAX_OBJECT_SIZE);
            in_cache = 1;
            break;
        }
    }
    P(&mutex);
    readcnt--;
    if (readcnt == 0) {
        V(&w);
    }
    V(&mutex);
    return in_cache;
}

void writer(char *uri, char *buf) {
    P(&w);
    strcpy(cache.objects[cache.used_cnt].name, uri);
    strcpy(cache.objects[cache.used_cnt].object, buf);
    ++cache.used_cnt;
    V(&w);
}
```

这样就完成了！最后：
