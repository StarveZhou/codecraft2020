// 去掉循环展开，保留hash和sort，但是sort之后不用hash映射结果
// merge read filter build
// 新增逗号绑定到buffer上
// 多线程build
// 基数排序，不去重
// 新版sort
// back2 放在 back3里面
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>

#include <pthread.h>
#include <string.h>


#include <algorithm>
#include <unordered_map>

#include <arm_neon.h>

//------------------ MACRO ---------------------

#include <errno.h>
#include <stdio.h>

#define INPUT_PATH     "resources/new_pretest.txt"
// #define INPUT_PATH     "resources/new_test_data.txt"
#define OUTPUT_PATH    "output/result.txt"

// #define INPUT_PATH  "/data/test_data.txt"
// #define OUTPUT_PATH "/projects/student/result.txt"

#define MAX_NODE               4000000
#define MAX_EDGE               2000000
#define MAX_ANSW               20000000
#define MAX_FOR_EACH_LINE      34
#define ONE_INT_CHAR_SIZE      11
#define MAX_3_ANSW             2000000
#define MAX_4_ANSW             5000000
#define PATH_PAR_2             5000000
#define PATH_PAR_3             10000000
#define PATH_PAR_4             20000000


#define CACHE_LINE_ALIGN       __attribute__((aligned(64)))
#define likely(x)             __builtin_expect((x),1)
#define unlikely(x)           __builtin_expect((x),0)


// ***************************************************************


void* fast_memcpy(void* dst, void* src, size_t size) {
    unsigned long *s = (unsigned long *) src;
    unsigned long *d = (unsigned long *) dst;
    vst1q_u64(&d[0], vld1q_u64(&s[0]));
    // if (size > 16) {
    //     vst1q_u64(&d[2], vld1q_u64(&s[2]));
    // }
    return dst;
}

// 每个长度11的char，第一位表示长度
char CACHE_LINE_ALIGN integer_buffer[MAX_NODE][12];
char CACHE_LINE_ALIGN integer_buffer_size[MAX_NODE];
void create_integer_buffer_0();
void create_integer_buffer_i(int i);
inline void deserialize_int(char* buffer, int& buffer_index, int x);
inline void deserialize_id(char* buffer, int& buffer_index, int x);
inline void deserialize_id_slow(char* buffer, int& buffer_index, int x);

struct Edge {
    int v;
    long long x;
    Edge() {}
    Edge(int v, long long x): v(v), x(x) {}
};

struct EdgeVec {
    Edge *from;
    int length;
};

bool searchable_nodes[2][MAX_NODE];

Edge *__fwd_edges, *__bck_edges;
EdgeVec *fwd_edges_vec, *bck_edges_vec;
EdgeVec *fwd_two_step_edge_vec;

pthread_barrier_t fast_edge_edge_barrier;

#define def_fast_edge_edge(tid) \
Edge *__fwd_edge_edge_##tid; \
void* build_edge_edge_##tid(void* args) { \
    __fwd_edge_edge_##tid = (Edge*) malloc(sizeof(Edge) * 10000000); \
    int head, tail, edge_edge_num = 0, index, u, v, i, j, w; \
    long long x, y; \
    head = tid * node_num / 4; tail = (tid + 1) * node_num / 4; \
    edge_edge_num = 0; \
    if (head == 0) { \
        u = head ++; \
        std::sort(fwd_edges_vec[u].from, fwd_edges_vec[u].from + fwd_edges_vec[u].length, \
            [](Edge& a, Edge& b) { \
                return a.v < b.v; \
            }); \
        std::sort(bck_edges_vec[u].from, bck_edges_vec[u].from + bck_edges_vec[u].length, \
            [](Edge& a, Edge& b) { \
                return a.v < b.v; \
            }); \
        create_integer_buffer_0(); \
    } \
    for (u=head; u<tail; ++u) { \
        std::sort(fwd_edges_vec[u].from, fwd_edges_vec[u].from + fwd_edges_vec[u].length, \
            [](Edge& a, Edge& b) { \
                return a.v < b.v; \
            }); \
    } \
    for (u=head; u<tail; ++u) { \
        std::sort(bck_edges_vec[u].from, bck_edges_vec[u].from + bck_edges_vec[u].length, \
            [](Edge& a, Edge& b) { \
                return a.v < b.v; \
            }); \
    } \
    for (u=head; u<tail; ++u) { \
        create_integer_buffer_i(u); \
    } \
    pthread_barrier_wait(&fast_edge_edge_barrier); \
    for (u=head; u<tail; ++u) { \
        for (i=0; i<fwd_edges_vec[u].length; ++i) { \
            v = (fwd_edges_vec[u].from + i) -> v; \
            x = (fwd_edges_vec[u].from + i) -> x; \
            index = (fwd_edges_vec[u].from - __fwd_edges) + i; \
            fwd_two_step_edge_vec[index].from = __fwd_edge_edge_##tid + edge_edge_num; \
            fwd_two_step_edge_vec[index].length = 0; \
            \
            for (j=0; j<fwd_edges_vec[v].length; ++j) { \
                w = (fwd_edges_vec[v].from + j) -> v; \
                y = (fwd_edges_vec[v].from + j) -> x; \
                if (w == u || x > 5LL * y || y > 3LL * x) continue; \
                __fwd_edge_edge_##tid[edge_edge_num].v = w; \
                __fwd_edge_edge_##tid[edge_edge_num].x = y; \
                edge_edge_num ++; \
                fwd_two_step_edge_vec[index].length ++; \
            } \
        } \
    } \
    return NULL; \
}




int CACHE_LINE_ALIGN data[MAX_EDGE][2];
long long data_value[MAX_EDGE];
int CACHE_LINE_ALIGN data_num;

int CACHE_LINE_ALIGN data_rev_mapping[MAX_NODE];
int CACHE_LINE_ALIGN node_num = 0;
int useful_edge_num = 0;

int data_assign_mapper[MAX_NODE];

struct Pair {
    int x, ptr;
    Pair() {}
    Pair(int x, int ptr): x(x), ptr(ptr) {}
};

Pair pair_vec[MAX_NODE];

def_fast_edge_edge(0)
def_fast_edge_edge(1)
def_fast_edge_edge(2)
def_fast_edge_edge(3)

int radix_counter0[256], radix_counter8[256], radix_counter16[256], radix_counter24[256];
int radix_from[256];
int *radix_index;
int *radix_index_a, *radix_index_b;

void do_hash() {
    int i, x, u, v, mask, size, mod;
    size = data_num << 1; mod = size % 8;
    radix_index = (int*) malloc(sizeof(int) * size);
    radix_index_a = (int*) malloc(sizeof(int) * size);
    radix_index_b = (int*) malloc(sizeof(int) * size);
    
    for (i=0; i<data_num; ++i) {
        u = data[i][0]; v = data[i][1];
        radix_index[i<<1] = u; radix_index[i<<1|1] = v;
        radix_index_a[i<<1] = i<<1; radix_index_a[i<<1|1] = (i<<1|1);
        radix_counter0[u&255] ++; radix_counter0[v&255] ++;
        radix_counter8[u>>8&255] ++; radix_counter8[v>>8&255] ++;
        radix_counter16[u>>16&255] ++; radix_counter16[v>>16&255] ++;
        radix_counter24[u>>24&255] ++; radix_counter24[v>>24&255] ++;
    }


    // 0
    radix_from[0] = 0;
    for (i=1; i<256; ++i) radix_from[i] = radix_from[i-1] + radix_counter0[i-1];
    i = 0;
    switch (mod)
    {
    case 6:
        radix_index_b[radix_from[radix_index[radix_index_a[i]]&255] ++] = radix_index_a[i]; ++ i;
        radix_index_b[radix_from[radix_index[radix_index_a[i]]&255] ++] = radix_index_a[i]; ++ i;
    case 4:
        radix_index_b[radix_from[radix_index[radix_index_a[i]]&255] ++] = radix_index_a[i]; ++ i;
        radix_index_b[radix_from[radix_index[radix_index_a[i]]&255] ++] = radix_index_a[i]; ++ i;
        i += 2;
    case 2:
        radix_index_b[radix_from[radix_index[radix_index_a[i]]&255] ++] = radix_index_a[i]; ++ i;
        radix_index_b[radix_from[radix_index[radix_index_a[i]]&255] ++] = radix_index_a[i]; ++ i;
        i += 2;
    default:
        break;
    }
    while (i < size) {
        radix_index_b[radix_from[radix_index[radix_index_a[i]]&255] ++] = radix_index_a[i]; ++ i;
        radix_index_b[radix_from[radix_index[radix_index_a[i]]&255] ++] = radix_index_a[i]; ++ i;
        radix_index_b[radix_from[radix_index[radix_index_a[i]]&255] ++] = radix_index_a[i]; ++ i;
        radix_index_b[radix_from[radix_index[radix_index_a[i]]&255] ++] = radix_index_a[i]; ++ i;
        radix_index_b[radix_from[radix_index[radix_index_a[i]]&255] ++] = radix_index_a[i]; ++ i;
        radix_index_b[radix_from[radix_index[radix_index_a[i]]&255] ++] = radix_index_a[i]; ++ i;
        radix_index_b[radix_from[radix_index[radix_index_a[i]]&255] ++] = radix_index_a[i]; ++ i;
        radix_index_b[radix_from[radix_index[radix_index_a[i]]&255] ++] = radix_index_a[i]; ++ i;
    }

    // 8
    radix_from[0] = 0;
    for (i=1; i<256; ++i) radix_from[i] = radix_from[i-1] + radix_counter8[i-1];
    i = 0;
    switch (mod)
    {
    case 6:
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>8&255] ++] = radix_index_b[i]; ++ i;
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>8&255] ++] = radix_index_b[i]; ++ i;
    case 4:
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>8&255] ++] = radix_index_b[i]; ++ i;
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>8&255] ++] = radix_index_b[i]; ++ i;
    case 2:
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>8&255] ++] = radix_index_b[i]; ++ i;
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>8&255] ++] = radix_index_b[i]; ++ i;
    default:
        break;
    }
    while (i < size) {
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>8&255] ++] = radix_index_b[i]; ++ i;
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>8&255] ++] = radix_index_b[i]; ++ i;
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>8&255] ++] = radix_index_b[i]; ++ i;
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>8&255] ++] = radix_index_b[i]; ++ i;
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>8&255] ++] = radix_index_b[i]; ++ i;
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>8&255] ++] = radix_index_b[i]; ++ i;
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>8&255] ++] = radix_index_b[i]; ++ i;
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>8&255] ++] = radix_index_b[i]; ++ i;
    }

    // 16
    radix_from[0] = 0;
    for (i=1; i<256; ++i) radix_from[i] = radix_from[i-1] + radix_counter16[i-1];
    i = 0;
    switch (mod)
    {
    case 6:
        radix_index_b[radix_from[radix_index[radix_index_a[i]]>>16&255] ++] = radix_index_a[i]; ++ i;
        radix_index_b[radix_from[radix_index[radix_index_a[i]]>>16&255] ++] = radix_index_a[i]; ++ i;
    case 4:
        radix_index_b[radix_from[radix_index[radix_index_a[i]]>>16&255] ++] = radix_index_a[i]; ++ i;
        radix_index_b[radix_from[radix_index[radix_index_a[i]]>>16&255] ++] = radix_index_a[i]; ++ i;
    case 2:
        radix_index_b[radix_from[radix_index[radix_index_a[i]]>>16&255] ++] = radix_index_a[i]; ++ i;
        radix_index_b[radix_from[radix_index[radix_index_a[i]]>>16&255] ++] = radix_index_a[i]; ++ i;
    default:
        break;
    }
    while (i < size) {
        radix_index_b[radix_from[radix_index[radix_index_a[i]]>>16&255] ++] = radix_index_a[i]; ++ i;
        radix_index_b[radix_from[radix_index[radix_index_a[i]]>>16&255] ++] = radix_index_a[i]; ++ i;
        radix_index_b[radix_from[radix_index[radix_index_a[i]]>>16&255] ++] = radix_index_a[i]; ++ i;
        radix_index_b[radix_from[radix_index[radix_index_a[i]]>>16&255] ++] = radix_index_a[i]; ++ i;
        radix_index_b[radix_from[radix_index[radix_index_a[i]]>>16&255] ++] = radix_index_a[i]; ++ i;
        radix_index_b[radix_from[radix_index[radix_index_a[i]]>>16&255] ++] = radix_index_a[i]; ++ i;
        radix_index_b[radix_from[radix_index[radix_index_a[i]]>>16&255] ++] = radix_index_a[i]; ++ i;
        radix_index_b[radix_from[radix_index[radix_index_a[i]]>>16&255] ++] = radix_index_a[i]; ++ i;
    }

    // 24
    radix_from[0] = 0;
    for (i=1; i<256; ++i) radix_from[i] = radix_from[i-1] + radix_counter24[i-1];
    i = 0;
    switch (mod)
    {
    case 6:
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>24&255] ++] = radix_index_b[i]; ++ i;
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>24&255] ++] = radix_index_b[i]; ++ i;
    case 4:
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>24&255] ++] = radix_index_b[i]; ++ i;
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>24&255] ++] = radix_index_b[i]; ++ i;
    case 2:
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>24&255] ++] = radix_index_b[i]; ++ i;
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>24&255] ++] = radix_index_b[i]; ++ i;
    default:
        break;
    }
    while (i < size) {
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>24&255] ++] = radix_index_b[i]; ++ i;
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>24&255] ++] = radix_index_b[i]; ++ i;
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>24&255] ++] = radix_index_b[i]; ++ i;
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>24&255] ++] = radix_index_b[i]; ++ i;
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>24&255] ++] = radix_index_b[i]; ++ i;
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>24&255] ++] = radix_index_b[i]; ++ i;
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>24&255] ++] = radix_index_b[i]; ++ i;
        radix_index_a[radix_from[radix_index[radix_index_b[i]]>>24&255] ++] = radix_index_b[i]; ++ i;
    }

    // rehash
    data[radix_index_a[0]>>1][radix_index_a[0]&1] = node_num;
    data_rev_mapping[node_num] = radix_index[radix_index_a[0]];
    for(i=1; i<size; ++i) {
        if (radix_index[radix_index_a[i]] != radix_index[radix_index_a[i-1]]) {
            node_num ++;
            data_rev_mapping[node_num] = radix_index[radix_index_a[i]];
        }
        data[radix_index_a[i]>>1][radix_index_a[i]&1] = node_num;      
    }
    node_num ++;

    printf("node_num: %d\n", node_num); fflush(stdout);

    free(radix_index_a);
    free(radix_index_b);
    free(radix_index);
}

void read_input() {
    int fd = open(INPUT_PATH, O_RDONLY);
    if (fd == -1) {
        printf("fail to open\n"); fflush(stdout);
        exit(0);
    }
    size_t size = lseek(fd, 0, SEEK_END);
    char *buffer = (char*)mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    

    int x = 0, i=0, mod, index = 0;
    int u, v;
    long long *u_ptr, *v_ptr;
    long long xx;


    i = 0; data_num = 0;
    bool flag;
    int dot_size;
    while (i < size-1) {
        x = 0;
        while (true) {
            if (buffer[i] == ',') {
                break;
            } else {
                x = x * 10 + buffer[i] - '0';
            }
            ++ i;
        }
        ++ i;
        data[data_num][0] = x;
        x = 0;
        while (true) {
            if (buffer[i] == ',') {
                break;
            } else {
                x = x * 10 + buffer[i] - '0';
            }
            ++ i;
        }
        ++ i;
        data[data_num][1] = x;
        xx = 0; flag = false;
        while (true) {
            if (i == size || buffer[i] == '\n') {
                break;
            } else if (buffer[i] == '.') {
                flag = true;
                break;
            } else {
                xx = xx * 10 + buffer[i] - '0';
            }
            ++ i;
        }
        ++ i;
        xx *= 100;
        if (flag) {
            x = 0; dot_size = 0;
            while (true) {
                if (i == size || buffer[i] == '\n') {
                    break;
                } else if (buffer[i] >= '0' && buffer[i] <= '9') {
                    x = x * 10 + buffer[i] - '0';
                    dot_size ++;
                }
                ++ i;
            }
            ++ i;
            if (dot_size == 1) xx += x * 10;
            if (dot_size == 2) xx += x;
        }
        data_value[data_num] = xx;
        data_num ++;
    }
    munmap((void *)buffer, size);
    close(fd);
    

    // printf("data_num: %d\n", data_num);
    // for (int i=0; i<data_num; ++i) {
    //     printf("%d %d %lld\n", data[i][0], data[i][1], data_value[i]);
    // }
    // printf("\n");
    // hash

    do_hash();

    // return;

    // hash & filter value

    long long* malloc_all = (long long*) malloc(sizeof(long long) * node_num * 4);
    memset(malloc_all, -1, sizeof(long long) * node_num * 4);

    for (i=0; i<data_num; ++i) {
        u = data[i][0];
        v = data[i][1];
        xx = data_value[i];

        u_ptr = malloc_all + (u << 2);
        v_ptr = malloc_all + (v << 2);
        if (u_ptr[2] == -1) {
            u_ptr[2] = u_ptr[3] = xx;
        } else {
            u_ptr[2] = std::min(u_ptr[2], xx);
            u_ptr[3] = std::max(u_ptr[3], xx);
        }

        if (v_ptr[0] == -1) {
            v_ptr[0] = v_ptr[1] = xx;
        } else {
            v_ptr[0] = std::min(v_ptr[0], xx);
            v_ptr[1] = std::max(v_ptr[1], xx);
        }
    }

    

    // filter value & build edge
    __fwd_edges = (Edge*) malloc(sizeof(Edge) * data_num);
    __bck_edges = (Edge*) malloc(sizeof(Edge) * data_num);
    fwd_edges_vec = (EdgeVec*) malloc(sizeof(EdgeVec) * node_num);
    bck_edges_vec = (EdgeVec*) malloc(sizeof(EdgeVec) * node_num);

    fwd_two_step_edge_vec = (EdgeVec*) malloc(sizeof(EdgeVec) * data_num);

    memset(fwd_edges_vec, 0, sizeof(EdgeVec) * node_num);
    memset(bck_edges_vec, 0, sizeof(EdgeVec) * node_num);

    for (i=0; i<data_num; ++i) {
        u = data[i][0]; v = data[i][1]; xx = data_value[i];
        u_ptr = malloc_all + (u << 2);
        v_ptr = malloc_all + (v << 2);

        if (5LL * xx < u_ptr[0] || 3LL * u_ptr[1] < xx || 5LL * v_ptr[3] < xx || 3LL * xx < v_ptr[2]) {
            data_value[i] = -1;
        } else {
            useful_edge_num ++;
            if (v < u) searchable_nodes[0][v] = true;
            if (v > u) searchable_nodes[1][u] = true;

            fwd_edges_vec[u].length ++;
            bck_edges_vec[v].length ++;
        }
    }

    // printf("useful edge: %d\n", useful_edge_num);

    free(malloc_all);

    // build edge
    int fwd_num = 0, bck_num = 0;
    for (i=0; i<node_num; ++i) {
        fwd_edges_vec[i].from = __fwd_edges + fwd_num;
        fwd_num += fwd_edges_vec[i].length;
        fwd_edges_vec[i].length = 0;

        bck_edges_vec[i].from = __bck_edges + bck_num;
        bck_num += bck_edges_vec[i].length;
        bck_edges_vec[i].length = 0;
    }

    // build edges
    Edge* edge;
    for (i=0; i<data_num; ++i) {
        xx = data_value[i];
        if (x == -1) continue;
        u = data[i][0]; v = data[i][1];

        edge = fwd_edges_vec[u].from + fwd_edges_vec[u].length ++;
        edge->v = v; edge->x = x;

        edge = bck_edges_vec[v].from + bck_edges_vec[v].length ++;
        edge->v = u; edge->x = x;
    }

    pthread_barrier_init(&fast_edge_edge_barrier, NULL, 4);

    pthread_t thr[4];
    pthread_create(thr + 0, NULL, build_edge_edge_0, NULL);
    pthread_create(thr + 1, NULL, build_edge_edge_1, NULL);
    pthread_create(thr + 2, NULL, build_edge_edge_2, NULL);
    pthread_create(thr + 3, NULL, build_edge_edge_3, NULL);
    
    for (int i=0; i<4; ++i) pthread_join(thr[i], NULL);

    printf("finish read\n"); fflush(stdout);
}