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

// #define INPUT_PATH     "resources/pretest.txt"
// #define INPUT_PATH     "resources/test_data.txt"
// #define OUTPUT_PATH    "output/result.txt"

#define INPUT_PATH  "/data/test_data.txt"
#define OUTPUT_PATH "/projects/student/result.txt"

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





int CACHE_LINE_ALIGN data[MAX_EDGE][3];
int CACHE_LINE_ALIGN data_num;

int CACHE_LINE_ALIGN data_rev_mapping[MAX_NODE];
int CACHE_LINE_ALIGN node_num = 0;



void read_input() {
    int fd = open(INPUT_PATH, O_RDONLY);
    if (fd == -1) {
        printf("fail to open\n"); fflush(stdout);
        exit(0);
    }
    size_t size = lseek(fd, 0, SEEK_END);
    printf("size: %d\n", (int) size); fflush(stdout);
    char *buffer = (char*)mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    printf("buf: %lld\n", buffer); fflush(stdout);
    

    int x = 0;
    int* local_data = &data[0][0];
    for (int i=0; i<size; ++i) {
        if (unlikely(buffer[i] == ',' || buffer[i] == '\n' || i == size-1)) {
            *local_data = x;
            local_data ++;
            x = 0;
        } else if (buffer[i] >= '0' && buffer[i] <= '9') {
            x = x * 10 + buffer[i] - '0';
        }
    }
    data_num = (local_data - &data[0][0]) / 3;

    printf("over\n"); fflush(stdout);

    std::unordered_map<int, int> hashmap;
    for (int i=0; i<data_num; ++i) {
        x = data[i][0];
        if (hashmap.find(x) == hashmap.end()) {
            hashmap[x] = node_num;
            data_rev_mapping[node_num ++] = x;
        }
        x = data[i][1];
        if (hashmap.find(x) == hashmap.end()) {
            hashmap[x] = node_num;
            data_rev_mapping[node_num ++] = x;
        }
    }

    

    std::sort(data_rev_mapping, data_rev_mapping + node_num);

    

    for (int i=0; i<node_num; ++i) {
        hashmap[data_rev_mapping[i]] = i;
    }


    for (int i=0; i<data_num; ++i) {
        data[i][0] = hashmap[data[i][0]];
        data[i][1] = hashmap[data[i][1]];
    }
    munmap((void *)buffer, size);
    close(fd);
    printf("node num: %d\n", node_num);
}







int useful_edge_num = 0;

void value_filter() {
    int* malloc_all = (int*) malloc(sizeof(int) * node_num * 4);
    memset(malloc_all, -1, sizeof(malloc_all));

    int u, v, x, *u_ptr, *v_ptr;
    for (int i=0; i<data_num; ++i) {
        u = data[i][0]; v = data[i][1]; x = data[i][2];
        u_ptr = malloc_all + (u << 2);
        v_ptr = malloc_all + (v << 2);
        if (u_ptr[2] == -1) {
            u_ptr[2] = u_ptr[3] = x;
        } else {
            u_ptr[2] = std::min(u_ptr[2], x);
            u_ptr[3] = std::max(u_ptr[3], x);
        }

        if (v_ptr[0] == -1) {
            v_ptr[0] = v_ptr[1] = x;
        } else {
            v_ptr[0] = std::min(v_ptr[0], x);
            v_ptr[1] = std::max(v_ptr[1], x);
        }
    }

    for (int i=0; i<data_num; ++i) {
        u = data[i][0]; v = data[i][1]; x = data[i][2];
        u_ptr = malloc_all + (u << 2);
        v_ptr = malloc_all + (v << 2);

        if (5LL * x < u_ptr[0] || 3LL * u_ptr[1] < x || 5LL * v_ptr[3] < x || 3LL * x < v_ptr[2] || u == v) {
            data[i][2] = -1;
            // printf("remove: %d %d\n", data_rev_mapping[u], data_rev_mapping[v]);
        } else {
            useful_edge_num ++;
            // printf("keep: %d %d\n", data_rev_mapping[u], data_rev_mapping[v]);
        }
    }

    free(malloc_all);
}

void filter_edges() {
    value_filter();
}

inline bool check_x_y(int x, int y) {
    return x <= 5LL * y && y <= 3LL * x;
}

inline bool not_check_x_y(int x, int y) {
    return x > 5LL * y || y > 3LL * x;
}

struct Edge {
    int v, x;
    Edge() {}
    Edge(int v, int x): v(v), x(x) {}
};

struct EdgeVec {
    Edge *from;
    int length;
};

bool searchable_nodes[2][MAX_NODE];

Edge *__fwd_edges, *__bck_edges;
EdgeVec *fwd_edges_vec, *bck_edges_vec;
EdgeVec *fwd_two_step_edge_vec;



#define def_fast_edge_edge(tid) \
Edge *__fwd_edge_edge_##tid; \
void* build_edge_edge_##tid(void* args) { \
    __fwd_edge_edge_##tid = (Edge*) malloc(sizeof(Edge) * 10000000); \
    int head, tail, edge_edge_num = 0, index, u, v, x, y, i, j, w; \
    head = tid * node_num / 4; tail = (tid + 1) * node_num / 4; \
    edge_edge_num = 0; \
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
                if (w == u || not_check_x_y(x, y)) continue; \
                __fwd_edge_edge_##tid[edge_edge_num].v = w; \
                __fwd_edge_edge_##tid[edge_edge_num].x = y; \
                edge_edge_num ++; \
                fwd_two_step_edge_vec[index].length ++; \
            } \
        } \
    } \
    return NULL; \
}

def_fast_edge_edge(0)
def_fast_edge_edge(1)
def_fast_edge_edge(2)
def_fast_edge_edge(3)

void build_edges() {
    __fwd_edges = (Edge*) malloc(sizeof(Edge) * useful_edge_num);
    __bck_edges = (Edge*) malloc(sizeof(Edge) * useful_edge_num);
    fwd_edges_vec = (EdgeVec*) malloc(sizeof(EdgeVec) * node_num);
    bck_edges_vec = (EdgeVec*) malloc(sizeof(EdgeVec) * node_num);

    fwd_two_step_edge_vec = (EdgeVec*) malloc(sizeof(EdgeVec) * useful_edge_num);

    memset(fwd_edges_vec, 0, sizeof(EdgeVec) * node_num);
    memset(bck_edges_vec, 0, sizeof(EdgeVec) * node_num);

    int u, v, x;
    for (int i=0; i<data_num; ++i) {
        x = data[i][2];
        if (x == -1) continue;
        u = data[i][0]; v = data[i][1];

        if (v < u) searchable_nodes[0][v] = true;
        if (v > u) searchable_nodes[1][u] = true;

        fwd_edges_vec[u].length ++;
        bck_edges_vec[v].length ++;
    }

    // for (int i=0; i<node_num; ++i) {
    //     printf("[%d]: %d %d\n", data_rev_mapping[i], fwd_edges_vec[i].length, bck_edges_vec[i].length);
    // }

    int fwd_num = 0, bck_num = 0;
    for (int i=0; i<node_num; ++i) {
        searchable_nodes[0][u] &= searchable_nodes[1][u];

        fwd_edges_vec[i].from = __fwd_edges + fwd_num;
        fwd_num += fwd_edges_vec[i].length;
        fwd_edges_vec[i].length = 0;

        bck_edges_vec[i].from = __bck_edges + bck_num;
        bck_num += bck_edges_vec[i].length;
        bck_edges_vec[i].length = 0;
    }

    // build edges
    Edge* edge;
    for (int i=0; i<data_num; ++i) {
        x = data[i][2];
        if (x == -1) continue;
        u = data[i][0]; v = data[i][1];

        edge = fwd_edges_vec[u].from + fwd_edges_vec[u].length ++;
        edge->v = v; edge->x = x;

        edge = bck_edges_vec[v].from + bck_edges_vec[v].length ++;
        edge->v = u; edge->x = x;
    }

    for (u=0; u<node_num; ++u) {
        std::sort(fwd_edges_vec[u].from, fwd_edges_vec[u].from + fwd_edges_vec[u].length,
            [](Edge& a, Edge& b) {
                return a.v < b.v;
            });
        std::sort(bck_edges_vec[u].from, bck_edges_vec[u].from + bck_edges_vec[u].length,
            [](Edge& a, Edge& b) {
                return a.v < b.v;
            });
    }

    pthread_t thr[4];
    pthread_create(thr + 0, NULL, build_edge_edge_0, NULL);
    pthread_create(thr + 1, NULL, build_edge_edge_1, NULL);
    pthread_create(thr + 2, NULL, build_edge_edge_2, NULL);
    pthread_create(thr + 3, NULL, build_edge_edge_3, NULL);
    
    for (int i=0; i<4; ++i) pthread_join(thr[i], NULL);
    // pthread_join(thr[0], NULL);
}

bool topo_useless[MAX_NODE];
int topo_counter[MAX_NODE], topo_queue[MAX_NODE];

void topo_filter() {
    int head = 0, tail = 0;
    for (int i=0; i<node_num; ++i) {
        topo_counter[i] = bck_edges_vec[i].length;
        if (topo_counter[i] == 0) {
            topo_useless[i] = true;
            topo_queue[tail ++] = i;
        }
    }
    int u, v;
    while (head < tail) {
        u = topo_queue[head ++];
        for (int i=0; i<fwd_edges_vec[u].length; ++i) {
            v = (fwd_edges_vec[u].from + i) -> v;
            topo_counter[v] --;
            if (topo_counter[v] == 0) {
                topo_useless[v] = true;
                topo_queue[tail ++] = v;
            }
        }
    }
}

// 每个长度11的char，第一位表示长度
char CACHE_LINE_ALIGN integer_buffer[MAX_NODE][11];
char CACHE_LINE_ALIGN integer_buffer_size[MAX_NODE];
void create_integer_buffer();
inline void deserialize_int(char* buffer, int& buffer_index, int x);
inline void deserialize_id(char* buffer, int& buffer_index, int x);
inline void deserialize_id_slow(char* buffer, int& buffer_index, int x);








#define INVALID_NODE    -1

struct Answer {
    int size, s, a, b, c, d, e, f;
    Answer() {}
    Answer(int size, int s, int a, int b, int c = INVALID_NODE, int d = INVALID_NODE, int e = INVALID_NODE, int f = INVALID_NODE):
        size(size), s(s), a(a), b(b), c(c), d(d), e(e), f(f) {}
};

struct BackTwoStep {
    long long value;
    int e, f, ex, fx;
    BackTwoStep() {}
    BackTwoStep(int e, int f, int ex, int fx):
        e(e), f(f), ex(ex), fx(fx) { value = (1LL << 30) * e + f;}
};

// 可以进一步利用缓存，不适用two_step指针
struct BackStep {
    BackTwoStep *two_step;
    BackStep *nxt, *nxt_far;
    int dx, size;
};

struct Bound {
    int head, tail;
    Bound() {}
    Bound(int head, int tail): head(head), tail(tail) {}
};

#define MILLION    1000000

int global_assign_num = 0;
int global_assign[MAX_NODE];
int total_answer_num = 0;

// header和tailer数组可以合并成一个，缓存

#define def_declr(tid) \
Answer *answer_##tid[5]; \
Bound* answer_bound_##tid[5]; \
int answer_num_##tid[5]; \
int total_answer_num_##tid; \
\
BackTwoStep *bck_two_step_vec_##tid; \
BackStep *bck_step_vec_##tid, **bck_step_header_##tid; \
EdgeVec *edge_vec_##tid, *fwd_two_step_edge_vec_##tid; \
bool *bck_step_visit_##tid; \
int* bck_step_visit_queue_##tid, bck_step_visit_queue_num_##tid; \
\
int fx_min_##tid, fx_max_##tid; \
void malloc_##tid() { \
    bck_two_step_vec_##tid = (BackTwoStep*) \
        malloc(sizeof(BackTwoStep) * MILLION); \
    for (int i=0; i<5; ++i) { \
        answer_##tid[i] = (Answer*) malloc(sizeof(Answer) * 10000000); \
        answer_bound_##tid[i] = (Bound*) malloc(sizeof(Bound) * node_num); \
    } \
    bck_step_vec_##tid = (BackStep*) malloc(sizeof(BackStep) * MILLION); \
    bck_step_header_##tid = (BackStep**) malloc(sizeof(BackStep*) * node_num); \
    bck_step_visit_##tid = (bool*) malloc(sizeof(bool) * node_num); \
    memset(bck_step_visit_##tid, false, sizeof(bool) * node_num); \
    bck_step_visit_queue_##tid = (int*) malloc(sizeof(int) * node_num); \
    bck_step_visit_queue_num_##tid = 0; \
    \
    edge_vec_##tid = (EdgeVec*) malloc(sizeof(EdgeVec) * node_num); \
    memcpy(edge_vec_##tid, fwd_edges_vec, sizeof(EdgeVec) * node_num); \
    \
    fwd_two_step_edge_vec_##tid = (EdgeVec*) malloc(sizeof(EdgeVec) * useful_edge_num); \
    memcpy(fwd_two_step_edge_vec_##tid, fwd_two_step_edge_vec, sizeof(EdgeVec) * useful_edge_num); \
} \
void free_##tid() { \
    free(bck_two_step_vec_##tid); \
    free(bck_step_vec_##tid); \
    free(bck_step_header_##tid); \
    free(bck_step_visit_queue_##tid); \
    free(bck_step_visit_##tid); \
    free(edge_vec_##tid); \
    free(fwd_two_step_edge_vec_##tid); \
}

#define def_bak_search(tid) \
bool do_bck_search_##tid(int starter) { \
    int bck_two_step_num = 0, bck_step_num = 0; \
    fx_min_##tid = 2147483647; fx_max_##tid = 0; \
    int d, e, f, fx, ex, dx, index; \
    Edge *edge_f, *edge_e, *edge_d; \
    edge_f = bck_edges_vec[starter].from + bck_edges_vec[starter].length; \
    while (edge_f > bck_edges_vec[starter].from) { \
        edge_f --; \
        f = edge_f -> v; fx = edge_f -> x; \
        if (f <= starter) break; \
        if (topo_useless[f]) continue; \
        edge_e = bck_edges_vec[f].from + bck_edges_vec[f].length; \
        while (edge_e > bck_edges_vec[f].from) { \
            edge_e --; \
            e = edge_e -> v; ex = edge_e -> x; \
            if (e <= starter) break; \
            if (topo_useless[e]) continue; \
            if (not_check_x_y(ex, fx)) continue; \
            bck_two_step_vec_##tid[bck_two_step_num ++] = {e, f, ex, fx}; \
        } \
    } \
    \
    \
    std::sort(bck_two_step_vec_##tid, bck_two_step_vec_##tid + bck_two_step_num, \
        [](BackTwoStep a, BackTwoStep b) { \
            return a.value < b.value; \
        }); \
    \
    BackTwoStep *step = bck_two_step_vec_##tid + bck_two_step_num; \
    BackStep *bck_header, *bck_step; \
    int size_ef, size_s = integer_buffer_size[starter], size_d; \
    while (step > bck_two_step_vec_##tid) { \
        step --; \
        e = step -> e; ex = step -> ex; \
        f = step -> f; \
        size_ef = integer_buffer_size[e] + integer_buffer_size[f]; \
        \
        edge_d = bck_edges_vec[e].from + bck_edges_vec[e].length; \
        while (edge_d > bck_edges_vec[e].from) { \
            edge_d --; \
            d = edge_d -> v; dx = edge_d -> x; \
            if (d < starter) break; \
            if (topo_useless[d]) continue; \
            if (d == f) continue; \
            if (not_check_x_y(dx, ex)) continue; \
            if (d == starter) { \
                f = step -> f; fx = step -> fx; \
                if (check_x_y(fx, dx)) { \
                    index = answer_num_##tid[0] ++; \
                    answer_##tid[0][index] = {size_ef + size_s + 3, starter, e, f}; \
                } \
            } else { \
                size_d = integer_buffer_size[d]; \
                bck_step = bck_step_vec_##tid + bck_step_num ++; \
                if (bck_step_visit_##tid[d] == false) { \
                    bck_step_visit_##tid[d] = true; \
                    bck_step_visit_queue_##tid[bck_step_visit_queue_num_##tid ++] = d; \
                    bck_step_header_##tid[d] = NULL; \
                    bck_step -> nxt_far = NULL; \
                } else { \
                    bck_step -> nxt_far = bck_step_header_##tid[d] -> nxt_far; \
                    if (step -> e != (bck_step_header_##tid[d] -> two_step) -> e) { \
                        bck_step -> nxt_far = bck_step_header_##tid[d]; \
                    } \
                } \
                bck_step -> two_step = step; \
                bck_step -> nxt = bck_step_header_##tid[d]; \
                bck_step -> dx = dx; \
                bck_step -> size = size_ef + size_d + size_s; \
                bck_step_header_##tid[d] = bck_step; \
                \
                fx = step -> fx; \
                fx_min_##tid = std::min(fx, fx_min_##tid); \
                fx_max_##tid = std::max(fx, fx_max_##tid); \
            } \
        } \
    } \
    return bck_step_num > 0; \
}

#define def_edge_vec_dealer(tid) \
void refresh_edge_vec_##tid(int u, int s) { \
    EdgeVec *vec = edge_vec_##tid + u; \
    while (vec -> length > 0 && vec -> from -> v <= s) { \
        vec -> length --; \
        vec -> from ++; \
    } \
}\
void refresh_edge_edge_vec_##tid(int index, int s) { \
    EdgeVec *vec = fwd_two_step_edge_vec_##tid + index; \
    while (vec -> length > 0 && vec -> from -> v <= s) { \
        vec -> length --; \
        vec -> from ++; \
    } \
}

#define def_fwd_search(tid) \
void do_fwd_search_##tid(int starter) { \
    int a, b, c, d, e, f, sx, ax, bx, cx, dx, fx, index, edge_edge_index; \
    int size_a, size_ab, size_abc, size_bck; \
    int s_num, a_num, b_num, c_num; \
    BackStep * bck_step; \
    \
    Edge *edge_s, *edge_a, *edge_b, *edge_c; \
    refresh_edge_vec_##tid(starter, starter); \
    \
    s_num = edge_vec_##tid[starter].length; \
    edge_s = edge_vec_##tid[starter].from; \
    while (s_num --) { \
        a = edge_s -> v; sx = edge_s -> x; \
        edge_s ++; \
        if (topo_useless[a]) continue; \
        if (fx_min_##tid > 5LL * sx || sx > 3LL * fx_max_##tid) continue; \
        \
        if (bck_step_visit_##tid[a]) { \
            bck_step = bck_step_header_##tid[a]; \
            while (bck_step != NULL) { \
                e = bck_step -> two_step -> e; \
                f = bck_step -> two_step -> f; \
                dx = bck_step -> dx; \
                fx = bck_step -> two_step -> fx; \
                size_bck = bck_step -> size; \
                if (not_check_x_y(fx, sx) || not_check_x_y(sx, dx)){ \
                    bck_step = bck_step -> nxt; \
                    continue; \
                } else { \
                    index = answer_num_##tid[1] ++; \
                    answer_##tid[1][index] = {size_bck + 4, starter, a, e, f}; \
                } \
                bck_step = bck_step -> nxt; \
            } \
        } \
        \
        refresh_edge_vec_##tid(a, starter); \
        a_num = edge_vec_##tid[a].length; \
        edge_a = edge_vec_##tid[a].from; \
        size_a  = integer_buffer_size[a]; \
        while (a_num --) { \
            b = edge_a -> v; ax = edge_a -> x; \
            edge_a ++; \
            if (topo_useless[b]) continue; \
            if (not_check_x_y(sx, ax)) continue; \
            \
            if (bck_step_visit_##tid[b]) { \
                bck_step = bck_step_header_##tid[b]; \
                while (bck_step != NULL) { \
                    e = bck_step -> two_step -> e; \
                    f = bck_step -> two_step -> f; \
                    if (b == f || a == f) { \
                        bck_step = bck_step -> nxt; \
                        continue; \
                    } else if (a == e) { \
                        bck_step = bck_step -> nxt_far; \
                        continue; \
                    } else { \
                        dx = bck_step -> dx; \
                        fx = bck_step -> two_step -> fx; \
                        size_bck = bck_step -> size; \
                        bck_step = bck_step -> nxt; \
                        if (not_check_x_y(fx, sx) || not_check_x_y(ax, dx)) { \
                            continue; \
                        } else { \
                            index = answer_num_##tid[2] ++; \
                            answer_##tid[2][index] = {size_bck + size_a + 5, starter, a, b, e, f}; \
                        } \
                    } \
                } \
            } \
            \
            refresh_edge_vec_##tid(b, starter); \
            b_num = edge_vec_##tid[b].length; \
            edge_b = edge_vec_##tid[b].from; \
            size_ab = size_a + integer_buffer_size[b]; \
            while (b_num --) { \
                c = edge_b -> v; bx = edge_b -> x; \
                edge_edge_index = (edge_b - __fwd_edges); \
                edge_b ++; \
                if (topo_useless[c]) continue; \
                if (c == a) continue; \
                if (not_check_x_y(ax, bx)) continue; \
                \
                if (bck_step_visit_##tid[c]) { \
                    bck_step = bck_step_header_##tid[c]; \
                    while (bck_step != NULL) { \
                        e = bck_step -> two_step -> e; \
                        f = bck_step -> two_step -> f; \
                        if (a == f || b == f || c == f) { \
                            bck_step = bck_step -> nxt; \
                            continue; \
                        } else if (a == e || b == e) { \
                            bck_step = bck_step -> nxt_far; \
                            continue; \
                        } else  { \
                            dx = bck_step -> dx; \
                            fx = bck_step -> two_step -> fx; \
                            size_bck = bck_step -> size; \
                            bck_step = bck_step -> nxt; \
                            if (not_check_x_y(fx, sx) || not_check_x_y(bx, dx)) { \
                                continue; \
                            } else { \
                                index = answer_num_##tid[3] ++; \
                                answer_##tid[3][index] = {size_bck + size_ab + 6, starter, a, b, c, e, f}; \
                            } \
                        } \
                    } \
                } \
                \
                refresh_edge_edge_vec_##tid(edge_edge_index, starter); \
                c_num = fwd_two_step_edge_vec_##tid[edge_edge_index].length; \
                edge_c = fwd_two_step_edge_vec_##tid[edge_edge_index].from; \
                size_abc = size_ab + integer_buffer_size[c]; \
                if (c_num & 1) { \
                    c_num --; \
                    d = edge_c -> v; cx = edge_c -> x; \
                    edge_c ++; \
                    if (!topo_useless[d] && d != a) { \
                    \
                        if (bck_step_visit_##tid[d]) { \
                            bck_step = bck_step_header_##tid[d]; \
                            while (bck_step != NULL) { \
                                e = bck_step -> two_step -> e; \
                                f = bck_step -> two_step -> f; \
                                if (a == f || b == f || c == f || d == f) { \
                                    bck_step = bck_step -> nxt; \
                                    continue; \
                                } else if (a == e || b == e || c == e) { \
                                    bck_step = bck_step -> nxt_far; \
                                    continue; \
                                } else { \
                                    dx = bck_step -> dx; \
                                    fx = bck_step -> two_step -> fx; \
                                    size_bck = bck_step -> size; \
                                    bck_step = bck_step -> nxt; \
                                    if (not_check_x_y(fx, sx) || not_check_x_y(cx, dx)) { \
                                        continue; \
                                    } else { \
                                        index = answer_num_##tid[4] ++; \
                                        answer_##tid[4][index] = {size_bck + size_abc + 7, starter, a, b, c, d, e, f}; \
                                    } \
                                } \
                            } \
                        } \
                    } \
                } \
                while (c_num > 0) { \
                    c_num -= 2; \
                    d = edge_c -> v; cx = edge_c -> x; \
                    edge_c ++; \
                    if (!topo_useless[d] && d != a) { \
                        \
                        if (bck_step_visit_##tid[d]) { \
                            bck_step = bck_step_header_##tid[d]; \
                            while (bck_step != NULL) { \
                                e = bck_step -> two_step -> e; \
                                f = bck_step -> two_step -> f; \
                                if (a == f || b == f || c == f || d == f) { \
                                    bck_step = bck_step -> nxt; \
                                    continue; \
                                } else if (a == e || b == e || c == e) { \
                                    bck_step = bck_step -> nxt_far; \
                                    continue; \
                                } else { \
                                    dx = bck_step -> dx; \
                                    fx = bck_step -> two_step -> fx; \
                                    size_bck = bck_step -> size; \
                                    bck_step = bck_step -> nxt; \
                                    if (not_check_x_y(fx, sx) || not_check_x_y(cx, dx)) { \
                                        continue; \
                                    } else { \
                                        index = answer_num_##tid[4] ++; \
                                        answer_##tid[4][index] = {size_bck + size_abc + 7, starter, a, b, c, d, e, f}; \
                                    } \
                                } \
                            } \
                        } \
                    } \
                    d = edge_c -> v; cx = edge_c -> x; \
                    edge_c ++; \
                    if (!topo_useless[d] && d != a) { \
                        \
                        if (bck_step_visit_##tid[d]) { \
                            bck_step = bck_step_header_##tid[d]; \
                            while (bck_step != NULL) { \
                                e = bck_step -> two_step -> e; \
                                f = bck_step -> two_step -> f; \
                                if (a == f || b == f || c == f || d == f) { \
                                    bck_step = bck_step -> nxt; \
                                    continue; \
                                } else if (a == e || b == e || c == e) { \
                                    bck_step = bck_step -> nxt_far; \
                                    continue; \
                                } else { \
                                    dx = bck_step -> dx; \
                                    fx = bck_step -> two_step -> fx; \
                                    size_bck = bck_step -> size; \
                                    bck_step = bck_step -> nxt; \
                                    if (not_check_x_y(fx, sx) || not_check_x_y(cx, dx)) { \
                                        continue; \
                                    } else { \
                                        index = answer_num_##tid[4] ++; \
                                        answer_##tid[4][index] = {size_bck + size_abc + 7, starter, a, b, c, d, e, f}; \
                                    } \
                                } \
                            } \
                        } \
                    } \
                } \
            } \
        } \
    } \
}

#define def_do_search(tid) \
void do_search_##tid(int starter) { \
    for (int i=0; i<5; ++i) { \
        answer_bound_##tid[i][starter].head = answer_num_##tid[i]; \
    } \
    \
    if (do_bck_search_##tid(starter)) { \
        do_fwd_search_##tid(starter); \
        while (bck_step_visit_queue_num_##tid > 0) { \
            bck_step_visit_##tid[bck_step_visit_queue_##tid[-- bck_step_visit_queue_num_##tid]] = false; \
        } \
    } \
    \
    for (int i=0; i<5; ++i) { \
        answer_bound_##tid[i][starter].tail = answer_num_##tid[i]; \
    } \
    std::reverse(answer_##tid[0] + answer_bound_##tid[0][starter].head, \
        answer_##tid[0] + answer_bound_##tid[0][starter].tail); \
    return; \
}


#define def_search_thr(tid) \
void* search_##tid(void* args) { \
    malloc_##tid(); \
    int u; \
    while (true) { \
        u = __sync_fetch_and_add(&global_assign_num, 1); \
        if (u >= node_num) break; \
        global_assign[u] = -1; \
        if (!searchable_nodes[0][u] || topo_useless[u]) continue; \
        global_assign[u] = tid; \
        do_search_##tid(u); \
    } \
    for (int i=0; i<5; ++i) { \
        total_answer_num_##tid += answer_num_##tid[i]; \
    } \
    __sync_fetch_and_add(&total_answer_num, total_answer_num_##tid); \
    free_##tid(); \
    return NULL; \
}



#define def_search_body(tid) \
def_declr(tid) \
def_bak_search(tid) \
def_edge_vec_dealer(tid) \
def_fwd_search(tid) \
def_do_search(tid) \
def_search_thr(tid)


def_search_body(0)
def_search_body(1)
def_search_body(2)
def_search_body(3)




#define add_common buffer[buffer_index ++] = ',';
#define add_break  buffer[buffer_index ++] = '\n';


Answer* answer_all;
int process_answer_from[5], process_buffer_from[5];



char total_answer_num_buffer[100];
int total_answer_num_buffer_num = 0;

int merge_answer() {
    int now = 0, u, head, tail, total_size = 0, temp_size, div_size, t_s;
    Answer* local_answer;

    answer_all = (Answer*) malloc(sizeof(Answer) * total_answer_num);

#define def_merge_answer_case(aid, tid) \
        case tid: \
            local_answer = answer_##tid[aid]; \
            head = answer_bound_##tid[aid][u].head; \
            tail = answer_bound_##tid[aid][u].tail; \
            break;


#define def_merge_answer_for(aid) \
    for (u=0; u<node_num; ++ u) { \
        if (global_assign[u] == -1) continue; \
        switch (global_assign[u]) \
        { \
        def_merge_answer_case(aid, 0) \
        def_merge_answer_case(aid, 1) \
        def_merge_answer_case(aid, 2) \
        def_merge_answer_case(aid, 3) \
        default: \
            break; \
        } \
        memcpy(answer_all + now , local_answer + head, (tail - head) * sizeof(Answer)); \
        now += tail - head; \
    }

    def_merge_answer_for(0)
    def_merge_answer_for(1)
    def_merge_answer_for(2)
    def_merge_answer_for(3)
    def_merge_answer_for(4)

    printf("after merge now: %d\n", now);

    for (int i=0; i<total_answer_num; ++i) {
        total_size += answer_all[i].size;
    }

    deserialize_int(total_answer_num_buffer, total_answer_num_buffer_num, total_answer_num);
    total_answer_num_buffer[total_answer_num_buffer_num ++] = '\n';
    
    total_size += total_answer_num_buffer_num;

    div_size = total_size / 4;

    temp_size = total_answer_num_buffer_num; 
    now = 0; 
    t_s = total_answer_num_buffer_num;

    for (int i=0; i<total_answer_num; ++i) {
        temp_size += answer_all[i].size;
        t_s += answer_all[i].size;
        if (temp_size >= div_size) {
            process_answer_from[++ now] = i+1;
            process_buffer_from[now] = t_s;
            temp_size = 0;
            if (now == 3) break;
        }
    }
    process_answer_from[4] = total_answer_num;

    
    // printf("total size: %d\n", total_size);
    // for (int i=0; i<5; ++i) {
    //     printf("process: %d %d %d\n", i, process_answer_from[i], process_buffer_from[i]);
    // }
    return total_size;
}



int total_answer_buffer_size;

void deserialize_answer(char* buffer, int& buffer_index, Answer& ans) {
    deserialize_id(buffer, buffer_index, ans.s);
    add_common
    deserialize_id(buffer, buffer_index, ans.a);
    add_common
    deserialize_id(buffer, buffer_index, ans.b);
    if (ans.c != INVALID_NODE) {
        add_common
        deserialize_id(buffer, buffer_index, ans.c);
        if (ans.d != INVALID_NODE) {
            add_common
            deserialize_id(buffer, buffer_index, ans.d);
            if (ans.e != INVALID_NODE) {
                add_common
                deserialize_id(buffer, buffer_index, ans.e);
                if (ans.f != INVALID_NODE) {
                    add_common
                    deserialize_id(buffer, buffer_index, ans.f);
                }
            }
        }
    }
    add_break
}

void deserialize_answer_slow(char* buffer, int& buffer_index, Answer& ans) {
    deserialize_id_slow(buffer, buffer_index, ans.s);
    add_common
    deserialize_id_slow(buffer, buffer_index, ans.a);
    add_common
    deserialize_id_slow(buffer, buffer_index, ans.b);
    if (ans.c != INVALID_NODE) {
        add_common
        deserialize_id_slow(buffer, buffer_index, ans.c);
        if (ans.d != INVALID_NODE) {
            add_common
            deserialize_id_slow(buffer, buffer_index, ans.d);
            if (ans.e != INVALID_NODE) {
                add_common
                deserialize_id_slow(buffer, buffer_index, ans.e);
                if (ans.f != INVALID_NODE) {
                    add_common
                    deserialize_id_slow(buffer, buffer_index, ans.f);
                }
            }
        }
    }
    add_break
}

void do_write_to_disk(int id) {
    // printf("into process: %d\n", id);
    int u, now, mod, head, tail;

    int fd = open(OUTPUT_PATH, O_RDWR, 0666);
    char* ans = (char*)mmap(NULL, total_answer_buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    now = process_buffer_from[id];
    // printf("before: %d %d\n", id, now);
    if (id == 0) {
        memcpy(ans, total_answer_num_buffer, total_answer_num_buffer_num);
        now += total_answer_num_buffer_num;
    }
    head = process_answer_from[id]; tail = process_answer_from[id+1];
    mod = (tail - head) % 10;

#define def_write_to_disk_case(id) \
    case id: \
        deserialize_answer(ans, now, answer_all[head]); \
        head ++;

    switch (mod)
    {
    def_write_to_disk_case(9)
    def_write_to_disk_case(8)
    def_write_to_disk_case(7)
    def_write_to_disk_case(6)
    def_write_to_disk_case(5)
    def_write_to_disk_case(4)
    def_write_to_disk_case(3)
    def_write_to_disk_case(2)
    def_write_to_disk_case(1)    
    default:
        break;
    }
    while (head < tail - 10) {
        deserialize_answer(ans, now, answer_all[head]);
        deserialize_answer(ans, now, answer_all[head + 1]);
        deserialize_answer(ans, now, answer_all[head + 2]);
        deserialize_answer(ans, now, answer_all[head + 3]);
        deserialize_answer(ans, now, answer_all[head + 4]);
        deserialize_answer(ans, now, answer_all[head + 5]);
        deserialize_answer(ans, now, answer_all[head + 6]);
        deserialize_answer(ans, now, answer_all[head + 7]);
        deserialize_answer(ans, now, answer_all[head + 8]);
        deserialize_answer(ans, now, answer_all[head + 9]);
        head += 10;
    }
    deserialize_answer_slow(ans, now, answer_all[head]);
    deserialize_answer_slow(ans, now, answer_all[head + 1]);
    deserialize_answer_slow(ans, now, answer_all[head + 2]);
    deserialize_answer_slow(ans, now, answer_all[head + 3]);
    deserialize_answer_slow(ans, now, answer_all[head + 4]);
    deserialize_answer_slow(ans, now, answer_all[head + 5]);
    deserialize_answer_slow(ans, now, answer_all[head + 6]);
    deserialize_answer_slow(ans, now, answer_all[head + 7]);
    deserialize_answer_slow(ans, now, answer_all[head + 8]);
    deserialize_answer_slow(ans, now, answer_all[head + 9]);

    // printf("now: %d %d\n", id, now);

    munmap(ans, total_answer_buffer_size);
}

void do_write() {
    total_answer_buffer_size = merge_answer();
    printf("total ans: %d\n", total_answer_buffer_size);


    int writer_fd = open(OUTPUT_PATH, O_RDWR | O_CREAT , 0666);
    if (writer_fd == -1) {
        exit(0);
    }
    ftruncate(writer_fd, total_answer_buffer_size);
    close(writer_fd); 
    
    // for (int i=0; i<4; ++i) {
    //     do_write_to_disk(i);
    // }

    int id;
    pid_t pid;
    for (int i=1; i<5; ++i) {
        id = i;
        pid = fork();
        if (pid <= 0) break;
    }

    if (pid == -1) {
        printf("fork failed\n");
        exit(0);
    }

    if (pid == 0) {
        do_write_to_disk(id-1);
    } else {
        // do_write_to_disk(0);
        exit(0);
    }
}

int main() {
    read_input();

    filter_edges();

    return 0;

    printf("after read\n"); fflush(stdout);

    printf("after filter\n"); fflush(stdout);
    
    build_edges();

    printf("after build\n"); fflush(stdout);

    topo_filter();

    create_integer_buffer(); 

    // return 0;

    pthread_t search_thr[4];
    pthread_create(search_thr + 0, NULL, search_0, NULL);
    pthread_create(search_thr + 1, NULL, search_1, NULL);
    pthread_create(search_thr + 2, NULL, search_2, NULL);
    pthread_create(search_thr + 3, NULL, search_3, NULL);

    for (int i=0; i<4; ++i) pthread_join(search_thr[i], NULL);

    // return 0;

    printf("total answer: %d\n", total_answer_num);

    printf("after search\n"); fflush(stdout);

    do_write();

}
































void create_integer_buffer() {
    int x, i=0, l, r;
    if (data_rev_mapping[0] == 0) {
        integer_buffer_size[0] = integer_buffer[0][0] = 1;
        integer_buffer[0][1] = '0';
        i ++;
    }
    for (; i<node_num; ++i) {
        x = data_rev_mapping[i];
        integer_buffer_size[i] = 1;
        while (unlikely(x)) {
            integer_buffer[i][integer_buffer_size[i] ++] = (x % 10) + '0';
            x /= 10;
        }
        std::reverse(integer_buffer[i]+1, integer_buffer[i] + integer_buffer_size[i]);
        integer_buffer[i][0] = -- integer_buffer_size[i];
    }
}

inline void deserialize_int(char* buffer, int& buffer_index, int x) {
    if (x == 0) {
        buffer[buffer_index ++] = '0';
        return;
    }
    int original_index = buffer_index;
    while (x) {
        buffer[buffer_index ++] = (x % 10) + '0';
        x /= 10;
    }
    for (int i=original_index, j=buffer_index-1; i<j; ++i, --j) {
        original_index = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = original_index;
    }
}


inline void deserialize_id(char* buffer, int& buffer_index, int x) {
    fast_memcpy(buffer + buffer_index, integer_buffer[x] + 1, integer_buffer[x][0] * sizeof(char));
    buffer_index += integer_buffer[x][0];
}

inline void deserialize_id_slow(char* buffer, int& buffer_index, int x) {
    memcpy(buffer + buffer_index, integer_buffer[x] + 1, integer_buffer[x][0] * sizeof(char));
    buffer_index += integer_buffer[x][0];
}