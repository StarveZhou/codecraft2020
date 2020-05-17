#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <string.h>

#include <pthread.h>
#include <algorithm>

// ------------------ TEST ---------------------

#include <time.h>
#include <stdio.h>
#include <string>
clock_t start_time, end_time;
void record_start_time() { start_time = clock(); }
void record_end_time(std::string str) {
    end_time = clock();
    printf(str.c_str());
    printf(" : %d clock\n", end_time - start_time);
    fflush(stdout);
}



// ----------------- MACRO --------------------

// #define INPUT_PATH    "resources/pretest.txt"
#define INPUT_PATH    "resources/test_data.txt"
#define OUTPUT_PATH   "output/result.txt"

#define MAX_NODE              400000
#define MAX_EDGE              200000
#define MAX_ANSW              200000
#define MAX_FOR_EACH_LINE     34

#define MAX_BCK_1             400000
#define MAX_BCK_2             2000000
#define MAX_BCK_3             4000000

// **************************************************************
#define C_HASH_MAP_SIZE    (1 << 16)
const int hash_map_mask = (1 << 16) - 1;
struct c_hash_map_t {
    int counter = 1;
    int header[C_HASH_MAP_SIZE];
    int key[MAX_NODE + 10], value[MAX_NODE + 10], ptr[MAX_NODE + 10];
};
typedef c_hash_map_t* c_hash_map;
int c_hash_map_size();
int c_hash_map_size(c_hash_map hash);
void c_hash_map_insert(c_hash_map hash, int key, int value);
void c_hash_map_replace(c_hash_map hash, int key, int value);
int c_hash_map_get(c_hash_map hash, int key);
// ***************************************************************

// ----------------- FUNC ---------------------

char input_buffer[MAX_EDGE * MAX_FOR_EACH_LINE];
int input_buffer_size;

void read_input_file() {
    int fd = open(INPUT_PATH, O_RDONLY);
    if (fd == -1) {
        printf("open read only file failed\n");
        exit(0);
    }

    input_buffer_size = read(fd, input_buffer, MAX_EDGE * MAX_FOR_EACH_LINE);
    close(fd);
}

#define EACH_EDGE_NUM       800000

int reader_deser_header[8];

#define def_reader_de(tid) \
int input_data_##tid[EACH_EDGE_NUM][3]; \
int input_data_num_##tid; \
int input_data_sorted_##tid[EACH_EDGE_NUM << 1]; \
int input_data_sorted_unique_num_##tid; \
void* reader_deser_##tid(void* args) { \
    int from = ((int*)args)[0]; \
    int to = ((int*)args)[1]; \
    int* data_now = &input_data_##tid[0][0]; \
    int x = 0; \
    while (from < to) { \
        if (input_buffer[from] == ',' || input_buffer[from] == '\n') { \
            *data_now = x; \
            data_now ++; \
            x = 0; \
        } else if (input_buffer[from] >= '0' && input_buffer[from] <= '9') { \
            x = x * 10 + input_buffer[from] - '0'; \
        } \
        from ++; \
    } \
    input_data_num_##tid = (data_now - &input_data_##tid[0][0]) / 3; \
    int sorted_num = 0; \
    for (int i=0; i<input_data_num_##tid; ++i) { \
        input_data_sorted_##tid[sorted_num ++] = input_data_##tid[i][0]; \
        input_data_sorted_##tid[sorted_num ++] = input_data_##tid[i][1]; \
    } \
    std::sort(input_data_sorted_##tid, input_data_sorted_##tid + sorted_num); \
    int* tail_ptr = std::unique(input_data_sorted_##tid, input_data_sorted_##tid + sorted_num); \
    input_data_sorted_unique_num_##tid = tail_ptr - input_data_sorted_##tid; \
    return NULL; \
}

def_reader_de(0)
def_reader_de(1)
def_reader_de(2)
def_reader_de(3)
def_reader_de(4)
def_reader_de(5)




void merge_array(int* dst, int &dst_num, int* src0, int src0_num, int* src1, int src1_num) {
    int x = 0, y = 0, flag;
    dst_num = 0;
    while (x < src0_num || y < src1_num) {
        if (y == src1_num) flag = 0;
        else if (x == src0_num) flag = 1;
        else flag = src0[x] >= src1[y];

        if (flag == 0) {
            if (dst_num > 0 && src0[x] == dst[dst_num-1]) {
                x ++;
                continue;
            }
            dst[dst_num ++] = src0[x ++];
        } else {
            if (dst_num > 0 && src1[y] == dst[y-1]) {
                y ++;
                continue;
            }
            dst[dst_num ++] = src1[y ++];
        }
    }
}

c_hash_map_t mapping_t; c_hash_map mapping = &mapping_t;
int node_num = 0;
int sorted_vertex[2][MAX_NODE];
int rev_data_mapping[MAX_NODE];

void hash_vertex() {

// #define def_printf_sorted_num(tid) \
//     printf("where: %d\n", input_data_sorted_unique_num_##tid); fflush(stdout);

    int length = 0;
    // todo multi thread
    merge_array(sorted_vertex[0], length, input_data_sorted_0, input_data_sorted_unique_num_0, 
        input_data_sorted_1, input_data_sorted_unique_num_1);
    merge_array(sorted_vertex[1], length, input_data_sorted_2, input_data_sorted_unique_num_2,
        sorted_vertex[0], length);
    merge_array(sorted_vertex[0], length, input_data_sorted_3, input_data_sorted_unique_num_3,
        sorted_vertex[1], length);
    merge_array(sorted_vertex[1], length, input_data_sorted_4, input_data_sorted_unique_num_4,
        sorted_vertex[0], length);
    merge_array(rev_data_mapping, node_num, input_data_sorted_5, input_data_sorted_unique_num_5,
        sorted_vertex[1], length);

    int* sorted_nodes = sorted_vertex[0];
    for (int i=0; i<node_num; ++i) {
        c_hash_map_insert(mapping, rev_data_mapping[i], i);
    }

#define def_hash_input_data(tid) \
    for (int i=0; i<input_data_num_##tid; ++i) { \
        input_data_##tid[i][0] = c_hash_map_get(mapping, input_data_##tid[i][0]); \
        input_data_##tid[i][1] = c_hash_map_get(mapping, input_data_##tid[i][1]); \
    }

    def_hash_input_data(0)
    def_hash_input_data(1)
    def_hash_input_data(2)
    def_hash_input_data(3)
    def_hash_input_data(4)
    def_hash_input_data(5)


    printf("node_num: %d\n", node_num);
}

void print_all_data() {
#define def_print_data_for(tid) \
    for (int i=0; i<input_data_num_##tid; ++i) { \
        printf("%d %d %d\n", input_data_##tid[i][0], input_data_##tid[i][1], input_data_##tid[i][2]); \
    } \
    printf("\n");

    printf("print data --------------------\n");
    def_print_data_for(0)
    def_print_data_for(1)
    def_print_data_for(2)
    def_print_data_for(3)
    def_print_data_for(4)
    def_print_data_for(5)
    printf("print data over --------------\n");

}

void fire_reader() {
    read_input_file();

    printf("buffer size: %d\n", input_buffer_size); fflush(stdout);
    if (input_buffer[input_buffer_size-1] >= '0' && input_buffer[input_buffer_size-1] <= '9') {
        input_buffer[input_buffer_size ++] = '\n';
    }
    reader_deser_header[0] = 0;
    for (int i=1; i<7; ++i) {
        reader_deser_header[i] = input_buffer_size * i / 6;
        while (reader_deser_header[i] < input_buffer_size) {
            if (input_buffer[reader_deser_header[i]] != '\n') reader_deser_header[i] ++;
            else break;
        }
        reader_deser_header[i] ++;
    }

    pthread_t deser_thr[6];
    pthread_create(deser_thr + 0, NULL, reader_deser_0, (void*)(reader_deser_header + 0));
    pthread_create(deser_thr + 1, NULL, reader_deser_1, (void*)(reader_deser_header + 1));
    pthread_create(deser_thr + 2, NULL, reader_deser_2, (void*)(reader_deser_header + 2));
    pthread_create(deser_thr + 3, NULL, reader_deser_3, (void*)(reader_deser_header + 3));
    pthread_create(deser_thr + 4, NULL, reader_deser_4, (void*)(reader_deser_header + 4));
    pthread_create(deser_thr + 5, NULL, reader_deser_5, (void*)(reader_deser_header + 5));

    for (int i=0; i<6; ++i) pthread_join(deser_thr[i], NULL);

    printf("after deser\n"); fflush(stdout);

    // print_all_data();

    hash_vertex();
}

bool vertex_filter[MAX_NODE], vertex_visit[MAX_NODE];
// o for minimal, 1 for maximal
int in_edge_st[MAX_NODE][2], out_edge_st[MAX_NODE][2];

void fire_filter() {
    int u, v;
    int x, y;

#define def_vertex_statistic_for(tid) \
    for (int i=0; i<input_data_num_##tid; ++i) { \
        u = input_data_##tid[i][0]; v = input_data_##tid[i][1]; \
        x = input_data_##tid[i][2]; \
        if (vertex_visit[u] == false) { \
            vertex_visit[u] = true; \
            out_edge_st[u][0] = out_edge_st[u][1] = x; \
        } else { \
            out_edge_st[u][0] = std::min(out_edge_st[u][0], x); \
            out_edge_st[u][1] = std::max(out_edge_st[u][1], x); \
        } \
        if (vertex_visit[v] == false) { \
            vertex_visit[v] = true; \
            in_edge_st[v][0] = in_edge_st[v][1] = x; \
        } else { \
            in_edge_st[v][0] = std::min(in_edge_st[v][0], x); \
            in_edge_st[v][1] = std::max(in_edge_st[v][1], x); \
        } \
    }

    def_vertex_statistic_for(0)
    def_vertex_statistic_for(1)
    def_vertex_statistic_for(2)
    def_vertex_statistic_for(3)
    def_vertex_statistic_for(4)
    def_vertex_statistic_for(5)

    // todo filter
    // long long lmin, lmax, rmin, rmax;

    // for (int i=0; i<node_num; ++i) {
    //     lmin = in_edge_st[i][0];
    //     lmax = in_edge_st[i][1];
    //     rmin = out_edge_st[i][0];
    //     rmax = out_edge_st[i][1];
    //     if (rmin > 3*lmax || lmin > 5*rmax) {
    //         vertex_filter[i] = true;
    //     }
    // }
}

int build_raw_edges_header[8];

#define def_build_chain_edges_for(tid, nid) \
    for (int i=0; i<input_data_num_##tid; ++i) { \
        u = input_data_##tid[i][0]; \
        if (u < from || u >= to) continue; \
        v = input_data_##tid[i][1]; \
        if (vertex_filter[u] || vertex_filter[v]) continue; \
        insert_chain_edges_##nid(u, v, input_data_##tid[i][2]); \
    }

#define def_build_raw_edges(nid) \
int raw_edges_chain_num_##nid = 1; \
int raw_edges_chain_header_##nid[MAX_NODE]; \
int raw_edges_chain_nxt_##nid[MAX_EDGE]; \
int raw_edges_chain_v_##nid[MAX_EDGE]; \
int raw_edges_chain_value_##nid[MAX_EDGE]; \
\
int raw_edges_num_##nid = 0; \
int raw_edges_header_##nid[MAX_NODE]; \
\
int raw_edges_v_fake_##nid[MAX_EDGE]; \
long long raw_edges_value_fake_##nid[MAX_EDGE]; \
\
int raw_edges_v_##nid[MAX_EDGE]; \
long long raw_edges_value_##nid[MAX_EDGE]; \
int raw_edges_id_##nid[MAX_EDGE]; \
void insert_chain_edges_##nid(int u, int v, int value) { \
    int index = raw_edges_chain_num_##nid ++; \
    raw_edges_chain_v_##nid[index] = v; \
    raw_edges_chain_value_##nid[index] = value; \
    raw_edges_chain_nxt_##nid[index] = raw_edges_chain_header_##nid[u]; \
    raw_edges_chain_header_##nid[u] = index; \
} \
void* build_raw_edges_##nid(void* args) { \
    int from = ((int*)args)[0]; \
    int to = ((int*)args)[1]; \
    int u, v; \
    \
    def_build_chain_edges_for(0, nid) \
    def_build_chain_edges_for(1, nid) \
    def_build_chain_edges_for(2, nid) \
    def_build_chain_edges_for(3, nid) \
    def_build_chain_edges_for(4, nid) \
    def_build_chain_edges_for(5, nid) \
    \
    int num, index; \
    long long lx; \
    raw_edges_header_##nid[from] = 0; \
    for (u=from; u<to; ++u) { \
        for (int i=raw_edges_chain_header_##nid[u]; i!=0; i=raw_edges_chain_nxt_##nid[i]) { \
            v = raw_edges_chain_v_##nid[i]; \
            lx = raw_edges_chain_value_##nid[i]; \
            num = raw_edges_num_##nid ++; \
            raw_edges_v_fake_##nid[num] = v; \
            raw_edges_value_fake_##nid[num] = lx; \
            raw_edges_id_##nid[num] = num; \
        } \
        raw_edges_header_##nid[u+1] = raw_edges_num_##nid; \
        std::sort(raw_edges_id_##nid + raw_edges_header_##nid[u], raw_edges_id_##nid + raw_edges_header_##nid[u+1], [](int x, int y) -> bool { \
            return raw_edges_v_fake_##nid[x] < raw_edges_v_fake_##nid[y]; \
        }); \
        for (int i=raw_edges_header_##nid[u]; i<raw_edges_header_##nid[u+1]; ++i) { \
            index = raw_edges_id_##nid[i]; \
            raw_edges_v_##nid[i] = raw_edges_v_fake_##nid[index]; \
            raw_edges_value_##nid[i] = raw_edges_value_fake_##nid[index]; \
        } \
    } \
    \
    return NULL; \
}

def_build_raw_edges(0)
def_build_raw_edges(1)
def_build_raw_edges(2)
def_build_raw_edges(3)
def_build_raw_edges(4)
def_build_raw_edges(5)

int fwd_edges_num = 0;
int fwd_edges[MAX_EDGE];
int fwd_headers[MAX_NODE];
long long fwd_value[MAX_EDGE];

#define def_build_rev_edges(nid) \
int raw_rev_edges_chain_num_##nid = 1; \
int raw_rev_edges_chain_v_##nid[MAX_EDGE]; \
int raw_rev_edges_chain_header_##nid[MAX_NODE]; \
int raw_rev_edges_chain_nxt_##nid[MAX_NODE]; \
long long raw_rev_edges_chain_value_##nid[MAX_EDGE]; \
void insert_rev_chain_edges_##nid(int u, int v, long long value) { \
    int index = raw_rev_edges_chain_num_##nid ++; \
    raw_rev_edges_chain_v_##nid[index] = v; \
    raw_rev_edges_chain_value_##nid[index] = value; \
    raw_rev_edges_chain_nxt_##nid[index] = raw_rev_edges_chain_header_##nid[u]; \
    raw_rev_edges_chain_header_##nid[u] = index; \
} \
void* build_rev_edges_##nid(void* args) { \
    int from = ((int*)args)[0]; \
    int to = ((int*)args)[1]; \
    int u, v; \
    for (u=0; u<node_num; ++u) { \
        for (int i=fwd_headers[u]; i<fwd_headers[u+1]; ++i) { \
            v = fwd_edges[i]; \
            if (v < from || v >= to) continue; \
            insert_rev_chain_edges_##nid(v, u, fwd_value[i]); \
        } \
    } \
    \
    int index; \
    long long lx; \
    raw_edges_header_0[from] = raw_edges_num_##nid = 0; \
    for (u=from; u<to; ++u) { \
        for (int i=raw_rev_edges_chain_header_##nid[u]; i!=0; i=raw_rev_edges_chain_nxt_##nid[i]) { \
            v = raw_rev_edges_chain_v_##nid[i]; \
            lx = raw_rev_edges_chain_value_##nid[i]; \
            index = raw_edges_num_##nid ++; \
            raw_edges_v_fake_##nid[index] = v; \
            raw_edges_value_fake_##nid[index] = lx; \
            raw_edges_id_##nid[index] = index; \
        } \
        raw_edges_header_##nid[u+1] = raw_edges_num_##nid; \
        std::sort(raw_edges_id_##nid + raw_edges_header_##nid[u], raw_edges_id_##nid + raw_edges_header_##nid[u+1], [](int x, int y) -> bool { \
            return raw_edges_v_fake_##nid[x] < raw_edges_v_fake_##nid[y]; \
        }); \
        for (int i=raw_edges_header_##nid[u]; i<raw_edges_header_##nid[u+1]; ++i) { \
            index = raw_edges_id_##nid[i]; \
            raw_edges_v_##nid[i] = raw_edges_v_fake_##nid[index]; \
            raw_edges_value_##nid[i] = raw_edges_value_fake_##nid[index]; \
        } \
    } \
    return NULL; \
}


def_build_rev_edges(0)
def_build_rev_edges(1)
def_build_rev_edges(2)
def_build_rev_edges(3)
def_build_rev_edges(4)
def_build_rev_edges(5)

int bck_edges_num = 0;
int bck_edges[MAX_EDGE];
int bck_headers[MAX_NODE];
long long bck_value[MAX_EDGE];

void fire_build_edge() {
    build_raw_edges_header[0] = 0;
    for (int i=1; i<=6; ++i) build_raw_edges_header[i] = i * node_num / 6;
    for (int i=0; i<7; ++i) printf("%d ", build_raw_edges_header[i]); printf("\n");

    pthread_t build_edges_thr[6];
    pthread_create(build_edges_thr + 0, NULL, build_raw_edges_0, (void*)(build_raw_edges_header + 0));
    pthread_create(build_edges_thr + 1, NULL, build_raw_edges_1, (void*)(build_raw_edges_header + 1));
    pthread_create(build_edges_thr + 2, NULL, build_raw_edges_2, (void*)(build_raw_edges_header + 2));
    pthread_create(build_edges_thr + 3, NULL, build_raw_edges_3, (void*)(build_raw_edges_header + 3));
    pthread_create(build_edges_thr + 4, NULL, build_raw_edges_4, (void*)(build_raw_edges_header + 4));
    pthread_create(build_edges_thr + 5, NULL, build_raw_edges_5, (void*)(build_raw_edges_header + 5));

    for (int i=0; i<6; ++i) pthread_join(build_edges_thr[i], NULL);

    int from, to, size;

#define def_build_edges_for(nid) \
    from = build_raw_edges_header[nid]; to = build_raw_edges_header[nid+1]; \
    size = raw_edges_num_##nid; \
    memcpy(fwd_edges + fwd_edges_num, raw_edges_v_##nid, size * sizeof(int)); \
    memcpy(fwd_value + fwd_edges_num, raw_edges_value_##nid, size * sizeof(long long)); \
    for (int i=from; i<to; ++i) { \
        fwd_headers[i] = raw_edges_header_##nid[i] + fwd_edges_num; \
    } \
    fwd_edges_num += size;

    def_build_edges_for(0)
    def_build_edges_for(1)
    def_build_edges_for(2)
    def_build_edges_for(3)
    def_build_edges_for(4)
    def_build_edges_for(5)

    fwd_headers[node_num] = fwd_edges_num;

    // for (int i=0; i<node_num; ++i) {
    //     printf("[%d]: ", rev_data_mapping[i]);
    //     for (int j=fwd_headers[i]; j<fwd_headers[i+1]; ++j) {
    //         printf("%d[%lld] ", rev_data_mapping[fwd_edges[j]], fwd_value[j]);
    //     }
    //     printf("\n");
    // }

    // printf("fwd size: %d\n", fwd_edges_num); fflush(stdout);

    pthread_t build_rev_edges_thr[6];
    pthread_create(build_rev_edges_thr + 0, NULL, build_rev_edges_0, (void*)(build_raw_edges_header + 0));
    pthread_create(build_rev_edges_thr + 1, NULL, build_rev_edges_1, (void*)(build_raw_edges_header + 1));
    pthread_create(build_rev_edges_thr + 2, NULL, build_rev_edges_2, (void*)(build_raw_edges_header + 2));
    pthread_create(build_rev_edges_thr + 3, NULL, build_rev_edges_3, (void*)(build_raw_edges_header + 3));
    pthread_create(build_rev_edges_thr + 4, NULL, build_rev_edges_4, (void*)(build_raw_edges_header + 4));
    pthread_create(build_rev_edges_thr + 5, NULL, build_rev_edges_5, (void*)(build_raw_edges_header + 5));

    for (int i=0; i<6; ++i) pthread_join(build_rev_edges_thr[i], NULL);

#define def_build_rev_edges_for(nid) \
    from = build_raw_edges_header[nid]; to = build_raw_edges_header[nid+1]; \
    size = raw_edges_num_##nid; \
    memcpy(bck_edges + bck_edges_num, raw_edges_v_##nid, size * sizeof(int)); \
    memcpy(bck_value + bck_edges_num, raw_edges_value_##nid, size * sizeof(long long)); \
    for (int i=from; i<to; ++i) { \
        bck_headers[i] = raw_edges_header_##nid[i] + bck_edges_num; \
    } \
    bck_edges_num += size;

    def_build_rev_edges_for(0)
    def_build_rev_edges_for(1)
    def_build_rev_edges_for(2)
    def_build_rev_edges_for(3)
    def_build_rev_edges_for(4)
    def_build_rev_edges_for(5)

    bck_headers[node_num] = bck_edges_num;

    // for (int i=0; i<node_num; ++i) {
    //     printf("[%d]: ", rev_data_mapping[i]);
    //     for (int j=bck_headers[i]; j<bck_headers[i+1]; ++j) {
    //         printf("%d[%lld] ", rev_data_mapping[bck_edges[j]], bck_value[j]);
    //     }
    //     printf("\n");
    // }

    // printf("rev size: %d\n", bck_edges_num); fflush(stdout);


}

inline bool check_x_y(long long x, long long y) {
    return x <= 5*y && y <= 3*x; 
}

int global_search_sequence[MAX_EDGE][2];
int global_search_assign[MAX_EDGE];
int global_search_num = 0, global_search_flag = 0;

#define def_do_search(tid) \
int total_answer_num_##tid = 0; \
int answer_##tid[MAX_ANSW / 4][7], answer_from_##tid[5][MAX_NODE], answer_to_##tid[5][MAX_NODE]; \
int answer_num_##tid = 0, answer_bck_num_##tid = MAX_ANSW / 4; \
\
int fwd_starter_##tid[MAX_NODE]; \
inline void update_fwd_starter_##tid(int v, int starter) { \
    while (fwd_starter_##tid[v] < fwd_headers[v+1] && fwd_edges[fwd_starter_##tid[v]] <= starter) fwd_starter_##tid[v] ++; \
} \
int bck_search_1_visit_##tid[MAX_NODE], bck_search_1_mask_##tid = 123; \
\
int bck_search_1_queue_num_##tid = 0; \
int bck_search_1_queue_edges_##tid[MAX_BCK_1], bck_search_1_queue_values_##tid[MAX_BCK_1]; \
int bck_search_1_queue_header_##tid[MAX_NODE]; \
\
void bck_1_bfs_##tid(int left, int right, long long y) { \
    bck_search_1_mask_##tid ++; \
    bck_search_1_queue_num_##tid = 0; \
    int v, index; long long x; \
    for (int i=bck_headers[left+1]-1; i>=bck_headers[left]; --i) { \
        v = bck_edges[i]; \
        x = bck_value[i]; \
        if (v <= left) break; \
        if (v == right) continue; \
        if (check_x_y(x, y)) { \
            index = bck_search_1_queue_num_##tid ++; \
            bck_search_1_queue_edges_##tid[index] = v; \
            bck_search_1_queue_values_##tid[index] = x; \
            \
            if (bck_search_1_visit_##tid[v] != bck_search_1_mask_##tid) { \
                bck_search_1_visit_##tid[v] = bck_search_1_mask_##tid; \
            } \
            bck_search_1_queue_header_##tid[v] = index; \
        } \
    } \
} \
\
int bck_search_2_visit_##tid[MAX_NODE], bck_search_2_mask_##tid = 1234; \
\
int bck_search_2_queue_num_##tid = 0; \
int bck_search_2_queue_edges_##tid[MAX_BCK_2][2]; \
long long bck_search_2_queue_values_##tid[MAX_BCK_2]; \
int bck_search_2_queue_header_##tid[MAX_NODE], bck_search_2_queue_nxt_##tid[MAX_BCK_2]; \
\
int bck_search_2_sorted_visit_##tid[MAX_NODE]; \
int bck_search_2_sorted_id_from_##tid[MAX_NODE], bck_search_2_sorted_id_to_##tid[MAX_NODE]; \
int bck_search_2_sorted_id_##tid[MAX_BCK_2], bck_search_2_sorted_id_num_##tid; \
\
void bck_2_bfs_##tid(int left, int right) { \
    bck_search_2_mask_##tid ++; \
    bck_search_2_queue_num_##tid = 0; \
    bck_search_2_sorted_id_num_##tid = 0; \
    \
    int u, v, index; long long x, y; \
    for (int i=0; i<bck_search_1_queue_num_##tid; ++i) { \
        y = bck_search_1_queue_values_##tid[i]; \
        u = bck_search_1_queue_edges_##tid[i]; \
        for (int j=bck_headers[u+1]-1; j>=bck_headers[u]; --j) { \
            v = bck_edges[j]; \
            if (v <= left) break; \
            if (v == right) continue; \
            x = bck_value[j]; \
            if (check_x_y(x, y)) { \
                index = bck_search_2_queue_num_##tid ++; \
                bck_search_2_queue_edges_##tid[index][0] = u; \
                bck_search_2_queue_edges_##tid[index][1] = v; \
                bck_search_2_queue_values_##tid[index] = x; \
                \
                if (bck_search_2_visit_##tid[v] != bck_search_2_mask_##tid) { \
                    bck_search_2_visit_##tid[v] = bck_search_2_mask_##tid; \
                    bck_search_2_queue_header_##tid[v] = -1; \
                } \
                bck_search_2_queue_nxt_##tid[index] = bck_search_2_queue_header_##tid[v]; \
                bck_search_2_queue_header_##tid[v] = index; \
            } \
        } \
    } \
} \
void bck_2_workout_sorted_##tid(int u) { \
    if (bck_search_2_sorted_visit_##tid[u] == bck_search_2_mask_##tid) return; \
    bck_search_2_sorted_visit_##tid[u] = bck_search_2_mask_##tid; \
    bck_search_2_sorted_id_from_##tid[u] = bck_search_2_sorted_id_num_##tid; \
    for (int i=bck_search_2_queue_header_##tid[u]; i!=-1; i=bck_search_2_queue_nxt_##tid[i]) { \
        bck_search_2_sorted_id_##tid[bck_search_2_sorted_id_num_##tid ++] = i; \
    } \
    bck_search_2_sorted_id_to_##tid[u] = bck_search_2_sorted_id_num_##tid; \
    std::sort(bck_search_2_sorted_id_##tid + bck_search_2_sorted_id_from_##tid[u], bck_search_2_sorted_id_##tid + bck_search_2_sorted_id_to_##tid[u], \
        [](int i, int j) -> bool { return bck_search_2_queue_edges_##tid[i][0] < bck_search_2_queue_edges_##tid[j][0]; } \
    ); \
} \
\
int bck_search_3_visit_##tid[MAX_NODE], bck_search_3_mask_##tid = 12345; \
\
int bck_search_3_queue_num_##tid = 0; \
int bck_search_3_queue_edges_##tid[MAX_BCK_3][3]; \
long long bck_search_3_queue_values_##tid[MAX_BCK_3]; \
int bck_search_3_queue_header_##tid[MAX_NODE], bck_search_3_queue_nxt_##tid[MAX_BCK_3]; \
\
int bck_search_3_sorted_visit_##tid[MAX_NODE]; \
int bck_search_3_sorted_id_from_##tid[MAX_NODE], bck_search_3_sorted_id_to_##tid[MAX_NODE]; \
int bck_search_3_sorted_id_##tid[MAX_BCK_3], bck_search_3_sorted_id_num_##tid; \
\
void bck_3_bfs_##tid(int left, int right) { \
    bck_search_3_mask_##tid ++; \
    bck_search_3_queue_num_##tid = 0; \
    bck_search_3_sorted_id_num_##tid = 0; \
    \
    int u, v, w, index; long long x, y; \
    for (int i=0; i<bck_search_2_queue_num_##tid; ++i) { \
        y = bck_search_2_queue_values_##tid[i]; \
        u = bck_search_2_queue_edges_##tid[i][1]; \
        w = bck_search_2_queue_edges_##tid[i][0]; \
        for (int j=bck_headers[u+1]-1; j>=bck_headers[u]; --j) { \
            v = bck_edges[j]; \
            if (v <= left) break; \
            if (v == right || v == w) continue; \
            x = bck_value[j]; \
            if (check_x_y(x, y)) { \
                index = bck_search_3_queue_num_##tid ++; \
                bck_search_3_queue_edges_##tid[index][0] = w; \
                bck_search_3_queue_edges_##tid[index][1] = u; \
                bck_search_3_queue_edges_##tid[index][2] = v; \
                bck_search_3_queue_values_##tid[index] = x; \
                \
                if (bck_search_3_visit_##tid[v] != bck_search_3_mask_##tid) { \
                    bck_search_3_visit_##tid[v] = bck_search_3_mask_##tid; \
                    bck_search_3_queue_header_##tid[v] = -1; \
                } \
                bck_search_3_queue_nxt_##tid[index] = bck_search_3_queue_header_##tid[v]; \
                bck_search_3_queue_header_##tid[v] = index; \
            } \
        } \
    } \
} \
void bck_3_workout_sorted_##tid(int u) { \
    if (bck_search_3_sorted_visit_##tid[u] == bck_search_3_mask_##tid) return; \
    bck_search_3_sorted_visit_##tid[u] = bck_search_3_mask_##tid; \
    bck_search_3_sorted_id_from_##tid[u] = bck_search_3_sorted_id_num_##tid; \
    for (int i=bck_search_3_queue_header_##tid[u]; i!=-1; i=bck_search_3_queue_nxt_##tid[i]) { \
        bck_search_3_sorted_id_##tid[bck_search_3_sorted_id_num_##tid ++] = i; \
    } \
    bck_search_3_sorted_id_to_##tid[u] = bck_search_3_sorted_id_num_##tid; \
    std::sort(bck_search_3_sorted_id_##tid + bck_search_3_sorted_id_from_##tid[u], \
              bck_search_3_sorted_id_##tid + bck_search_3_sorted_id_to_##tid[u], \
              [](int i, int j) -> bool { \
                    if (bck_search_3_queue_edges_##tid[i][0] != bck_search_3_queue_edges_##tid[j][0]) \
                        return bck_search_3_queue_edges_##tid[i][0] < bck_search_3_queue_edges_##tid[j][0]; \
                    return bck_search_3_queue_edges_##tid[i][1] != bck_search_3_queue_edges_##tid[j][1]; \
              } \
        ); \
} \
\
int fwd_search_1_queue_num_##tid = 0; \
int fwd_search_1_queue_edges_##tid[MAX_BCK_1]; \
long long fwd_search_1_queue_values_##tid[MAX_BCK_1]; \
\
void fwd_1_bfs_##tid(int left, int right, long long x, int global_id) { \
    fwd_search_1_queue_num_##tid = 0; \
    int v, index; long long y, z; \
    \
    answer_from_##tid[0][global_id] = answer_num_##tid; \
    update_fwd_starter_##tid(right, left); \
    for (int i=fwd_starter_##tid[right]; i<fwd_headers[right+1]; ++i) { \
        v = fwd_edges[i]; \
        y = fwd_value[i]; \
        if (check_x_y(x, y)) { \
            index = fwd_search_1_queue_num_##tid ++; \
            fwd_search_1_queue_edges_##tid[index] = v; \
            fwd_search_1_queue_values_##tid[index] = y; \
            \
            if (bck_search_1_visit_##tid[v] != bck_search_1_mask_##tid) continue; \
            index = bck_search_1_queue_header_##tid[v]; \
            z = bck_search_1_queue_values_##tid[index]; \
            if (!check_x_y(y, z)) continue; \
            index = answer_num_##tid ++; \
            answer_##tid[index][0] = left; \
            answer_##tid[index][1] = right; \
            answer_##tid[index][2] = v; \
            total_answer_num_##tid ++; \
        } \
    } \
    answer_to_##tid[0][global_id] = answer_num_##tid; \
} \
\
int fwd_search_2_queue_num_##tid = 0; \
int fwd_search_2_queue_edges_##tid[MAX_BCK_2][2]; \
long long fwd_search_2_queue_values_##tid[MAX_BCK_2]; \
\
void fwd_2_bfs_##tid(int left, int right, int global_id) { \
    fwd_search_2_queue_num_##tid = 0; \
    int u, v, w, index; long long x, y, z; \
    \
    answer_from_##tid[1][global_id] = answer_bck_num_##tid; \
    answer_from_##tid[2][global_id] = answer_num_##tid; \
    \
    for (int i=0; i<fwd_search_1_queue_num_##tid; ++i) { \
        u = fwd_search_1_queue_edges_##tid[i]; \
        x = fwd_search_1_queue_values_##tid[i]; \
        update_fwd_starter_##tid(u, left); \
        for (int j=fwd_starter_##tid[u]; j<fwd_headers[u+1]; ++j) { \
            v = fwd_edges[j]; \
            if (v == right) continue; \
            y = fwd_value[j]; \
            if (!check_x_y(x, y)) continue; \
            index = fwd_search_2_queue_num_##tid ++; \
            fwd_search_2_queue_edges_##tid[index][0] = u; \
            fwd_search_2_queue_edges_##tid[index][1] = v; \
            fwd_search_2_queue_values_##tid[index] = y; \
            \
            if (bck_search_1_visit_##tid[v] == bck_search_1_mask_##tid) { \
                index = bck_search_1_queue_header_##tid[v]; \
                z = bck_search_1_queue_values_##tid[index]; \
                if (check_x_y(y, z)) { \
                    index = -- answer_bck_num_##tid; \
                    answer_##tid[index][0] = left; \
                    answer_##tid[index][1] = right; \
                    answer_##tid[index][2] = u; \
                    answer_##tid[index][3] = v; \
                    total_answer_num_##tid ++; \
                } \
            } \
            \
            if (bck_search_2_visit_##tid[v] == bck_search_2_mask_##tid) { \
                bck_2_workout_sorted_##tid(v); \
                for (int k=bck_search_2_sorted_id_from_##tid[v]; k<bck_search_2_sorted_id_to_##tid[v]; ++k) { \
                    index = bck_search_2_sorted_id_##tid[k]; \
                    z = bck_search_2_queue_values_##tid[index]; \
                    w = bck_search_2_queue_edges_##tid[index][0]; \
                    if (w != u && check_x_y(y, z)) { \
                        index = answer_num_##tid ++; \
                        answer_##tid[index][0] = left; \
                        answer_##tid[index][1] = right; \
                        answer_##tid[index][2] = u; \
                        answer_##tid[index][3] = v; \
                        answer_##tid[index][4] = w; \
                        total_answer_num_##tid ++; \
                    } \
                } \
            } \
        } \
    } \
    \
    answer_to_##tid[1][global_id] = answer_bck_num_##tid; \
    answer_to_##tid[2][global_id] = answer_num_##tid; \
} \
\
void fwd_3_bfs_##tid(int left, int right, int global_id) { \
    int u, v, w, ww, o, index; long long x, y, z; \
    \
    answer_from_##tid[3][global_id] = answer_bck_num_##tid; \
    answer_from_##tid[4][global_id] = answer_num_##tid; \
    \
    for (int i=0; i<fwd_search_2_queue_num_##tid; ++i) { \
        u = fwd_search_2_queue_edges_##tid[i][1]; \
        o = fwd_search_2_queue_edges_##tid[i][0]; \
        x = fwd_search_2_queue_values_##tid[i]; \
        update_fwd_starter_##tid(u, left); \
        for (int j=fwd_starter_##tid[u]; j<fwd_headers[u+1]; ++j) { \
            v = fwd_edges[j]; \
            if (v == right || v == o) continue; \
            y = fwd_value[j]; \
            if (!check_x_y(x, y)) continue; \
            \
            if (bck_search_2_visit_##tid[v] == bck_search_2_mask_##tid) { \
                bck_2_workout_sorted_##tid(v); \
                for (int k=bck_search_2_sorted_id_from_##tid[v]; k<bck_search_2_sorted_id_to_##tid[v]; ++k) { \
                    index = bck_search_2_sorted_id_##tid[k]; \
                    z = bck_search_2_queue_values_##tid[index]; \
                    w = bck_search_2_queue_edges_##tid[index][0]; \
                    if (w != o && w != u && check_x_y(y, z)) { \
                        index = -- answer_bck_num_##tid; \
                        answer_##tid[index][0] = left; \
                        answer_##tid[index][1] = right; \
                        answer_##tid[index][2] = o; \
                        answer_##tid[index][3] = u; \
                        answer_##tid[index][4] = v; \
                        answer_##tid[index][5] = w; \
                        total_answer_num_##tid ++; \
                    } \
                } \
            } \
            \
            if (bck_search_3_visit_##tid[v] == bck_search_3_mask_##tid) { \
                bck_3_workout_sorted_##tid(v); \
                for (int k=bck_search_3_sorted_id_from_##tid[v]; k<bck_search_3_sorted_id_to_##tid[v]; ++k) { \
                    index = bck_search_3_sorted_id_##tid[k]; \
                    z = bck_search_3_queue_values_##tid[index]; \
                    w = bck_search_3_queue_edges_##tid[index][1]; \
                    ww = bck_search_3_queue_edges_##tid[index][0]; \
                    if (ww != o && ww != u && w != o && w != u && check_x_y(y, z)) { \
                        index = answer_num_##tid ++; \
                        answer_##tid[index][0] =left; \
                        answer_##tid[index][1] = right; \
                        answer_##tid[index][2] = o; \
                        answer_##tid[index][3] = u; \
                        answer_##tid[index][4] = v; \
                        answer_##tid[index][5] = w; \
                        answer_##tid[index][6] = ww; \
                        total_answer_num_##tid ++; \
                    } \
                } \
            } \
        } \
    } \
    \
    answer_to_##tid[3][global_id] = answer_bck_num_##tid; \
    answer_to_##tid[4][global_id] = answer_num_##tid; \
} \
\
void do_search_##tid(int left, int right, long long x, int global_id) { \
    bck_1_bfs_##tid(left, right, x); \
    if (bck_search_1_queue_num_##tid == 0) return; \
    fwd_1_bfs_##tid(left, right, x, global_id); \
    if (fwd_search_1_queue_num_##tid == 0) return; \
    bck_2_bfs_##tid(left, right); \
    fwd_2_bfs_##tid(left, right, global_id); \
    if (fwd_search_2_queue_num_##tid == 0) return; \
    bck_3_bfs_##tid(left, right); \
    fwd_3_bfs_##tid(left, right, global_id); \
} \
\
void* search_##tid(void* args) { \
    memcpy(fwd_starter_##tid, fwd_headers, sizeof(int) * node_num); \
    int u, v, i, j; \
    long long x; \
    while (true) { \
        i = __sync_fetch_and_add(&global_search_flag, 1); \
        if (i >= global_search_num) break; \
        j = global_search_sequence[i][0]; \
        u = global_search_sequence[i][1]; \
        v = fwd_edges[j]; \
        x = fwd_value[j]; \
        global_search_assign[i] = tid; \
        do_search_##tid(u, v, x, i); \
    } \
    return NULL; \
}

def_do_search(0)
def_do_search(1)
def_do_search(2)
def_do_search(3)
def_do_search(4)
def_do_search(5)

void print_answer() {
    for (int i=0; i<node_num; ++i) {
        for (int j=answer_from_0[0][i]; j<answer_to_0[0][i]; ++j) {
            for (int k=0; k<3; ++k) {
                printf("%d ", rev_data_mapping[answer_0[j][k]]);
            }
            printf("\n");
        }
        for (int j=answer_from_0[1][i]-1; j>=answer_to_0[1][i]; --j) {
            for (int k=0; k<4; ++k) {
                printf("%d ", rev_data_mapping[answer_0[j][k]]);
            }
            printf("\n");
        }
        for (int j=answer_from_0[2][i]; j<answer_to_0[2][i]; ++j) {
            for (int k=0; k<5; ++k) {
                printf("%d ", rev_data_mapping[answer_0[j][k]]);
            }
            printf("\n");
        }
        for (int j=answer_from_0[3][i]-1; j>=answer_to_0[3][i]; ++j) {
            for (int k=0; k<6; ++k) {
                printf("%d ", rev_data_mapping[answer_0[j][k]]);
            }
            printf("\n");
        }
        for (int j=answer_from_0[4][i]; j<answer_to_0[4][i]; ++j) {
            for (int k=0; k<7; ++k) {
                printf("%d ", rev_data_mapping[answer_0[j][k]]);
            }
            printf("\n");
        }
    }
}

int total_answer = 0;

void fire_search() {
    int u, v;
    for (u=0; u<node_num; ++u) {
        for (int i=fwd_headers[u]; i<fwd_headers[u+1]; ++i) {
            v = fwd_edges[i];
            if (v <= u) continue;
            global_search_sequence[global_search_num][0] = i;
            global_search_sequence[global_search_num][1] = u;
            global_search_num ++;
        }
    }
    
    pthread_t search_thr[6];
    pthread_create(search_thr + 0, NULL, search_0, NULL);
    pthread_create(search_thr + 1, NULL, search_1, NULL);
    pthread_create(search_thr + 2, NULL, search_2, NULL);
    pthread_create(search_thr + 3, NULL, search_3, NULL);
    pthread_create(search_thr + 4, NULL, search_4, NULL);
    pthread_create(search_thr + 5, NULL, search_5, NULL);

    for (int i=0; i<6; ++i) pthread_join(search_thr[i], NULL);

    total_answer += total_answer_num_0;
    total_answer += total_answer_num_1;
    total_answer += total_answer_num_2;
    total_answer += total_answer_num_3;
    total_answer += total_answer_num_4;
    total_answer += total_answer_num_5;

    printf("total answer: %d\n", total_answer);
    // print_answer();
}

void serialize_int(char* buffer, int& buffer_num, int x) {
    int from = buffer_num;
    while (x) {
        buffer[buffer_num ++] = x % 10 + '0';
        x /= 10;
    }
    char ch;
    for (int i=from, j=buffer_num-1; i<j; ++i, --j) {
        ch = buffer[i]; buffer[i] = buffer[j]; buffer[j] = ch;
    }
}

int serialize_buffer_header[8];
char serialize_buffer[MAX_NODE][10];
int serialize_buffer_length[MAX_NODE];
#define def_initialize_serialize_buffer(tid) \
void* initialize_serialize_buffer_##tid(void* args) { \
    int from = ((int*)args)[0]; \
    int to = ((int*)args)[1]; \
    int x; char ch; \
    for (int i=from; i<to; ++i) { \
        serialize_int(serialize_buffer[i], serialize_buffer_length[i], rev_data_mapping[i]); \
    } \
    return NULL; \
} \

def_initialize_serialize_buffer(0)
def_initialize_serialize_buffer(1)
def_initialize_serialize_buffer(2)
def_initialize_serialize_buffer(3)
def_initialize_serialize_buffer(4)
def_initialize_serialize_buffer(5)


inline void serialize_line(int* line, int size, char* buffer, int& buffer_num) {
    for (int i=0; i<size; ++i) {
        memcpy(buffer + buffer_num, serialize_buffer[line[i]], serialize_buffer_length[line[i]]);
        buffer_num += serialize_buffer_length[line[i]];
        buffer[buffer_num ++] = i == size-1 ? '\n' : ',';
    }
}

void do_serialize(int from, int to, int len_id, char* buffer, int& buffer_num) {
#define def_ser_for(tid) \
    if ((len_id & 1) == 0) \
        for (int j=answer_from_##tid[len_id][i]; j<answer_to_##tid[len_id][i]; ++j) { \
            serialize_line(answer_##tid[j], size, buffer, buffer_num); \
        } \
    else \
        for (int j=answer_from_##tid[len_id][i]-1; j>=answer_to_##tid[len_id][i]; --j) { \
            serialize_line(answer_##tid[j], size, buffer, buffer_num); \
        }
    
    int size = len_id + 3;
    for (int i=from; i<to; ++i) {
        switch (global_search_assign[i])
        {
        case 0:
            def_ser_for(0)
            break;
        case 1:
            def_ser_for(1)
            break;
        case 2:
            def_ser_for(2)
            break;
        case 3:
            def_ser_for(3)
            break;
        case 4:
            def_ser_for(4)
            break;
        case 5:
            def_ser_for(5)
            break;
        default:
            break;
        }
    }
}

int writer_header[8][3];

#define def_ser_thr(tid) \
char* s_buffer_##tid = (char*)malloc(385000000); \
int s_buffer_num_##tid = 0; \
void* serialize_##tid(void* args) { \
    int* prop = (int*) args; \
    if (prop[0] == -1) { \
        serialize_int(s_buffer_##tid, s_buffer_num_##tid, total_answer); \
        s_buffer_##tid[s_buffer_num_##tid ++] = '\n'; \
        do_serialize(prop[1], prop[2], 0, s_buffer_##tid, s_buffer_num_##tid); \
        do_serialize(prop[1], prop[2], 1, s_buffer_##tid, s_buffer_num_##tid); \
        do_serialize(prop[1], prop[2], 2, s_buffer_##tid, s_buffer_num_##tid); \
    } else { \
        do_serialize(prop[1], prop[2], prop[0], s_buffer_##tid, s_buffer_num_##tid); \
    } \
    return NULL; \
} \

def_ser_thr(0)
def_ser_thr(1)
def_ser_thr(2)
def_ser_thr(3)
def_ser_thr(4)
def_ser_thr(5)

void fire_ser() {

    serialize_buffer_header[0] = 0;
    for (int i=1; i<=6; ++i) serialize_buffer_header[i] = i*node_num / 6;
    pthread_t init_thr[6];
    pthread_create(init_thr + 0, NULL, initialize_serialize_buffer_0, (void*) (serialize_buffer_header + 0));
    pthread_create(init_thr + 1, NULL, initialize_serialize_buffer_1, (void*) (serialize_buffer_header + 1));
    pthread_create(init_thr + 2, NULL, initialize_serialize_buffer_2, (void*) (serialize_buffer_header + 2));
    pthread_create(init_thr + 3, NULL, initialize_serialize_buffer_3, (void*) (serialize_buffer_header + 3));
    pthread_create(init_thr + 4, NULL, initialize_serialize_buffer_4, (void*) (serialize_buffer_header + 4));
    pthread_create(init_thr + 5, NULL, initialize_serialize_buffer_5, (void*) (serialize_buffer_header + 5));
    for (int i=0; i<6; ++i) pthread_join(init_thr[0], NULL);

    // for (int i=0; i<node_num; ++i) {
    //     printf("%d %d: ", i, rev_data_mapping[i]); 
    //     for (int j=0; j<serialize_buffer_length[i]; j++) {
    //         printf("%c", serialize_buffer[i][j]);
    //     }
    //     printf("\n");
    // }

    writer_header[0][0] = -1; writer_header[0][1] = 0; writer_header[0][2] = global_search_num;
    writer_header[1][0] = 3; writer_header[1][1] = 0; writer_header[1][2] = global_search_num;
    writer_header[2][0] = 4; writer_header[2][1] = 0; writer_header[2][2] = global_search_num/8;
    writer_header[3][0] = 4; writer_header[3][1] = global_search_num/8; writer_header[3][2] = global_search_num/4;
    writer_header[4][0] = 4; writer_header[4][1] = global_search_num/4; writer_header[4][2] = global_search_num/2;
    writer_header[5][0] = 4; writer_header[5][1] = global_search_num/2; writer_header[5][2] = global_search_num;

    pthread_t ser_thr[6];
    pthread_create(ser_thr + 0, NULL, serialize_0, (void*)writer_header[0]);
    pthread_create(ser_thr + 1, NULL, serialize_1, (void*)writer_header[1]);
    pthread_create(ser_thr + 2, NULL, serialize_2, (void*)writer_header[2]);
    pthread_create(ser_thr + 3, NULL, serialize_3, (void*)writer_header[3]);
    pthread_create(ser_thr + 4, NULL, serialize_4, (void*)writer_header[4]);
    pthread_create(ser_thr + 5, NULL, serialize_5, (void*)writer_header[5]);
    
    int writer_fd = open(OUTPUT_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    size_t rub;

#define def_ser_thr_join(tid) \
    pthread_join(ser_thr[tid], NULL); \
    printf("s buffer num: %d\n", s_buffer_num_##tid); \
    rub = write(writer_fd, s_buffer_##tid, s_buffer_num_##tid);

    def_ser_thr_join(0)
    def_ser_thr_join(1)
    def_ser_thr_join(2)
    def_ser_thr_join(3)
    def_ser_thr_join(4)
    def_ser_thr_join(5)
}

// ----------------- BODY ----------------------





int main() {
    record_start_time();
    fire_reader();
    record_end_time("fire reader");

    // fire_filter();
    // record_end_time("fire filter");

    fire_build_edge();
    record_end_time("fire build edges");


    fire_search();
    record_end_time("fire search");

    fire_ser();
    record_end_time("fire ser");
}



































// ------------------- template --------------------------
int c_hash_map_hash(int x) {
    return hash_map_mask & ((x >> 16) ^ x);
}
int c_hash_map_size(c_hash_map hash) {
    return hash -> counter - 1;
}
void c_hash_map_insert(c_hash_map hash, int key, int value) {
    int index = c_hash_map_hash(key);
    hash->key[hash->counter] = key;
    hash->value[hash->counter] = value;
    hash->ptr[hash->counter] = hash->header[index];
    hash->header[index] = hash->counter ++;
}
void c_hash_map_replace(c_hash_map hash, int key, int value) {
    int index = c_hash_map_hash(key);
    for (int i=hash->header[index]; i!=0; i=hash->ptr[i]) {
        if (key == hash->key[i]) {
            hash->value[i] = value;
        }
    }
}
int c_hash_map_get(c_hash_map hash, int key) {
    int index = c_hash_map_hash(key);
    for (int i=hash->header[index]; i!=0; i=hash->ptr[i]) {
        if (key == hash->key[i]) {
            return hash->value[i];
        }
    }
    return -1;
}