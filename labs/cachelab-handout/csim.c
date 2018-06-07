#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <assert.h>

// [space]operation address, size
// I -- instruction load
// L -- data load
// S -- data store
// M -- data modify

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

//result
int hit_count = 0;
int miss_count = 0;
int eviction_count = 0;

/*-------------------------------------------------------------------*/

void init_args(Arguments *args) {
    args->h = 0;
    args->v = 0;
    args->s = -1;
    args->E = -1;
    args->b = -1;
    args->t = NULL;
}

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
                printf("wrong argument\n");
                exit(0);
                break;
        }
    }
}

void process_help(Arguments args) {
    if (args.h == 1 || args.b == -1 || args.E == -1 
    || args.s == -1 || args.t == NULL) {
        FILE *fp = fopen("help.txt", "rt");
        char c;
        while (fscanf(fp, "%c", &c) != EOF) {
            putchar(c);
        }
        fclose(fp);
        exit(0);
    }
}

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

int num_lines(const char *filename) {
    char c;
    int num_lines = 0;
    FILE *fp = fopen(filename, "rt");
    while ((fscanf(fp, "%c", &c)) != EOF) {
        if (c == '\n') {
            ++num_lines;
        }
    }
    fclose(fp);
    return num_lines;
}

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

// 0 -- hit
// 1 -- miss
// 2 -- eviction
void process(Cache *cache, Arguments *args, int address) {
    address >>= args->b;
    int set_index = 0, i, tag_bits;
    long mask = 0xffffffffffffffff >> (64 - args->s);
    set_index = mask & address;
    address >>= args->s;
    tag_bits = address;
    printf("缓存组:%x 标记位:%x\t", set_index, tag_bits);

    CacheSet set = (*cache)[set_index];
    for (i = 0; i < args->E; ++i) {
        if (set[i].valid_bit == 1) {
            set[i].times++;
        }
    }

    for (i = 0; i < args->E; ++i) {
        //printf("Line %d\tvalid_bits:%d\ttag:%d\n", i, line.valid_bit, line.tag_bit);
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


int main(int argc, char **argv)
{
    Arguments args;
    init_args(&args);
    get_args(&args, argc, argv);
    process_help(args);

    Cache cache;
    init_cache(&cache, &args);

    int num_op = num_lines(args.t);
    Operation *operations = (Operation*)malloc(sizeof(Operation) * num_op);    
    parse_traces(operations, args.t);

    for (int i = 0; i < num_op; ++i) {
        switch (operations[i].op) {
            case 'I': // not used in this lab.
                break;
            case 'M':
                printf("M %lx,%d ", operations[i].address, operations[i].size);
                process(&cache, &args, operations[i].address);
                process(&cache, &args, operations[i].address);
                printf("\n");
                break;
            case 'L':
                printf("L %lx,%d ", operations[i].address, operations[i].size);
                process(&cache, &args, operations[i].address);
                printf("\n");
                break;
            case 'S':
                printf("S %lx,%d ", operations[i].address, operations[i].size);
                process(&cache, &args, operations[i].address);
                printf("\n");
                break;
            default:
                printf("Error: invalid operation.");
                exit(0);
        }
    }
    printSummary(hit_count, miss_count, eviction_count);

    for (int i = 0; i < (1 << args.s); ++i) {
        free(cache[i]);
    }
    free(cache);
    return 0;
}
