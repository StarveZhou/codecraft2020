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



struct Edge {
    int v, x;
    Edge() {}
    Edge(int v, int x): v(v), x(x) {}
};

struct EdgeVec {
    Edge *from;
    int length;
};


Edge *__fwd_edges, *__bck_edges;
EdgeVec *fwd_edges_vec, *bck_edges_vec;

void build_edges() {
    __fwd_edges = (Edge*) malloc(sizeof(Edge) * useful_edge_num);
    __bck_edges = (Edge*) malloc(sizeof(Edge) * useful_edge_num);
    fwd_edges_vec = (EdgeVec*) malloc(sizeof(EdgeVec) * node_num);
    bck_edges_vec = (EdgeVec*) malloc(sizeof(EdgeVec) * node_num);

    memset(fwd_edges_vec, 0, sizeof(EdgeVec) * node_num);
    memset(bck_edges_vec, 0, sizeof(EdgeVec) * node_num);

    int u, v, x;
    for (int i=0; i<data_num; ++i) {
        x = data[i][2];
        if (x == -1) continue;
        u = data[i][0]; v = data[i][1]; 
        fwd_edges_vec[u].length ++;
        bck_edges_vec[v].length ++;
    }

    // for (int i=0; i<node_num; ++i) {
    //     printf("[%d]: %d %d\n", data_rev_mapping[i], fwd_edges_vec[i].length, bck_edges_vec[i].length);
    // }

    int fwd_num = 0, bck_num = 0;
    for (int i=0; i<node_num; ++i) {
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

    // printf("edges: \n");
    // for(u=0; u<node_num; ++u) {
    //     printf("[%d] %d: ", data_rev_mapping[u], fwd_edges_vec[u].length);
    //     for (int i=0; i<fwd_edges_vec[u].length; ++i) {
    //         printf("%d ", data_rev_mapping[fwd_edges_vec[u].from[i].v]);
    //     }
    //     printf("\n");
    // }

    // printf("rev edges: \n");
    // for (u=0; u<node_num; ++u) {
    //     printf("[%d] %d: ", data_rev_mapping[u], bck_edges_vec[u].length);
    //     for (int i=0; i<bck_edges_vec[u].length; ++i) {
    //         printf("%d ", data_rev_mapping[bck_edges_vec[u].from[i].v]);
    //     }
    //     printf("\n");
    // }
}



inline bool check_x_y(int x, int y) {
    return x <= 5LL * y && y <= 3LL * x;
}

struct Answer {
    int s, a, b, c, d, e, f;
    Answer() {}
    Answer(int s, int a, int b, int c = -1, int d = -1, int e = -1, int f = -1):
        s(s), a(a), b(b), c(c), d(d), e(e), f(f) {}
};

struct BackTwoStep {
    int e, f, ex, fx;
    BackTwoStep() {}
    BackTwoStep(int e, int f, int ex, int fx):
        e(e), f(f), ex(ex), fx(fx) {}
};

// 可以进一步利用缓存，不适用two_step指针
struct BackStep {
    BackTwoStep *two_step;
    BackStep* nxt;
    int dx;
    BackStep() {}
    BackStep(BackTwoStep* two_step, BackStep* nxt, int dx):
        two_step(two_step), nxt(nxt), dx(dx) {}
};

#define MILLION    1000000

int global_assign_num = 0;
int global_assign[MAX_NODE];
int total_answer_num = 0;

// header和tailer数组可以合并成一个，缓存

#define def_declr(tid) \
Answer *answer_##tid[5]; \
int *answer_header_##tid[5], *answer_tailer_##tid[5]; \
int answer_num_##tid[5]; \
int total_answer_num_##tid; \
\
BackTwoStep *bck_two_step_vec_##tid; \
BackStep *bck_step_vec_##tid, **bck_step_header_##tid; \
EdgeVec *edge_vec_##tid; \
int *bck_step_visit_##tid; \
\
int fx_min_##tid, fx_max_##tid; \
void malloc_##tid() { \
    bck_two_step_vec_##tid = (BackTwoStep*) \
        malloc(sizeof(BackTwoStep) * MILLION); \
    for (int i=0; i<5; ++i) { \
        answer_##tid[i] = (Answer*) malloc(sizeof(Answer) * 10000000); \
        answer_header_##tid[i] = (int*) malloc(sizeof(int) * node_num); \
        answer_tailer_##tid[i] = (int*) malloc(sizeof(int) * node_num); \
    } \
    bck_step_vec_##tid = (BackStep*) malloc(sizeof(BackStep) * MILLION); \
    bck_step_header_##tid = (BackStep**) malloc(sizeof(BackStep*) * node_num); \
    bck_step_visit_##tid = (int*) malloc(sizeof(int) * node_num); \
    memset(bck_step_visit_##tid, -1, sizeof(int) * node_num); \
    \
    edge_vec_##tid = (EdgeVec*) malloc(sizeof(EdgeVec) * node_num); \
    memcpy(edge_vec_##tid, fwd_edges_vec, sizeof(EdgeVec) * node_num); \
} \
void free_##tid() { \
    free(bck_two_step_vec_##tid); \
    free(bck_step_vec_##tid); \
    free(bck_step_header_##tid); \
}

#define def_free_answer(tid) \
void free_answer_##tid() { \
    for (int i=0; i<5; ++i) { \
        free(answer_##tid[i]); \
        free(answer_header_##tid[i]); \
        free(answer_tailer_##tid[i]); \
    } \
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
        edge_e = bck_edges_vec[f].from + bck_edges_vec[f].length; \
        while (edge_e > bck_edges_vec[f].from) { \
            edge_e --; \
            e = edge_e -> v; ex = edge_e -> x; \
            if (e <= starter) break; \
            if (!check_x_y(ex, fx)) continue; \
            bck_two_step_vec_##tid[bck_two_step_num ++] = {e, f, ex, fx}; \
        } \
    } \
    \
    \
    std::sort(bck_two_step_vec_##tid, bck_two_step_vec_##tid + bck_two_step_num, \
        [](BackTwoStep a, BackTwoStep b) { \
            return a.e != b.e ? a.e < b.e : a.f < b.f; \
        }); \
    \
    BackTwoStep *step = bck_two_step_vec_##tid + bck_two_step_num; \
    BackStep *bck_header, *bck_step; \
    while (step > bck_two_step_vec_##tid) { \
        step --; \
        e = step -> e; ex = step -> ex; \
        f = step -> f; \
        edge_d = bck_edges_vec[e].from + bck_edges_vec[e].length; \
        while (edge_d > bck_edges_vec[e].from) { \
            edge_d --; \
            d = edge_d -> v; dx = edge_d -> x; \
            if (d < starter) break; \
            if (d == f) continue; \
            if (!check_x_y(dx, ex)) continue; \
            if (d == starter) { \
                f = step -> f; fx = step -> fx; \
                if (check_x_y(fx, dx)) { \
                    index = answer_num_##tid[0] ++; \
                    answer_##tid[0][index] = {starter, e, f}; \
                } \
            } else { \
                if (bck_step_visit_##tid[d] != starter) { \
                    bck_step_visit_##tid[d] = starter; \
                    bck_step_header_##tid[d] = NULL; \
                } \
                bck_step = bck_step_vec_##tid + bck_step_num ++; \
                bck_step -> two_step = step; \
                bck_step -> nxt = bck_step_header_##tid[d]; \
                bck_step -> dx = dx; \
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
}

#define def_fwd_search(tid) \
void do_fwd_search_##tid(int starter) { \
    int a, b, c, d, e, f, sx, ax, bx, cx, dx, fx, index; \
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
        if (fx_min_##tid > 5LL * sx || sx > 3LL * fx_max_##tid) continue; \
        \
        if (bck_step_visit_##tid[a] == starter) { \
            bck_step = bck_step_header_##tid[a]; \
            while (bck_step != NULL) { \
                e = bck_step -> two_step -> e; \
                f = bck_step -> two_step -> f; \
                dx = bck_step -> dx; \
                fx = bck_step -> two_step -> fx; \
                if (check_x_y(fx, sx) && check_x_y(sx, dx)) { \
                    index = answer_num_##tid[1] ++; \
                    answer_##tid[1][index] = {starter, a, e, f}; \
                } \
                bck_step = bck_step -> nxt; \
            } \
        } \
        \
        refresh_edge_vec_##tid(a, starter); \
        a_num = edge_vec_##tid[a].length; \
        edge_a = edge_vec_##tid[a].from; \
        while (a_num --) { \
            b = edge_a -> v; ax = edge_a -> x; \
            edge_a ++; \
            if (!check_x_y(sx, ax)) continue; \
            \
            if (bck_step_visit_##tid[b] == starter) { \
                bck_step = bck_step_header_##tid[b]; \
                while (bck_step != NULL) { \
                    e = bck_step -> two_step -> e; \
                    f = bck_step -> two_step -> f; \
                    if (b != f && a != e && a != f) { \
                        dx = bck_step -> dx; \
                        fx = bck_step -> two_step -> fx; \
                        if (check_x_y(fx, sx) && check_x_y(ax, dx)) { \
                            index = answer_num_##tid[2] ++; \
                            answer_##tid[2][index] = {starter, a, b, e, f}; \
                        } \
                    } \
                    bck_step = bck_step -> nxt; \
                } \
            } \
            \
            refresh_edge_vec_##tid(b, starter); \
            b_num = edge_vec_##tid[b].length; \
            edge_b = edge_vec_##tid[b].from; \
            while (b_num --) { \
                c = edge_b -> v; bx = edge_b -> x; \
                edge_b ++; \
                if (c == a) continue; \
                if (!check_x_y(ax, bx)) continue; \
                \
                if (bck_step_visit_##tid[c] == starter) { \
                    bck_step = bck_step_header_##tid[c]; \
                    while (bck_step != NULL) { \
                        e = bck_step -> two_step -> e; \
                        f = bck_step -> two_step -> f; \
                        if (a != e && a != f && b != e && b != f && c != f) { \
                            dx = bck_step -> dx; \
                            fx = bck_step -> two_step -> fx; \
                            if (check_x_y(fx, sx) && check_x_y(bx, dx)) { \
                                index = answer_num_##tid[3] ++; \
                                answer_##tid[3][index] = {starter, a, b, c, e, f}; \
                            } \
                        } \
                        bck_step = bck_step -> nxt; \
                    } \
                } \
                \
                refresh_edge_vec_##tid(c, starter); \
                c_num = edge_vec_##tid[c].length; \
                edge_c = edge_vec_##tid[c].from; \
                while (c_num --) { \
                    d = edge_c -> v; cx = edge_c -> x; \
                    edge_c ++; \
                    if (d == a || d == b) continue; \
                    if (!check_x_y(bx, cx)) continue; \
                    \
                    if (bck_step_visit_##tid[d] == starter) { \
                        bck_step = bck_step_header_##tid[d]; \
                        while (bck_step != NULL) { \
                            e = bck_step -> two_step -> e; \
                            f = bck_step -> two_step -> f; \
                            if (a != e && a != f && b != e && b != f && c != e && c != f && d != f) { \
                                dx = bck_step -> dx; \
                                fx = bck_step -> two_step -> fx; \
                                if (check_x_y(fx, sx) && check_x_y(cx, dx)) { \
                                    index = answer_num_##tid[4] ++; \
                                    answer_##tid[4][index] = {starter, a, b, c, d, e, f}; \
                                } \
                            } \
                            bck_step = bck_step -> nxt; \
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
        answer_header_##tid[i][starter] = answer_num_##tid[i]; \
    } \
    \
    if (false) {printf("starter: %d %d\n", tid, starter); fflush(stdout);} \
    if (do_bck_search_##tid(starter)) { \
        if (false) {printf("finish bck %d\n", starter); fflush(stdout);} \
        do_fwd_search_##tid(starter); \
        if (false) {printf("finish fwd %d\n", starter); fflush(stdout);} \
    } \
    \
    for (int i=0; i<5; ++i) { \
        answer_tailer_##tid[i][starter] = answer_num_##tid[i]; \
    } \
    return; \
}


#define def_search_thr(tid) \
void* search_##tid(void* args) { \
    malloc_##tid(); \
    int u; \
    while (true) { \
        u = __sync_fetch_and_add(&global_assign_num, 1); \
        if (u >= node_num) break; \
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
def_search_body(4)
def_search_body(5)


def_free_answer(0)
def_free_answer(1)
def_free_answer(2)
def_free_answer(3)
def_free_answer(4)
def_free_answer(5)


char CACHE_LINE_ALIGN integer_buffer[MAX_NODE][10];
int CACHE_LINE_ALIGN integer_buffer_size[MAX_NODE];
void create_integer_buffer();
inline void deserialize_int(char* buffer, int& buffer_index, int x);
inline void deserialize_id(char* buffer, int& buffer_index, int x);

#define add_common buffer[buffer_index ++] = ',';
#define add_break  buffer[buffer_index ++] = '\n';

void deserialize_answer(char* buffer, int& buffer_index, Answer* answer, int id) {
    deserialize_id(buffer, buffer_index, answer -> s);
    add_common
    deserialize_id(buffer, buffer_index, answer -> a);
    add_common
    deserialize_id(buffer, buffer_index, answer -> b);
    int* nxt = &(answer -> c);
    switch (id)
    {
    case 4:
        add_common
        deserialize_id(buffer, buffer_index, *nxt);
        nxt ++;
    case 3:
        add_common
        deserialize_id(buffer, buffer_index, *nxt);
        nxt ++;
    case 2:
        add_common
        deserialize_id(buffer, buffer_index, *nxt);
        nxt ++;    
    case 1:
        add_common
        deserialize_id(buffer, buffer_index, *nxt);
        break; 
    default:
        break;
    }
    add_break
}

void deserialize(char* buffer, int& buffer_index, int id, int from, int to) {
#define def_case(tid) \
    case tid: \
        local_answer = answer_##tid[id]; \
        header = answer_header_##tid[id][u]; \
        tailer = answer_tailer_##tid[id][u]; \
        break;

    Answer *local_answer;
    int header, tailer;
    for (int u=from; u<to; ++u) {
        switch (global_assign[u])
        {
        def_case(0) 
        def_case(1) 
        def_case(2) 
        def_case(3) 
        def_case(4) 
        def_case(5) 
        default:
            break;
        }
        if (id == 0)
            for (int i=tailer-1; i>=header; --i) {
                deserialize_answer(buffer, buffer_index, local_answer + i, id);
            }
        else
            for (int i=header; i<tailer; ++i) {
                deserialize_answer(buffer, buffer_index, local_answer + i, id);
            }
    }
}

#define def_write_declar(tid) \
char* write_buffer_##tid; \
int write_buffer_num_##tid;

def_write_declar(0)
void* write_thr_0(void* args) {
    write_buffer_0 = (char*) malloc(sizeof(char) * total_answer_num * ONE_INT_CHAR_SIZE * 6);
    deserialize_int(write_buffer_0, write_buffer_num_0, total_answer_num);
    write_buffer_0[write_buffer_num_0 ++] = '\n';
    deserialize(write_buffer_0, write_buffer_num_0, 0, 0, node_num);
    deserialize(write_buffer_0, write_buffer_num_0, 1, 0, node_num);
    deserialize(write_buffer_0, write_buffer_num_0, 2, 0, node_num);
    deserialize(write_buffer_0, write_buffer_num_0, 3, 0, node_num);
    return NULL;
}


def_write_declar(1) 
void* write_thr_1(void* args) { 
    write_buffer_1 = (char*) malloc(sizeof(char) * 300000000); 
    deserialize(write_buffer_1, write_buffer_num_1, 4, 0, node_num / 30); 
    return NULL; 
}

def_write_declar(2) 
void* write_thr_2(void* args) { 
    write_buffer_2 = (char*) malloc(sizeof(char) * 300000000); 
    deserialize(write_buffer_2, write_buffer_num_2, 4, node_num / 30, node_num / 15); 
    return NULL; 
}

def_write_declar(3) 
void* write_thr_3(void* args) { 
    write_buffer_3 = (char*) malloc(sizeof(char) * 300000000); 
    deserialize(write_buffer_3, write_buffer_num_3, 4, node_num / 15, node_num / 9); 
    return NULL; 
}

def_write_declar(4) 
void* write_thr_4(void* args) { 
    write_buffer_4 = (char*) malloc(sizeof(char) * 300000000); 
    deserialize(write_buffer_4, write_buffer_num_4, 4, node_num / 9, node_num / 5); 
    return NULL; 
}

def_write_declar(5) 
void* write_thr_5(void* args) { 
    write_buffer_5 = (char*) malloc(sizeof(char) * 300000000); 
    deserialize(write_buffer_5, write_buffer_num_5, 4, node_num / 5, node_num); 
    return NULL; 
}

int write_from[6];
char* disk_buf;

#define def_write_to_disk(tid) \
void* write_to_disk_##tid(void* args) { \
    int starter = write_from[tid]; \
    memcpy(disk_buf + starter, write_buffer_##tid, write_buffer_num_##tid); \
    return NULL; \
} \

def_write_to_disk(0)
def_write_to_disk(1)
def_write_to_disk(2)
def_write_to_disk(3)
def_write_to_disk(4)
def_write_to_disk(5)

extern int errno;

void do_write() {
    

    printf("into do write\n"); fflush(stdout); 

    pthread_t write_thr[6];
    pthread_create(write_thr + 0, NULL, write_thr_0, NULL);
    pthread_create(write_thr + 1, NULL, write_thr_1, NULL);
    pthread_create(write_thr + 2, NULL, write_thr_2, NULL);
    pthread_create(write_thr + 3, NULL, write_thr_3, NULL);
    pthread_create(write_thr + 4, NULL, write_thr_4, NULL);
    pthread_create(write_thr + 5, NULL, write_thr_5, NULL);

    for (int i=0; i<6; ++i) pthread_join(write_thr[i], NULL);

    printf("0 %d\n", write_buffer_num_0);
    printf("1 %d\n", write_buffer_num_1);
    printf("2 %d\n", write_buffer_num_2);
    printf("3 %d\n", write_buffer_num_3);
    printf("4 %d\n", write_buffer_num_4);
    printf("5 %d\n", write_buffer_num_5);

    int writer_fd = open(OUTPUT_PATH, O_RDWR | O_CREAT , 0666);
    if (writer_fd == -1) {
        exit(0);
    }


    write_from[0] = 0;
    write_from[1] = write_buffer_num_0;
    write_from[2] = write_buffer_num_1;
    write_from[3] = write_buffer_num_2;
    write_from[4] = write_buffer_num_3;
    write_from[5] = write_buffer_num_4;

    for (int i=2; i<6; ++i) write_from[i] += write_from[i-1];

    int write_len = write_from[5] + write_buffer_num_5;

    printf("write len: %d\n", write_len); 

    ftruncate(writer_fd, write_len);    
    disk_buf = (char *)mmap(NULL, write_len, PROT_READ | PROT_WRITE, MAP_SHARED, writer_fd, 0);


    char* mesg = strerror(errno);
    printf("%s\n", mesg);
    
    printf("disk_buf: %lld %d\n", disk_buf, writer_fd);

    pthread_t disk_thr[6];

    pthread_create(disk_thr + 0, NULL, write_to_disk_0, NULL);
    pthread_create(disk_thr + 1, NULL, write_to_disk_1, NULL);
    pthread_create(disk_thr + 2, NULL, write_to_disk_2, NULL);
    pthread_create(disk_thr + 3, NULL, write_to_disk_3, NULL);
    pthread_create(disk_thr + 4, NULL, write_to_disk_4, NULL);
    pthread_create(disk_thr + 5, NULL, write_to_disk_5, NULL);

    for (int i=0; i<6; ++i) pthread_join(disk_thr[i], NULL);

    munmap((void *) disk_buf, write_len);
    // write(writer_fd, "\n", 1);
    close(writer_fd);
}

int main() {
    read_input();

    printf("after read\n"); fflush(stdout);

    filter_edges();

    printf("after filter\n"); fflush(stdout);
    
    build_edges();

    printf("after build\n"); fflush(stdout);

    pthread_t search_thr[6];
    pthread_create(search_thr + 0, NULL, search_0, NULL);
    pthread_create(search_thr + 1, NULL, search_1, NULL);
    pthread_create(search_thr + 2, NULL, search_2, NULL);
    pthread_create(search_thr + 3, NULL, search_3, NULL);
    pthread_create(search_thr + 4, NULL, search_4, NULL);
    pthread_create(search_thr + 5, NULL, search_5, NULL);

    create_integer_buffer(); 

    for (int i=0; i<6; ++i) pthread_join(search_thr[i], NULL);
    // pthread_join(search_thr[0], NULL);

    printf("total answer: %d\n", total_answer_num);

    printf("after search\n"); fflush(stdout);

    do_write();

}
































void create_integer_buffer() {
    int x, i=0, l, r;
    if (data_rev_mapping[0] == 0) {
        integer_buffer_size[0] = 1;
        integer_buffer[0][0] = '0';
        i ++;
    }
    for (; i<node_num; ++i) {
        x = data_rev_mapping[i];
        while (unlikely(x)) {
            integer_buffer[i][integer_buffer_size[i] ++] = (x % 10) + '0';
            x /= 10;
        }
        for (l=0,r=integer_buffer_size[i]-1; l<r; ++l, --r) {
            x = integer_buffer[i][l];
            integer_buffer[i][l] = integer_buffer[i][r];
            integer_buffer[i][r] = x;
        }
    }
}

inline void deserialize_int(char* buffer, int& buffer_index, int x) {
    if (x == 0) {
        buffer[buffer_index ++] = '0';
        return;
    }
    int orignial_index = buffer_index;
    while (x) {
        buffer[buffer_index ++] = (x % 10) + '0';
        x /= 10;
    }
    for (int i=orignial_index, j=buffer_index-1; i<j; ++i, --j) {
        orignial_index = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = orignial_index;
    }
}


inline void deserialize_id(char* buffer, int& buffer_index, int x) {
    memcpy(buffer + buffer_index, integer_buffer[x], integer_buffer_size[x] * sizeof(char));
    buffer_index += integer_buffer_size[x];
}