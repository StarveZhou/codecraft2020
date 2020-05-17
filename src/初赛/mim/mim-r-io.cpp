#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <pthread.h>

#include <string.h>
#include <assert.h>
#include <math.h>

#include <algorithm>

//------------------ MACRO ---------------------


#include <time.h>
clock_t start_time, end_time, mid_time;

// 日志输出
#include <stdio.h>

// #define INPUT_PATH   "resources/topo_edge_test.txt"
// #define INPUT_PATH   "resources/topo_test.txt"
// #define INPUT_PATH  "resources/data1.txt"
#define INPUT_PATH  "resources/2861665.txt"
// #define INPUT_PATH   "resources/test_data.txt"
// #define INPUT_PATH   "resources/pre_test.txt"
#define OUTPUT_PATH  "r_io_mim.txt"


// #define INPUT_PATH  "/data/test_data.txt"
// #define OUTPUT_PATH "/projects/student/result.txt"

#define MAX_NODE               570000
#define MAX_EDGE               285056
#define MAX_ANSW               3000128
#define MAX_FOR_EACH_LINE      34
#define ONE_INT_CHAR_SIZE      11
#define MAX_3_ANSW             500000
#define MAX_4_ANSW             1000000
#define PATH_PAR_2             500000
#define PATH_PAR_3             1000000
#define PATH_PAR_4             1500000

#define CACHE_LINE_ALIGN       __attribute__((aligned(64)))
#define likely(x)             __builtin_expect((x),1)
#define unlikely(x)           __builtin_expect((x),0)

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







char CACHE_LINE_ALIGN buffer[(MAX_EDGE * MAX_FOR_EACH_LINE)];

int CACHE_LINE_ALIGN data[MAX_EDGE][3];
int CACHE_LINE_ALIGN data_num;

int CACHE_LINE_ALIGN data_rev_mapping[MAX_NODE];
int CACHE_LINE_ALIGN node_num = 0;
c_hash_map_t mapping_t; c_hash_map mapping = &mapping_t;

void read_input() {
    int fd = open(INPUT_PATH, O_RDONLY);
    if (fd == -1) {
        printf("open read only file failed\n");
        exit(0);
    }
    size_t size = read(fd, &buffer, MAX_EDGE * MAX_FOR_EACH_LINE);
    // 这一步没啥必要？
    close(fd);

    // 解析
    
    if (buffer[size-1] >= '0' && buffer[size-1] <= '9') {
        buffer[size ++] = '\n';
    }

    int x = 0;
    int* local_data = &data[0][0];
    for (int i=0; i<size; ++i) {
        if (unlikely(buffer[i] == ',' || buffer[i] == '\n')) {
            *local_data = x;
            local_data ++;
            x = 0;
        } else {
            x = x * 10 + buffer[i] - '0';
        }
    }
    data_num = (local_data - &data[0][0]) / 3;
    printf("data num: %d\n", data_num); fflush(stdout);

    for (int i=0; i<data_num; ++i) {
        if (unlikely(c_hash_map_get(mapping, data[i][0]) == -1)) {
            c_hash_map_insert(mapping, data[i][0], 0);
            data_rev_mapping[node_num ++] = data[i][0];
        }
        if (unlikely(c_hash_map_get(mapping, data[i][1]) == -1)) {
            c_hash_map_insert(mapping, data[i][1], 0);
            data_rev_mapping[node_num ++] = data[i][1];
        }
    }

    std::sort(data_rev_mapping, data_rev_mapping + node_num);

    for (int i=0; i<node_num; ++i) {
        c_hash_map_replace(mapping, data_rev_mapping[i], i);
    }

    for (int i=0; i<data_num; ++i) {
        x = c_hash_map_get(mapping, data[i][0]);
        data[i][0] = x;
        x = c_hash_map_get(mapping, data[i][1]);
        data[i][1] = x;
    }

}

int writer_fd;
void create_writer_fd() {
    writer_fd = open(OUTPUT_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (writer_fd == -1) {
        printf("failed open!");
        exit(0);
    }
}


int CACHE_LINE_ALIGN edge_num = 1;
int CACHE_LINE_ALIGN edge_v[MAX_EDGE];
int CACHE_LINE_ALIGN edge_ptr[MAX_EDGE];
int CACHE_LINE_ALIGN edge_header[MAX_NODE];
bool CACHE_LINE_ALIGN filter_useless_node[MAX_EDGE];
int CACHE_LINE_ALIGN filter_cnt[MAX_NODE];

int CACHE_LINE_ALIGN node_queue[MAX_NODE << 1];


void node_topo_filter(bool revert) {
    int u, v, head = (revert == false), tail = (0 ^ head);
    if (edge_num != 1) {
        memset(edge_header, 0, sizeof(int) * node_num);
        memset(filter_cnt, 0, sizeof(int) * node_num);
        edge_num = 0;
    }
    for (int i=0; i<data_num; ++i) {
        u = data[i][head]; v = data[i][tail];
        if (unlikely(filter_useless_node[u] || filter_useless_node[v])) continue;

        edge_v[edge_num] = v;
        edge_ptr[edge_num] = edge_header[u];
        edge_header[u] = edge_num ++;

        filter_cnt[v] ++;
    }

    head = tail = 0;
    for (int i=0; i<node_num; ++i) if (unlikely(filter_cnt[i] == 0)) {
        node_queue[tail ++] = i;
    }
    while (head < tail) {
        u = node_queue[head ++];
        filter_useless_node[u] = true;
        for (int i=edge_header[u]; i!=0; i=edge_ptr[i]) {
            v = edge_v[i];
            filter_cnt[v] --;
            if (unlikely(filter_cnt[v] == 0)) node_queue[tail ++] = i;
        }
    }
}

int CACHE_LINE_ALIGN rehash_mapping[MAX_NODE];
int CACHE_LINE_ALIGN edge_topo_header[MAX_NODE];
int CACHE_LINE_ALIGN edge_topo_edges[MAX_EDGE];
int CACHE_LINE_ALIGN rev_edge_topo_header[MAX_NODE];
int CACHE_LINE_ALIGN rev_edge_topo_edges[MAX_NODE];

void rehash_nodes() {
    int rehash_node_num = 0;
    for (int i=0; i<node_num; ++i) {
        if (unlikely(filter_useless_node[i])) continue;
        data_rev_mapping[rehash_node_num] = data_rev_mapping[i];
        rehash_mapping[i] = rehash_node_num ++;
    }


    int u, v;
    edge_num = 1;
    memset(edge_header, 0, sizeof(int) * rehash_node_num);
    for (int i=0; i<data_num; ++i) {
        u = data[i][0]; v = data[i][1];
        if (unlikely(filter_useless_node[u] || filter_useless_node[v])) continue;
        u = rehash_mapping[u]; v = rehash_mapping[v];
        
        edge_v[edge_num] = v;
        edge_ptr[edge_num] = edge_header[u];
        edge_header[u] = edge_num ++; 
    }
    node_num = rehash_node_num;

    edge_num = 0;
    for (u=0; u<node_num; ++u) {
        for (int i=edge_header[u]; i!=0; i=edge_ptr[i]) {
            v = edge_v[i];
            edge_topo_edges[edge_num ++] = v;
        }
        edge_topo_header[u+1] = edge_num;
        if (edge_topo_header[u+1] == edge_topo_header[u] + 1) {
            continue;
        } else {
            std::sort(edge_topo_edges + edge_topo_header[u], edge_topo_edges + edge_topo_header[u+1]);
        }
    }

    // build rev edge for bfs
    edge_num = 1;
    memset(edge_header, 0, sizeof(int) * rehash_node_num);
    for (u=0; u<node_num; ++u) {
        for (int i=edge_topo_header[u]; i<edge_topo_header[u+1]; ++i) {
            v = edge_topo_edges[i];

            edge_v[edge_num] = u;
            edge_ptr[edge_num] = edge_header[v];
            edge_header[v] = edge_num ++;            
        }
    }
    edge_num = 0;
    for (u=0; u<node_num; ++u) {
        for (int i=edge_header[u]; i!=0; i=edge_ptr[i]) {
            v = edge_v[i];
            rev_edge_topo_edges[edge_num ++] = v;
        }
        rev_edge_topo_header[u+1] = edge_num;
        std::sort(rev_edge_topo_edges + rev_edge_topo_header[u], rev_edge_topo_edges + rev_edge_topo_header[u+1], [](int i, int j) -> bool {
            return i > j;
        });
    }
}




bool CACHE_LINE_ALIGN searchable_nodes[2][MAX_NODE];
void filter_searchable_nodes() {
    int v;
    for (int u=0; u<node_num; ++u) {
        for (int j=edge_topo_header[u]; j<edge_topo_header[u+1]; ++j) {
            v = edge_topo_edges[j];
            if (v < u) searchable_nodes[0][v] = true;
            if (v > u) searchable_nodes[1][u] = true;
        }
    }
    for (int u=0; u<node_num; ++u) {
        searchable_nodes[0][u] &= searchable_nodes[1][u];
    }

    // int total_nodes_c = 0, searchable_nodes_c = 0;
    // for (int i=0; i<node_num; ++i) {
    //     total_nodes_c ++;
    //     searchable_nodes_c += (searchable_nodes[0][i] == true && searchable_nodes[1][i] == true);
    // }
    // printf("total node: %d, searchable: %d\n", total_nodes_c, searchable_nodes_c);
}

char CACHE_LINE_ALIGN integer_buffer[MAX_NODE][10];
int CACHE_LINE_ALIGN integer_buffer_size[MAX_NODE];
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

#define MAX_SP_DIST       (4)

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

int std_id_arr[PATH_PAR_3];
int sizeof_int_mul_node_num;

// ********************************************* SEARCH THREAD
// ********************************************* SEARCH THREAD
// ********************************************* SEARCH THREAD
// ********************************************* SEARCH THREAD
// ********************************************* SEARCH THREAD
// ********************************************* SEARCH THREAD
// ********************************************* SEARCH THREAD
// ********************************************* SEARCH THREAD


#define MAX_THR                 (4)
#define CACHE_FILL              (32)

int global_search_flag = 0;
int global_search_assignment[MAX_NODE];
int search_thr_id[CACHE_FILL];




int CACHE_LINE_ALIGN visit[MAX_THR][MAX_NODE], mask[MAX_NODE][CACHE_FILL];
int CACHE_LINE_ALIGN edge_topo_starter[MAX_THR][MAX_NODE];
int dfs_path[MAX_THR][CACHE_FILL], dfs_path_num[MAX_THR][CACHE_FILL];
// here
char dfs_path_buffer[MAX_THR][ONE_INT_CHAR_SIZE * 64];
int dfs_path_buffer_bound[MAX_THR][CACHE_FILL];
int dfs_path_buffer_index_now[MAX_THR][CACHE_FILL];

char CACHE_LINE_ALIGN answer_3[MAX_THR][MAX_3_ANSW * ONE_INT_CHAR_SIZE * 3];
char CACHE_LINE_ALIGN answer_4[MAX_THR][MAX_4_ANSW * ONE_INT_CHAR_SIZE * 4];
int answer_3_buffer_num[MAX_THR][CACHE_FILL], answer_4_buffer_num[MAX_THR][CACHE_FILL];
int answer_3_num[MAX_THR][CACHE_FILL], answer_4_num[MAX_THR][CACHE_FILL];
int answer_3_from[MAX_THR][MAX_NODE], answer_3_to[MAX_THR][MAX_NODE];
int answer_4_from[MAX_THR][MAX_NODE], answer_4_to[MAX_THR][MAX_NODE];

inline void check_forward_path(int thr_id, int need_depth) {
    int index = dfs_path_buffer_index_now[thr_id][0];
    while (index < need_depth) {
        dfs_path_buffer_bound[thr_id][index] = dfs_path_buffer_bound[thr_id][index-1];
        deserialize_id(dfs_path_buffer[thr_id], dfs_path_buffer_bound[thr_id][index], dfs_path[thr_id][index]);
        dfs_path_buffer[thr_id][dfs_path_buffer_bound[thr_id][index] ++] = ',';
        index ++;
    }
    dfs_path_buffer_index_now[thr_id][0] = index;
}

inline void extract_3_answer(int thr_id) {
    check_forward_path(thr_id, 3);
    answer_3_num[thr_id][0] ++;
    memcpy(answer_3[thr_id] + answer_3_buffer_num[thr_id][0], dfs_path_buffer[thr_id], dfs_path_buffer_bound[thr_id][2]);
    answer_3_buffer_num[thr_id][0] += dfs_path_buffer_bound[thr_id][3];
    answer_3[thr_id][answer_3_buffer_num[thr_id][0] - 1] = '\n';
}

inline void extract_4_answer(int thr_id) {
    check_forward_path(thr_id, 4);
    answer_4_num[thr_id][0] ++;
    memcpy(answer_4[thr_id] + answer_4_buffer_num[thr_id][0], dfs_path_buffer[thr_id], dfs_path_buffer_bound[thr_id][3]);
    answer_4_buffer_num[thr_id][0] += dfs_path_buffer_bound[thr_id][4];
    answer_4[thr_id][answer_4_buffer_num[thr_id][0] - 1] = '\n';
}


int backward_3_path_header[MAX_THR][MAX_NODE];
int backward_3_path_ptr[MAX_THR][MAX_NODE];
int backward_3_path_value[MAX_THR][PATH_PAR_3][2];
int backward_3_num[MAX_THR][CACHE_FILL];
int backward_contains[MAX_THR][MAX_NODE], backward_contains_num[MAX_THR][CACHE_FILL];

int backward_success[MAX_THR][CACHE_FILL];

inline void extract_backward_3_path(int thr_id) {
    backward_success[thr_id][0] = dfs_path[thr_id][0];
    if (backward_3_path_header[thr_id][dfs_path[thr_id][3]] == -1) backward_contains[thr_id][backward_contains_num[thr_id][0] ++] = dfs_path[thr_id][3];
    backward_3_path_value[thr_id][backward_3_num[thr_id][0]][0] = dfs_path[thr_id][1];
    backward_3_path_value[thr_id][backward_3_num[thr_id][0]][1] = dfs_path[thr_id][2];
    backward_3_path_ptr[thr_id][backward_3_num[thr_id][0]] = backward_3_path_header[thr_id][dfs_path[thr_id][3]];
    backward_3_path_header[thr_id][dfs_path[thr_id][3]] = backward_3_num[thr_id][0] ++;
}



void backward_dfs(int thr_id) {
    int u, v, original_path_num = dfs_path_num[thr_id][0];
    u = dfs_path[thr_id][dfs_path_num[thr_id][0] - 1]; dfs_path_num[thr_id][0] ++;
    for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
        v = rev_edge_topo_edges[i];
        if (v <= dfs_path[thr_id][0]) break;
        if (visit[thr_id][v] == mask[thr_id][0]) continue;
        dfs_path[thr_id][original_path_num] = v; 
        if (dfs_path_num[thr_id][0] == 4) {
            extract_backward_3_path(thr_id);
            continue;
        }
        visit[thr_id][v] = mask[thr_id][0];
        backward_dfs(thr_id);
        visit[thr_id][v] = 0;
    }
    dfs_path_num[thr_id][0] --;
}

int sorted_backward_3_path[MAX_THR][PATH_PAR_3];
int sorted_backward_3_path_fin[MAX_THR][PATH_PAR_3][2];
int sorted_backward_3_path_from[MAX_THR][MAX_NODE];
int sorted_backward_3_path_to[MAX_THR][MAX_NODE];
int sorted_backward_id[MAX_THR][PATH_PAR_3];
int sorted_backward_buffer_num[MAX_THR][PATH_PAR_3][CACHE_FILL];
char sorted_backward_buffer[MAX_THR][PATH_PAR_3][ONE_INT_CHAR_SIZE << 1];

void sort_out(int thr_id) {
    int i, u, v, num = 0, from, to, x, y;
    for (i=0; i<backward_contains_num[thr_id][0]; ++i) {
        u = backward_contains[thr_id][i];
        from = sorted_backward_3_path_from[thr_id][u] = num;
        for (int j=backward_3_path_header[thr_id][u]; j!=-1; j=backward_3_path_ptr[thr_id][j]) {
            sorted_backward_3_path[thr_id][num ++] = j;
        }
        to = sorted_backward_3_path_to[thr_id][u] = num;
        std::sort(sorted_backward_3_path[thr_id] + from, sorted_backward_3_path[thr_id] + to, [thr_id](int x, int y) -> bool {
            if (backward_3_path_value[thr_id][x][1] != backward_3_path_value[thr_id][y][1]) return backward_3_path_value[thr_id][x][1] < backward_3_path_value[thr_id][y][1];
            else return backward_3_path_value[thr_id][x][0] < backward_3_path_value[thr_id][y][0];
        });
        for (int j=from; j<to; ++j) {
            sorted_backward_3_path_fin[thr_id][j][0] = backward_3_path_value[thr_id][sorted_backward_3_path[thr_id][j]][0];
            sorted_backward_3_path_fin[thr_id][j][1] = backward_3_path_value[thr_id][sorted_backward_3_path[thr_id][j]][1];
            sorted_backward_buffer_num[thr_id][j][0] = 0;
        }
    }
} 

inline void check_backward_buffer(int thr_id, int index) {
    if (sorted_backward_buffer_num[thr_id][index][0] != 0) return;
    deserialize_id(sorted_backward_buffer[thr_id][index], sorted_backward_buffer_num[thr_id][index][0], sorted_backward_3_path_fin[thr_id][index][1]);
    sorted_backward_buffer[thr_id][index][sorted_backward_buffer_num[thr_id][index][0] ++] = ',';
    deserialize_id(sorted_backward_buffer[thr_id][index], sorted_backward_buffer_num[thr_id][index][0], sorted_backward_3_path_fin[thr_id][index][0]);
    sorted_backward_buffer[thr_id][index][sorted_backward_buffer_num[thr_id][index][0] ++] = '\n';
}

char answer_5_buffer[MAX_THR][MAX_ANSW * ONE_INT_CHAR_SIZE * 2];
char answer_6_buffer[MAX_THR][MAX_ANSW * ONE_INT_CHAR_SIZE * 2];
char answer_7_buffer[MAX_THR][MAX_ANSW * ONE_INT_CHAR_SIZE * 2];
int answer_5_6_7_buffer_num[MAX_THR][CACHE_FILL];
int answer_5_6_7_num[MAX_THR][CACHE_FILL];
int answer_5_from[MAX_THR][MAX_NODE], answer_5_to[MAX_THR][MAX_NODE];
int answer_6_from[MAX_THR][MAX_NODE], answer_6_to[MAX_THR][MAX_NODE];
int answer_7_from[MAX_THR][MAX_NODE], answer_7_to[MAX_THR][MAX_NODE];

inline void extract_forward_2_path(int thr_id) {
    int v = dfs_path[thr_id][2], x, y;
    for (int i=sorted_backward_3_path_from[thr_id][v]; i<sorted_backward_3_path_to[thr_id][v]; ++i) {
        x = sorted_backward_3_path_fin[thr_id][i][0];
        y = sorted_backward_3_path_fin[thr_id][i][1];
        if (visit[thr_id][x] == mask[thr_id][0] || visit[thr_id][y] == mask[thr_id][0]) continue;

        check_forward_path(thr_id, 3);
        memcpy(answer_5_buffer[thr_id] + answer_5_6_7_buffer_num[thr_id][0], dfs_path_buffer[thr_id], dfs_path_buffer_bound[thr_id][2]);
        answer_5_6_7_buffer_num[thr_id][0] += dfs_path_buffer_bound[thr_id][2];
        check_backward_buffer(thr_id, i);
        memcpy(answer_5_buffer[thr_id] + answer_5_6_7_buffer_num[thr_id][0], sorted_backward_buffer[thr_id][i], sorted_backward_buffer_num[thr_id][i][0]);
        answer_5_6_7_buffer_num[thr_id][0] += sorted_backward_buffer_num[thr_id][i][0];

        answer_5_6_7_num[thr_id][0] ++;
    }
}


inline void extract_forward_3_path(int thr_id) {
    int v = dfs_path[thr_id][3], x, y;
    for (int i=sorted_backward_3_path_from[thr_id][v]; i<sorted_backward_3_path_to[thr_id][v]; ++i) {
        x = sorted_backward_3_path_fin[thr_id][i][0];
        y = sorted_backward_3_path_fin[thr_id][i][1];
        if (visit[thr_id][x] == mask[thr_id][0] || visit[thr_id][y] == mask[thr_id][0]) continue;

        check_forward_path(thr_id, 4);
        memcpy(answer_6_buffer[thr_id] + answer_5_6_7_buffer_num[thr_id][1], dfs_path_buffer[thr_id], dfs_path_buffer_bound[thr_id][3]);
        answer_5_6_7_buffer_num[thr_id][1] += dfs_path_buffer_bound[thr_id][3];
        check_backward_buffer(thr_id, i);
        memcpy(answer_6_buffer[thr_id] + answer_5_6_7_buffer_num[thr_id][1], sorted_backward_buffer[thr_id][i], sorted_backward_buffer_num[thr_id][i][0]);
        answer_5_6_7_buffer_num[thr_id][1] += sorted_backward_buffer_num[thr_id][i][0];

        answer_5_6_7_num[thr_id][1] ++;
    }
}

inline void extract_forward_4_path(int thr_id) {
    int v = dfs_path[thr_id][4], x, y;
    for (int i=sorted_backward_3_path_from[thr_id][v]; i<sorted_backward_3_path_to[thr_id][v]; ++i) {
        x = sorted_backward_3_path_fin[thr_id][i][0];
        y = sorted_backward_3_path_fin[thr_id][i][1];
        if (visit[thr_id][x] == mask[thr_id][0] || visit[thr_id][y] == mask[thr_id][0]) continue;
        
        check_forward_path(thr_id, 5);
        memcpy(answer_7_buffer[thr_id] + answer_5_6_7_buffer_num[thr_id][2], dfs_path_buffer[thr_id], dfs_path_buffer_bound[thr_id][4]);
        answer_5_6_7_buffer_num[thr_id][2] += dfs_path_buffer_bound[thr_id][4];
        check_backward_buffer(thr_id, i);
        memcpy(answer_7_buffer[thr_id] + answer_5_6_7_buffer_num[thr_id][2], sorted_backward_buffer[thr_id][i], sorted_backward_buffer_num[thr_id][i][0]);
        answer_5_6_7_buffer_num[thr_id][2] += sorted_backward_buffer_num[thr_id][i][0];

        answer_5_6_7_num[thr_id][2] ++;
    }
}

void forward_dfs(int thr_id) {
    int u, v, original_path_num = dfs_path_num[thr_id][0];
    u = dfs_path[thr_id][dfs_path_num[thr_id][0] - 1];  dfs_path_num[thr_id][0] ++;
    while (edge_topo_starter[thr_id][u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter[thr_id][u]] < dfs_path[thr_id][0]) edge_topo_starter[thr_id][u] ++;
    for (int i=edge_topo_starter[thr_id][u]; i<edge_topo_header[u+1]; ++i) {
        v = edge_topo_edges[i];
        if (v == dfs_path[thr_id][0]) {
            if (original_path_num == 3) extract_3_answer(thr_id);
            if (original_path_num == 4) extract_4_answer(thr_id);
            continue;
        }
        if (visit[thr_id][v] == mask[thr_id][0]) continue;
        dfs_path[thr_id][original_path_num] = v; 
        if (backward_3_path_header[thr_id][v] != -1) {
            switch (original_path_num)
            {
            case 2:
                extract_forward_2_path(thr_id);
                break;
            case 3:
                extract_forward_3_path(thr_id);
                break;
            case 4:
                extract_forward_4_path(thr_id);
                break;
            default:
                break;
            }
        }
        if (original_path_num == 4) continue;
        visit[thr_id][v] = mask[thr_id][0];
        forward_dfs(thr_id);
        visit[thr_id][v] = 0;
    }
    dfs_path_num[thr_id][0] --;
    if (dfs_path_num[thr_id][0] < dfs_path_buffer_index_now[thr_id][0]) {
        dfs_path_buffer_index_now[thr_id][0] = dfs_path_num[thr_id][0];
    }
}


void do_search_mim(int begin_with, int thr_id) {
    backward_3_num[thr_id][0] = 0;
    backward_contains_num[thr_id][0] = 0;
    // 可以不用memset，用一个visit就好了
    memset(backward_3_path_header[thr_id], -1, sizeof_int_mul_node_num);
    // printf("started with: %d %d\n", begin_with, data_rev_mapping[begin_with]); fflush(stdout);
    dfs_path[thr_id][0] = begin_with; dfs_path_num[thr_id][0] = 1;
    mask[thr_id][0] ++; visit[thr_id][begin_with] = mask[thr_id][0];
    backward_dfs(thr_id);
    if (backward_success[thr_id][0] != begin_with) {
        return;
    }
    sort_out(thr_id);
    dfs_path[thr_id][0] = begin_with; dfs_path_num[thr_id][0] = 1;
    mask[thr_id][0] ++; visit[thr_id][begin_with] = mask[thr_id][0];

    answer_3_from[thr_id][begin_with] = answer_3_buffer_num[thr_id][0];
    answer_4_from[thr_id][begin_with] = answer_4_buffer_num[thr_id][0];
    answer_5_from[thr_id][begin_with] = answer_5_6_7_buffer_num[thr_id][0];
    answer_6_from[thr_id][begin_with] = answer_5_6_7_buffer_num[thr_id][1];
    answer_7_from[thr_id][begin_with] = answer_5_6_7_buffer_num[thr_id][2];

    dfs_path_buffer_index_now[thr_id][0] = 1;
    dfs_path_buffer_bound[thr_id][0] = 0;
    deserialize_id(dfs_path_buffer[thr_id], dfs_path_buffer_bound[thr_id][0], begin_with);
    dfs_path_buffer[thr_id][dfs_path_buffer_bound[thr_id][0] ++] = ',';

    forward_dfs(thr_id);
    answer_3_to[thr_id][begin_with] = answer_3_buffer_num[thr_id][0];
    answer_4_to[thr_id][begin_with] = answer_4_buffer_num[thr_id][0];
    answer_5_to[thr_id][begin_with] = answer_5_6_7_buffer_num[thr_id][0];
    answer_6_to[thr_id][begin_with] = answer_5_6_7_buffer_num[thr_id][1];
    answer_7_to[thr_id][begin_with] = answer_5_6_7_buffer_num[thr_id][2];    
}

void* search(void* args) {
    int thr_id = *((int*)args);
    memcpy(edge_topo_starter[thr_id], edge_topo_header, sizeof_int_mul_node_num);
    backward_success[thr_id][0] = -1;
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_search_flag, 1);
        if (u >= node_num) break;
        if (unlikely(edge_header[u] == edge_header[u+1])) continue;
        if (unlikely(!searchable_nodes[0][u])) continue;
        global_search_assignment[u] = thr_id;
        do_search_mim(u, thr_id);
    }
    return NULL;
}





// ********************************************************* IO THREAD
// ********************************************************* IO THREAD
// ********************************************************* IO THREAD
// ********************************************************* IO THREAD
// ********************************************************* IO THREAD
// ********************************************************* IO THREAD
// ********************************************************* IO THREAD
// ********************************************************* IO THREAD
// ********************************************************* IO THREAD

#define IO_THR             (13)

int answer_num = 0;

// [0] -> {4(means 3&4), 5, 6, 7}, [1] -> from, [2] -> to
// for [0] == 4: from & to means nothing, and there will only be one 4, which is io thr 0
int io_thr_assign[IO_THR][CACHE_FILL];
int io_thr_id[CACHE_FILL];

char output_buffer[IO_THR][MAX_ANSW * ONE_INT_CHAR_SIZE];
int output_buffer_num[IO_THR][CACHE_FILL];
void* write_to_disk_thr(void* args) {
    int thr_id = *((int*)args), from, to;
    size_t rub;
    if (thr_id == 0) {
        deserialize_int(output_buffer[thr_id], output_buffer_num[thr_id][0], answer_num);
        output_buffer[thr_id][output_buffer_num[thr_id][0] ++] = '\n';
    }

    from = io_thr_assign[thr_id][1]; to = io_thr_assign[thr_id][2];
    int tid, length, i, j, k;
    switch (io_thr_assign[thr_id][0])
    {
    case 4:
        for (i=0; i<node_num; ++i) {
            tid = global_search_assignment[i];
            if (tid < 0) continue;
            length = answer_3_to[tid][i] - answer_3_from[tid][i];
            memcpy(output_buffer[thr_id] + output_buffer_num[thr_id][0], answer_3[tid] + answer_3_from[tid][i], length);
            output_buffer_num[thr_id][0] += length;
        }
        for (i=0; i<node_num; ++i) {
            tid = global_search_assignment[i];
            if (tid < 0) continue;
            length = answer_4_to[tid][i] - answer_4_from[tid][i];
            memcpy(output_buffer[thr_id] + output_buffer_num[thr_id][0], answer_4[tid] + answer_4_from[tid][i], length);
            output_buffer_num[thr_id][0] += length;
        }
        break;
    case 5:
        for (i=from; i<to; ++i) {
            tid = global_search_assignment[i];
            if (tid < 0) continue;
            length = answer_5_to[tid][i] - answer_5_from[tid][i];
            memcpy(output_buffer[thr_id] + output_buffer_num[thr_id][0], answer_5_buffer[tid] + answer_5_from[tid][i], length);
            output_buffer_num[thr_id][0] += length;
        }
        break;
    case 6:
        for (i=from; i<to; ++i) {
            tid = global_search_assignment[i];
            if (tid < 0) continue;
            length = answer_6_to[tid][i] - answer_6_from[tid][i];
            memcpy(output_buffer[thr_id] + output_buffer_num[thr_id][0], answer_6_buffer[tid] + answer_6_from[tid][i], length);
            output_buffer_num[thr_id][0] += length;
        }
        break;
    case 7:
        for (i=from; i<to; ++i) {
            tid = global_search_assignment[i];
            if (tid < 0) continue;
            length = answer_7_to[tid][i] - answer_7_from[tid][i];
            memcpy(output_buffer[thr_id] + output_buffer_num[thr_id][0], answer_7_buffer[tid] + answer_7_from[tid][i], length);
            output_buffer_num[thr_id][0] += length;
        }
        break;
    default:
        break;
    }
    printf("io: %d\n", thr_id); fflush(stdout);
    return NULL;
}

void write_to_disk() {
    // 3&4 -> 1
    // 5 -> 1
    // 6 -> 1
    // 7 -> 10
    io_thr_assign[0][0] = 4;
    io_thr_assign[1][0] = 5; io_thr_assign[1][1] = 0; io_thr_assign[1][2] = node_num;
    io_thr_assign[2][0] = 6; io_thr_assign[2][1] = 0; io_thr_assign[2][2] = node_num;
    for (int i=3; i<13; ++i) io_thr_assign[i][0] = 7;
    int counter = 0, one_tenth = 0;
    for (int j=0; j<MAX_THR; ++j) {
        one_tenth += answer_5_6_7_num[j][2];
    }
    one_tenth /= 10;
    printf("one tenth: %d\n", one_tenth);
    int now = 0;
    for (int i=0; i<9; ++i) {
        while (now < node_num) {
            for (int k=0; k<MAX_THR; ++k) {
                counter += answer_7_to[k][now] - answer_7_from[k][now];
            }
            if (counter > one_tenth) {
                // printf("counter %d %d\n", i+3, counter);
                counter = 0;
                io_thr_assign[i+3][2] = now+1;
                break;
            }
            now ++;
        }
    }
    for (int i=1; i<10; ++i) {
        io_thr_assign[i+3][1] = io_thr_assign[i+2][2];
    }
    io_thr_assign[3][1] = 0; io_thr_assign[12][2] = node_num;
    for (int i=0; i<10; ++i) {
        printf("split: %d %d %d\n", i, io_thr_assign[i+3][1], io_thr_assign[i+3][2]);
    }
    
    for (int i=0; i<13; ++i) io_thr_id[i] = i;


    size_t rub;
    pthread_t io_thr[13];
    for (int i=0; i<=16; ++i) {
        if (i >= 4) {
            pthread_join(io_thr[i-4], NULL);
            rub = write(writer_fd, output_buffer[i-4], output_buffer_num[i-4][0]);
        }
        if (i < 13) {
            pthread_create(io_thr + i, NULL, write_to_disk_thr, (void*)(io_thr_id + i));
        }
    }
}


//------------------ MBODY ---------------------
// 答案是错的
int main() {
    start_time = clock();

    read_input();
    printf("node num: %d\n", node_num); fflush(stdout);
    // for (int i=0; i<node_num; i++) printf("%d ", data_rev_mapping[i]);
    // printf("\n");

    end_time = clock();
    printf("read: %d ms\n", (int)(end_time - start_time)); fflush(stdout);
    

    node_topo_filter(false);
    node_topo_filter(true);
    rehash_nodes();
    

    printf("node num: %d\n", node_num); fflush(stdout);

    filter_searchable_nodes();
    create_integer_buffer();

    sizeof_int_mul_node_num = sizeof(int) * node_num;
    memset(global_search_assignment, -1, sizeof_int_mul_node_num);
    for (int i=0; i<PATH_PAR_3; ++i) std_id_arr[i] = i;

    end_time = clock();
    printf("build edge: %d ms\n", (int)(end_time - start_time)); fflush(stdout);
    int answer_d[5];
    for (int i=0; i<5; ++i) answer_d[i] = 0;
    mid_time = clock();
    pthread_t search_thr[MAX_THR];
    for (int i=0; i<MAX_THR; ++i) search_thr_id[i] = i;
    for (int i=0; i<MAX_THR; ++i) {
        pthread_create(search_thr + i, NULL, search, (void*)(search_thr_id + i));
    }
    for (int i=0; i<MAX_THR; ++i) {
        pthread_join(search_thr[i], NULL);
    }
    for (int i=0; i<MAX_THR; ++i) {
        answer_num += answer_3_num[i][0];
        answer_num += answer_4_num[i][0];
        answer_num += answer_5_6_7_num[i][0];
        answer_num += answer_5_6_7_num[i][1];
        answer_num += answer_5_6_7_num[i][2];

        answer_d[0] += answer_3_num[i][0];
        answer_d[1] += answer_4_num[i][0];
        answer_d[2] += answer_5_6_7_num[i][0];
        answer_d[3] += answer_5_6_7_num[i][1];
        answer_d[4] += answer_5_6_7_num[i][2];
    }
    printf("answer: %d\n", answer_num);
    for (int i=0; i<5; ++i) {
        printf("answer %d: %d\n", i+3, answer_d[i]);
    }
    for (int i=0; i<MAX_THR; ++i) {
        printf("char num: %d %d %d %d %d\n", answer_3_buffer_num[i][0], answer_4_buffer_num[i][0], answer_5_6_7_buffer_num[i][0], answer_5_6_7_buffer_num[i][1],answer_5_6_7_buffer_num[i][2]);
    }
    

    end_time = clock();
    printf("searching: %d ms\n", (int)(end_time - mid_time)); fflush(stdout);

    

    end_time = clock();
    printf("create integer buffer: %d ms\n", (int)(end_time - start_time)); fflush(stdout);

    mid_time = clock();
    create_writer_fd();
    write_to_disk();
    close(writer_fd);

    end_time = clock();
    printf("writing: %d ms\n", (int)(end_time - mid_time)); fflush(stdout);
    printf("running: %d ms\n", (int)(end_time - start_time)); fflush(stdout);
    return 0;
}

//------------------ FBODY ---------------------


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