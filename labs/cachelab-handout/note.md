# CSAPP Lab -- Cache Lab

在计算机存储器体系中，Cache是CPU和主存之间的这个中间地带，用来弥补CPU和主存速度差异。Cache Lab分为Part A和Part B两个部分。Part A是实现一个Cache模拟器，用来模拟程序运行过程中的cache访问的结果。Part B是通过blocking技术来优化矩阵求逆运算，减少miss次数。

## PART A
这个部分的目标，就是实现一个cache simulator，需要完成`csim.c`这个文件，然后通过`./test-csim`。Lab中已经提供了一个参考程序`./csim-ref`，通过运行这个程序，可以了解到我们应该实现的程序的大概样子。

![Alt text](./1525339896926.png)


输入的是一系列参数(-v -b -E -s -t):
```
• -h: Optional help flag that prints usage info
• -v: Optional verbose flag that displays trace info
• -s <s>: Number of set index bits (S = 2s
is the number of sets)
• -E <E>: Associativity (number of lines per set)
• -b <b>: Number of block bits (B = 2b
is the block size)
• -t <tracefile>: Name of the valgrind trace to replay
```

-t参数给是用来测试的traces，内容格式是这样:
```
I 0400d7d4,8
M 0421c7f0,4
L 04f6b868,8
S 7ff0005c8,8
```
每行都是`[space]operation address,size`的格式。这些traces都是程序运行过程中对cache的访问记录，是使用`valgrind`工具来获取得到的。trace中只有四种操作:
- I 表示load一条指令
- M 表示modify，也就是修改数据(先读取，后写入)
- L 表示load数据，就是读取数据
- S 表示store数据，就是写数据

所以，根据trace中的输入，我们要处理每一条operation，然后给出缓存模拟的结果(miss、hit、eviction)。

因为这个lab并不需要我们真正地读取或者写入数据，只需要返回cache hit或者cache miss等各种情况即可，所以，我们不需要用到
在了解了lab背景后，就可以开始设计程序了。

整个程序的框架： 解析命令行参数 -> 读取trace -> 初始化cache -> 逐行运行operation -> 输出结果。

首先，设计数据结构，cache和operation:
```c
typedef struct argumemts {
    int h; 
    int v; 
    int s; //number of set index bits. S = 2^s
    int E; //number of cache lines per set
    int b; //number of block bits. B = 2^b is the block size.
    char *t; //name of the valgrind trace to replay
} Arguments;

typedef struct cache_line {
    int valid_bit;
    int tag_bit;
    int times;
} CacheLine;

typedef CacheLine* CacheSet;
typedef CacheSet* Cache;

typedef struct operations {
    char op;
    long address;
    int size;
} Operation;
```

对于operation，就根据题目中所给的命令行参数，把参数都装在一个struct中就行了。

然后就是cache的设计，一个cache由若干cache set组成，而每个cache set又由若干cache line组成。实际上，相当于一个二维数组。所以，只需要设计好cache line就行了。

对于cache line，模拟实际cache line的设计，组成部分包括标志位和有效位，但是丢弃了真正存储数据的block（我们只需要实现模拟器，不需要真正存储数据）。 因为cache采用的时LRU的替换原则，所以，当发生冲突的时候，替换掉的应该是缓存组中最长时间没有使用的line。为了实现这个LRU原则，我们给每个cache line加上一个时间戳，用来记录time，为发生eviction的时候的替换作准备。

在设计好了数据结构之后，我们就可以开始实现整个模拟器的逻辑了。

首先解析命令行参数，使用了`getopt.h`这个库:
```c
void get_args(Arguments *args, int argc, char **argv) {
    int opt;
    while (-1 != (opt = getopt(argc, argv, "hvs:E:b:t:"))) {
        switch (opt) {
            case 'h':
                args->h = 1;
                break;
            case 'v':
                args->v = 1;
                break;
            case 's':
                args->s = atoi(optarg);
                break;
            case 'E':
                args->E = atoi(optarg);
                break;
            case 'b':
                args->b = atoi(optarg);
                break;
            case 't':
                args->t = (char*)malloc(sizeof(char)*strlen(optarg));
                strcpy(args->t, optarg);
                break;
            default:
                fprintf(stderr, "wrong argument\n");
                exit(0);
                break;
        }
    }
}
```

解析好了参数，我们就初始化cache：
```c
void init_cache(Cache *cache, Arguments *args) {
    int set_size = 1 << args->s, i, j;
    int num_lines = args->E;

    *cache = (CacheSet*)malloc(sizeof(CacheSet) * set_size);
    for (i = 0; i < set_size; ++i) {
        (*cache)[i] = (CacheLine*)malloc(sizeof(CacheLine) * num_lines);
        for (j = 0; j < num_lines; ++j) {
            (*cache)[i][j].valid_bit = 0;
            (*cache)[i][j].tag_bit = -1;
            (*cache)[i][j].times = 0;
        }
    }
}
```

再就是读取trace：
```c
void parse_traces(Operation *operations, const char *filename) {
    FILE *fp = fopen(filename, "rt");
    char op;
    long address;
    int size = -1, i = 0;

    while (fscanf(fp, "%c %lx,%d\n", &op, &address, &size) != EOF) {
        if (size == -1) {
            continue;
        }
        operations[i].op = op;
        operations[i].size = size;
        operations[i++].address = address;
    }
    fclose(fp);
}
```

到这里，我们的准备工作就完成了，只剩下最核心的部分了。我们遍历所有的operation，依次对于每个operation作出相应的操作。

operation总共有四种类型: I, M, S, L。其中I我们不处理(I时加载指令)，然后S和L实际上模拟的过程是一样的，不管是读还是写，我们都是根据一个地址来访问cache。所以我们统一用一个`process`函数来处理。而M不同，因为modify涉及到读和写，相当于调用两次`process`函数。

去掉关于IO部分的一些代码，遍历operations来处理的部分:
```c
    for (int i = 0; i < num_op; ++i) {
        switch (operations[i].op) {
            case 'I': // not used in this lab.
                break;
            case 'M':
                process(&cache, &args, operations[i].address);
                process(&cache, &args, operations[i].address);
                break;
            case 'L':
                process(&cache, &args, operations[i].address);
                break;
            case 'S':
                process(&cache, &args, operations[i].address);
                break;
            default:
                printf("Error: invalid operation.");
                exit(0);
        }
    }
```

最后，我们来设计process函数。我们可以和回忆cache的运作方式。

对于一个address，在访问cache的时候，它会被这样解析:
![Alt text](./1525339987095.png)
这里的10就是参数中的s，而4就是参数中的b。

所以，我们可以解析address，来分别得到标记位、缓存组索引和块偏移。因为我们不需要进行真正的数据处理，所以不用管块偏移。

然后，根据组索引找到相应的缓存组，遍历这个缓存组，如果由cache line的tagbits和我们地址中的tag bits对上了号，那么就是hit，打印"hit"，结束处理。

如果这一次没有找到，那么就说明我们是miss了，那么就是打印"miss"，然后开始第二次遍历，目的是为了寻找是否有空的cache line，如果有，就修改有关信息，然后结束。如果没有，就说明我们是eviction，打印"eviction"。然后第三次遍历，找到在这里面待了最长时间的cache line，根据LRU替换原则，将其替换掉。

附上process函数代码:
```c
void process(Cache *cache, Arguments *args, int address) {
    address >>= args->b;
    int set_index = 0, i, tag_bits;
    long mask = 0xffffffffffffffff >> (64 - args->s);
    set_index = mask & address;
    address >>= args->s;
    tag_bits = address;

    CacheSet set = (*cache)[set_index];
    for (i = 0; i < args->E; ++i) {
        if (set[i].valid_bit == 1) {
            set[i].times++;
        }
    }

    for (i = 0; i < args->E; ++i) {
        if (set[i].valid_bit == 1 && set[i].tag_bit == tag_bits) {
            printf("hit ");
            set[i].times = 0;
            ++hit_count;
            return;
        }
    }

    ++miss_count;
    printf("miss "); 
    for (i = 0; i < args->E; ++i) {
        if (set[i].valid_bit == 0) {
            set[i].tag_bit = tag_bits;
            set[i].valid_bit = 1;
            set[i].times = 0;
            return;
        }
    }

    ++eviction_count;
    printf("eviction ");

    int max_index = 0, max_time = set[0].times;
    for (i = 0; i < args->E; ++i) {
        if (set[i].times > max_time) {
            max_index = i;
            max_time = set[i].times;
        }
    }
    set[max_index].tag_bit = tag_bits;
    set[max_index].times = 0;
}
```

## PART B
这个部分就是利用分块(blocking)技术来加速矩阵逆转运算，减少cache miss。总共有三个题，分别是32*32、64*64和61*67的矩阵。

### 32*32
通过阅读csapp官网提供的ppt和关于block相关的资料，我们可以了解到分块减少cache miss的大概原理，然后开始尝试。

通过简单测试，可以发现分块size为8的时候，效果是最好的。而且，从理论上来分析，我们可以得到miss次数应该是:(32/8)*(32/8)*(8+8) = 256。但是，实验结果并不是这样，而是三百多次。

为了探究原因，可以使用PART A我们写的模拟器来运行这个lab生成的trace(保存在trace.f0中)。然后，我们修改模拟器加上一些信息，打印出每次操作所选择的cache set。这是前八行：
```
L 30a080,4 缓存组:4 标记位:c28	miss eviction 
S 34a080,4 缓存组:4 标记位:d28	miss eviction 
L 30a084,4 缓存组:4 标记位:c28	miss eviction 
S 34a100,4 缓存组:8 标记位:d28	miss 
L 30a088,4 缓存组:4 标记位:c28	hit 
S 34a180,4 缓存组:c 标记位:d28	miss 
L 30a08c,4 缓存组:4 标记位:c28	hit 
S 34a200,4 缓存组:10 标记位:d28	miss 
```

然后分析这个，我们可以发现，在一些不应该miss的地方，出现了miss，而且都是eviction，也就是发生了冲突。这样，就会在某些地方出现A和B的某个部分不停轮流占据某一个cache line，就造成了很多的miss。

为了解决这个问题，我们可以用临时变量来存储一个行，这样就可以提高效率，每次让A依次读完，而不会去和B抢占一个cahce line。
```c
for (i = 0; i < M; i+=8) {
    for (j = 0; j < N; j += 8) {
        for (k = i; k < i+8; ++k) {
            a = A[k][j];
            b = A[k][j+1];
            c = A[k][j+2];
            d = A[k][j+3];
            e = A[k][j+4];
            f = A[k][j+5];
            g = A[k][j+6];
            h = A[k][j+7];
            B[j][k] = a;
            B[j+1][k] = b;
            B[j+2][k] = c;
            B[j+3][k] = d;
            B[j+4][k] = e;
            B[j+5][k] = f;
            B[j+6][k] = g;
            B[j+7][k] = h;
        }
    }
}  
```
### 64*64
这次发现size为8的分块不再有效，miss达到了四千多次。思考一下，假设A[0][0]的地址为0x0，所以分组为0。然后A[1][0]的地址是64*4*1，，组索引的计算是(address >> 5取最底5位)，所以，组索引应该是0x1000。然后继续: 

A[2][0] -- 64*4*2 >> 5 -- 0x10000
A[3][0] -- 64*4*3 >> 5 -- 0x11000
A[4][0] -- 64*4*4 >> 5 -- 0x100000

到这时候，可以发现A[4][0]的组索引又到了0，所以，如果size为8的分块，会导致组内的冲突。

然后，我们可以尝试更小的分组，size为4的分组，再结合前面使用临时变量的方法，就可以把miss降到可以接受的水平。
```cpp
        for (i = 0; i < M; i+=4) {
            for (j = 0; j < N; j+=4) {
                for (k = i; k < i+4; ++k) {
                    a = A[k][j];
                    b = A[k][j+1];
                    c = A[k][j+2];
                    d = A[k][j+3];
                    B[j][k] = a;
                    B[j+1][k] = b;
                    B[j+2][k] = c;
                    B[j+3][k] = d;
                }
            }
        }  
```
这个解还是不能拿到满分，可以参考其他解决方法。

### 61*67
类似于前面，通过分析可以发现应该使用size为16的分块，然后额外解决没有处理到的那部分就行了。

```cpp
        for (i = 0; i+16 < N; i+=16) {
            for (j = 0; j+16 < M; j+=16) {
                for (k = i; k < i+16; ++k) {
                    a = A[k][j];
                    b = A[k][j+1];
                    c = A[k][j+2];
                    d = A[k][j+3];
                    e = A[k][j+4];
                    f = A[k][j+5];
                    g = A[k][j+6];
                    h = A[k][j+7];
                    B[j][k] = a;
                    B[j+1][k] = b;
                    B[j+2][k] = c;
                    B[j+3][k] = d;
                    B[j+4][k] = e;
                    B[j+5][k] = f;
                    B[j+6][k] = g;
                    B[j+7][k] = h;
                    
                    a = A[k][j+8];
                    b = A[k][j+9];
                    c = A[k][j+10];
                    d = A[k][j+11];
                    e = A[k][j+12];
                    f = A[k][j+13];
                    g = A[k][j+14];
                    h = A[k][j+15];
                    B[j+8][k] = a;
                    B[j+9][k] = b;
                    B[j+10][k] = c;
                    B[j+11][k] = d;
                    B[j+12][k] = e;
                    B[j+13][k] = f;
                    B[j+14][k] = g;
                    B[j+15][k] = h;

                }
            }
        }

        for (k = i; k < N; ++k) {
            for (l = 0; l < M; ++l){
                B[l][k] = A[k][l];
            }
        }
        for (k = 0; k < i; ++k) {
            for (l = j; l < M; ++l) {
                B[l][k] = A[k][l];
            }
        }
```