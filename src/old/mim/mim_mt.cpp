#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <pthread.h>

#include <string.h>
#include <assert.h>

#include <algorithm>

//------------------ MACRO ---------------------


#include <time.h>
clock_t start_time, end_time, mid_time;

// 日志输出
#include <stdio.h>

// #define INPUT_PATH   "resources/topo_edge_test.txt"
// #define INPUT_PATH   "resources/topo_test.txt"
// #define INPUT_PATH  "resources/data1.txt"
// #define INPUT_PATH  "resources/2861665.txt"
#define INPUT_PATH   "resources/2755223.txt"
// #define INPUT_PATH   "resources/test_data.txt"
// #define INPUT_PATH   "resources/pre_test.txt"
#define OUTPUT_PATH  "output_mim_mt.txt"


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

int answer_num = 0;

// ********************************************* SEARCH THREAD
// ********************************************* SEARCH THREAD
// ********************************************* SEARCH THREAD
// ********************************************* SEARCH THREAD
// ********************************************* SEARCH THREAD
// ********************************************* SEARCH THREAD
// ********************************************* SEARCH THREAD
// ********************************************* SEARCH THREAD

// cas

int global_search_flag = 0;
int global_search_assignment[MAX_NODE];

// search thread 0

int CACHE_LINE_ALIGN visit_mt_0[MAX_NODE], mask_mt_0 = 1111;
int CACHE_LINE_ALIGN edge_topo_starter_mt_0[MAX_NODE];
int dfs_path_mt_0[5], dfs_path_num_mt_0;

char CACHE_LINE_ALIGN answer_3_mt_0[MAX_3_ANSW * ONE_INT_CHAR_SIZE * 3];
char CACHE_LINE_ALIGN answer_4_mt_0[MAX_4_ANSW * ONE_INT_CHAR_SIZE * 4];
int answer_3_buffer_num_mt_0, answer_4_buffer_num_mt_0;
int answer_3_num_mt_0 = 0, answer_4_num_mt_0 = 0;

inline void extract_3_answer_mt_0() {
    answer_3_num_mt_0 ++;
    deserialize_id(answer_3_mt_0, answer_3_buffer_num_mt_0, dfs_path_mt_0[0]);
    answer_3_mt_0[answer_3_buffer_num_mt_0 ++] = ',';
    deserialize_id(answer_3_mt_0, answer_3_buffer_num_mt_0, dfs_path_mt_0[1]);
    answer_3_mt_0[answer_3_buffer_num_mt_0 ++] = ',';
    deserialize_id(answer_3_mt_0, answer_3_buffer_num_mt_0, dfs_path_mt_0[2]);
    answer_3_mt_0[answer_3_buffer_num_mt_0 ++] = '\n';
}

inline void extract_4_answer_mt_0() {
    answer_4_num_mt_0 ++;
    deserialize_id(answer_4_mt_0, answer_4_buffer_num_mt_0, dfs_path_mt_0[0]);
    answer_4_mt_0[answer_4_buffer_num_mt_0 ++] = ',';
    deserialize_id(answer_4_mt_0, answer_4_buffer_num_mt_0, dfs_path_mt_0[1]);
    answer_4_mt_0[answer_4_buffer_num_mt_0 ++] = ',';
    deserialize_id(answer_4_mt_0, answer_4_buffer_num_mt_0, dfs_path_mt_0[2]);
    answer_4_mt_0[answer_4_buffer_num_mt_0 ++] = ',';
    deserialize_id(answer_4_mt_0, answer_4_buffer_num_mt_0, dfs_path_mt_0[3]);
    answer_4_mt_0[answer_4_buffer_num_mt_0 ++] = '\n';
}


int backward_3_path_header_mt_0[MAX_NODE];
int backward_3_path_ptr_mt_0[MAX_NODE];
int backward_3_path_value_mt_0[PATH_PAR_3][2];
int backward_3_num_mt_0 = 0;
int backward_contains_mt_0[MAX_NODE], backward_contains_num_mt_0 = 0;

inline void extract_backward_3_path_mt_0() {
    if (backward_3_path_header_mt_0[dfs_path_mt_0[3]] == -1) backward_contains_mt_0[backward_contains_num_mt_0 ++] = dfs_path_mt_0[3];
    backward_3_path_value_mt_0[backward_3_num_mt_0][0] = dfs_path_mt_0[1];
    backward_3_path_value_mt_0[backward_3_num_mt_0][1] = dfs_path_mt_0[2];
    backward_3_path_ptr_mt_0[backward_3_num_mt_0] = backward_3_path_header_mt_0[dfs_path_mt_0[3]];
    backward_3_path_header_mt_0[dfs_path_mt_0[3]] = backward_3_num_mt_0 ++;
}



void backward_dfs_mt_0() {
    int u, v, original_path_num = dfs_path_num_mt_0;
    u = dfs_path_mt_0[dfs_path_num_mt_0 - 1]; dfs_path_num_mt_0 ++;
    for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
        v = rev_edge_topo_edges[i];
        if (v <= dfs_path_mt_0[0]) break;
        if (visit_mt_0[v] == mask_mt_0) continue;
        dfs_path_mt_0[original_path_num] = v; 
        if (dfs_path_num_mt_0 == 4) {
            extract_backward_3_path_mt_0();
            continue;
        }
        visit_mt_0[v] = mask_mt_0;
        backward_dfs_mt_0();
        visit_mt_0[v] = 0;
    }
    dfs_path_num_mt_0 --;
}

int sorted_backward_3_path_mt_0[PATH_PAR_3];
int sorted_backward_3_path_fin_mt_0[PATH_PAR_3][2];
int sorted_backward_3_path_from_mt_0[MAX_NODE];
int sorted_backward_3_path_to_mt_0[MAX_NODE];
int sorted_backward_id_mt_0[PATH_PAR_3];

void sort_out_mt_0(int begin_with) {
    int i, u, v, num = 0, from, to;
    for (i=0; i<backward_contains_num_mt_0; ++i) {
        u = backward_contains_mt_0[i];
        from = sorted_backward_3_path_from_mt_0[u] = num;
        for (int j=backward_3_path_header_mt_0[u]; j!=-1; j=backward_3_path_ptr_mt_0[j]) {
            sorted_backward_3_path_mt_0[num ++] = j;
        }
        to = sorted_backward_3_path_to_mt_0[u] = num;
        std::sort(sorted_backward_3_path_mt_0 + from, sorted_backward_3_path_mt_0 + to, [](int x, int y) -> bool {
            if (backward_3_path_value_mt_0[x][1] != backward_3_path_value_mt_0[y][1]) return backward_3_path_value_mt_0[x][1] < backward_3_path_value_mt_0[y][1];
            else return backward_3_path_value_mt_0[x][0] < backward_3_path_value_mt_0[y][0];
        });
        for (int j=from; j<to; ++j) {
            sorted_backward_3_path_fin_mt_0[j][0] = backward_3_path_value_mt_0[sorted_backward_3_path_mt_0[j]][0];
            sorted_backward_3_path_fin_mt_0[j][1] = backward_3_path_value_mt_0[sorted_backward_3_path_mt_0[j]][1];
        }
    }
} 


int answer_5_mt_0[MAX_ANSW][5];
int answer_6_mt_0[MAX_ANSW][6];
int answer_7_mt_0[MAX_ANSW][7];
int answer_5_num_mt_0 = 0, answer_6_num_mt_0 = 0, answer_7_num_mt_0 = 0;

int answer_3_header_mt_0[MAX_NODE], answer_3_tailer_mt_0[MAX_NODE];
int answer_4_header_mt_0[MAX_NODE], answer_4_tailer_mt_0[MAX_NODE];
int answer_5_header_mt_0[MAX_NODE], answer_5_tailer_mt_0[MAX_NODE];
int answer_6_header_mt_0[MAX_NODE], answer_6_tailer_mt_0[MAX_NODE];
int answer_7_header_mt_0[MAX_NODE], answer_7_tailer_mt_0[MAX_NODE];

inline void extract_forward_2_path_mt_0() {
    int v = dfs_path_mt_0[2], x, y;
    for (int i=sorted_backward_3_path_from_mt_0[v]; i<sorted_backward_3_path_to_mt_0[v]; ++i) {
        x = sorted_backward_3_path_fin_mt_0[i][0];
        y = sorted_backward_3_path_fin_mt_0[i][1];
        if (visit_mt_0[x] == mask_mt_0 || visit_mt_0[y] == mask_mt_0) continue;
        memcpy(answer_5_mt_0[answer_5_num_mt_0], dfs_path_mt_0, sizeof(int) * 3);
        answer_5_mt_0[answer_5_num_mt_0][3] = y;
        answer_5_mt_0[answer_5_num_mt_0][4] = x;
        answer_5_num_mt_0 ++;
    }
}


inline void extract_forward_3_path_mt_0() {
    int v = dfs_path_mt_0[3], x, y;
    for (int i=sorted_backward_3_path_from_mt_0[v]; i<sorted_backward_3_path_to_mt_0[v]; ++i) {
        x = sorted_backward_3_path_fin_mt_0[i][0];
        y = sorted_backward_3_path_fin_mt_0[i][1];
        if (visit_mt_0[x] == mask_mt_0 || visit_mt_0[y] == mask_mt_0) continue;
        memcpy(answer_6_mt_0[answer_6_num_mt_0], dfs_path_mt_0, sizeof(int) * 4);
        answer_6_mt_0[answer_6_num_mt_0][4] = y;
        answer_6_mt_0[answer_6_num_mt_0][5] = x;
        answer_6_num_mt_0 ++;
    }
}

inline void extract_forward_4_path_mt_0() {
    int v = dfs_path_mt_0[4], x, y;
    for (int i=sorted_backward_3_path_from_mt_0[v]; i<sorted_backward_3_path_to_mt_0[v]; ++i) {
        x = sorted_backward_3_path_fin_mt_0[i][0];
        y = sorted_backward_3_path_fin_mt_0[i][1];
        if (visit_mt_0[x] == mask_mt_0 || visit_mt_0[y] == mask_mt_0) continue;
        memcpy(answer_7_mt_0[answer_7_num_mt_0], dfs_path_mt_0, sizeof(int) * 5);
        answer_7_mt_0[answer_7_num_mt_0][5] = y;
        answer_7_mt_0[answer_7_num_mt_0][6] = x;
        answer_7_num_mt_0 ++;
    }
}

void forward_dfs_mt_0() {
    int u, v, original_path_num = dfs_path_num_mt_0;
    u = dfs_path_mt_0[dfs_path_num_mt_0 - 1];  dfs_path_num_mt_0 ++;
    while (edge_topo_starter_mt_0[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_0[u]] < dfs_path_mt_0[0]) edge_topo_starter_mt_0[u] ++;
    for (int i=edge_topo_starter_mt_0[u]; i<edge_topo_header[u+1]; ++i) {
        v = edge_topo_edges[i];
        if (v == dfs_path_mt_0[0]) {
            if (original_path_num == 3) extract_3_answer_mt_0();
            if (original_path_num == 4) extract_4_answer_mt_0();
            continue;
        }
        if (visit_mt_0[v] == mask_mt_0) continue;
        dfs_path_mt_0[original_path_num] = v; 
        if (backward_3_path_header_mt_0[v] != -1) {
            switch (original_path_num)
            {
            case 2:
                extract_forward_2_path_mt_0();
                break;
            case 3:
                extract_forward_3_path_mt_0();
                break;
            case 4:
                extract_forward_4_path_mt_0();
                break;
            default:
                break;
            }
        }
        if (original_path_num == 4) continue;
        visit_mt_0[v] = mask_mt_0;
        forward_dfs_mt_0();
        visit_mt_0[v] = 0;
    }
    dfs_path_num_mt_0 --;
}

void do_search_mim_mt_0(int begin_with) {
    backward_3_num_mt_0 = 0;
    backward_contains_num_mt_0 = 0;
    // 可以不用memset，用一个visit就好了
    memset(backward_3_path_header_mt_0 + begin_with, -1, sizeof(int) * (node_num - begin_with));
    // printf("started with: %d %d\n", begin_with, data_rev_mapping[begin_with]); fflush(stdout);
    dfs_path_mt_0[0] = begin_with; dfs_path_num_mt_0 = 1;
    mask_mt_0 ++; visit_mt_0[begin_with] = mask_mt_0;
    backward_dfs_mt_0();
    if (backward_3_num_mt_0 == 0) return;
    sort_out_mt_0(begin_with);
    dfs_path_mt_0[0] = begin_with; dfs_path_num_mt_0 = 1;
    mask_mt_0 ++; visit_mt_0[begin_with] = mask_mt_0;

    answer_3_header_mt_0[begin_with] = answer_3_buffer_num_mt_0;
    answer_4_header_mt_0[begin_with] = answer_4_buffer_num_mt_0;
    answer_5_header_mt_0[begin_with] = answer_5_num_mt_0;
    answer_6_header_mt_0[begin_with] = answer_6_num_mt_0;
    answer_7_header_mt_0[begin_with] = answer_7_num_mt_0;

    forward_dfs_mt_0();
    
    answer_3_tailer_mt_0[begin_with] = answer_3_buffer_num_mt_0;
    answer_4_tailer_mt_0[begin_with] = answer_4_buffer_num_mt_0;
    answer_5_tailer_mt_0[begin_with] = answer_5_num_mt_0;
    answer_6_tailer_mt_0[begin_with] = answer_6_num_mt_0;
    answer_7_tailer_mt_0[begin_with] = answer_7_num_mt_0;    
}

void* search_mt_0(void* args) {
    memcpy(edge_topo_starter_mt_0, edge_topo_header, sizeof_int_mul_node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_search_flag, 1);
        if (u >= node_num) break;
        // NOTE: this should be replaced
        global_search_assignment[u] = 0;
        if (unlikely(edge_header[u] == edge_header[u+1])) continue;
        if (unlikely(!searchable_nodes[0][u])) continue;
        do_search_mim_mt_0(u);
    }
    return NULL;
}

// search thread 0

// search thread 1

int CACHE_LINE_ALIGN visit_mt_1[MAX_NODE], mask_mt_1 = 1111;
int CACHE_LINE_ALIGN edge_topo_starter_mt_1[MAX_NODE];
int dfs_path_mt_1[5], dfs_path_num_mt_1;

char CACHE_LINE_ALIGN answer_3_mt_1[MAX_3_ANSW * ONE_INT_CHAR_SIZE * 3];
char CACHE_LINE_ALIGN answer_4_mt_1[MAX_4_ANSW * ONE_INT_CHAR_SIZE * 4];
int answer_3_buffer_num_mt_1, answer_4_buffer_num_mt_1;
int answer_3_num_mt_1 = 0, answer_4_num_mt_1 = 0;

inline void extract_3_answer_mt_1() {
    answer_3_num_mt_1 ++;
    deserialize_id(answer_3_mt_1, answer_3_buffer_num_mt_1, dfs_path_mt_1[0]);
    answer_3_mt_1[answer_3_buffer_num_mt_1 ++] = ',';
    deserialize_id(answer_3_mt_1, answer_3_buffer_num_mt_1, dfs_path_mt_1[1]);
    answer_3_mt_1[answer_3_buffer_num_mt_1 ++] = ',';
    deserialize_id(answer_3_mt_1, answer_3_buffer_num_mt_1, dfs_path_mt_1[2]);
    answer_3_mt_1[answer_3_buffer_num_mt_1 ++] = '\n';
}

inline void extract_4_answer_mt_1() {
    answer_4_num_mt_1 ++;
    deserialize_id(answer_4_mt_1, answer_4_buffer_num_mt_1, dfs_path_mt_1[0]);
    answer_4_mt_1[answer_4_buffer_num_mt_1 ++] = ',';
    deserialize_id(answer_4_mt_1, answer_4_buffer_num_mt_1, dfs_path_mt_1[1]);
    answer_4_mt_1[answer_4_buffer_num_mt_1 ++] = ',';
    deserialize_id(answer_4_mt_1, answer_4_buffer_num_mt_1, dfs_path_mt_1[2]);
    answer_4_mt_1[answer_4_buffer_num_mt_1 ++] = ',';
    deserialize_id(answer_4_mt_1, answer_4_buffer_num_mt_1, dfs_path_mt_1[3]);
    answer_4_mt_1[answer_4_buffer_num_mt_1 ++] = '\n';
}


int backward_3_path_header_mt_1[MAX_NODE];
int backward_3_path_ptr_mt_1[MAX_NODE];
int backward_3_path_value_mt_1[PATH_PAR_3][2];
int backward_3_num_mt_1 = 0;
int backward_contains_mt_1[MAX_NODE], backward_contains_num_mt_1 = 0;

inline void extract_backward_3_path_mt_1() {
    if (backward_3_path_header_mt_1[dfs_path_mt_1[3]] == -1) backward_contains_mt_1[backward_contains_num_mt_1 ++] = dfs_path_mt_1[3];
    backward_3_path_value_mt_1[backward_3_num_mt_1][0] = dfs_path_mt_1[1];
    backward_3_path_value_mt_1[backward_3_num_mt_1][1] = dfs_path_mt_1[2];
    backward_3_path_ptr_mt_1[backward_3_num_mt_1] = backward_3_path_header_mt_1[dfs_path_mt_1[3]];
    backward_3_path_header_mt_1[dfs_path_mt_1[3]] = backward_3_num_mt_1 ++;
}



void backward_dfs_mt_1() {
    int u, v, original_path_num = dfs_path_num_mt_1;
    u = dfs_path_mt_1[dfs_path_num_mt_1 - 1]; dfs_path_num_mt_1 ++;
    for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
        v = rev_edge_topo_edges[i];
        if (v <= dfs_path_mt_1[0]) break;
        if (visit_mt_1[v] == mask_mt_1) continue;
        dfs_path_mt_1[original_path_num] = v; 
        if (dfs_path_num_mt_1 == 4) {
            extract_backward_3_path_mt_1();
            continue;
        }
        visit_mt_1[v] = mask_mt_1;
        backward_dfs_mt_1();
        visit_mt_1[v] = 0;
    }
    dfs_path_num_mt_1 --;
}

int sorted_backward_3_path_mt_1[PATH_PAR_3];
int sorted_backward_3_path_fin_mt_1[PATH_PAR_3][2];
int sorted_backward_3_path_from_mt_1[MAX_NODE];
int sorted_backward_3_path_to_mt_1[MAX_NODE];
int sorted_backward_id_mt_1[PATH_PAR_3];

void sort_out_mt_1(int begin_with) {
    int i, u, v, num = 0, from, to;
    for (i=0; i<backward_contains_num_mt_1; ++i) {
        u = backward_contains_mt_1[i];
        from = sorted_backward_3_path_from_mt_1[u] = num;
        for (int j=backward_3_path_header_mt_1[u]; j!=-1; j=backward_3_path_ptr_mt_1[j]) {
            sorted_backward_3_path_mt_1[num ++] = j;
        }
        to = sorted_backward_3_path_to_mt_1[u] = num;
        std::sort(sorted_backward_3_path_mt_1 + from, sorted_backward_3_path_mt_1 + to, [](int x, int y) -> bool {
            if (backward_3_path_value_mt_1[x][1] != backward_3_path_value_mt_1[y][1]) return backward_3_path_value_mt_1[x][1] < backward_3_path_value_mt_1[y][1];
            else return backward_3_path_value_mt_1[x][0] < backward_3_path_value_mt_1[y][0];
        });
        for (int j=from; j<to; ++j) {
            sorted_backward_3_path_fin_mt_1[j][0] = backward_3_path_value_mt_1[sorted_backward_3_path_mt_1[j]][0];
            sorted_backward_3_path_fin_mt_1[j][1] = backward_3_path_value_mt_1[sorted_backward_3_path_mt_1[j]][1];
        }
    }
} 


int answer_5_mt_1[MAX_ANSW][5];
int answer_6_mt_1[MAX_ANSW][6];
int answer_7_mt_1[MAX_ANSW][7];
int answer_5_num_mt_1 = 0, answer_6_num_mt_1 = 0, answer_7_num_mt_1 = 0;

int answer_3_header_mt_1[MAX_NODE], answer_3_tailer_mt_1[MAX_NODE];
int answer_4_header_mt_1[MAX_NODE], answer_4_tailer_mt_1[MAX_NODE];
int answer_5_header_mt_1[MAX_NODE], answer_5_tailer_mt_1[MAX_NODE];
int answer_6_header_mt_1[MAX_NODE], answer_6_tailer_mt_1[MAX_NODE];
int answer_7_header_mt_1[MAX_NODE], answer_7_tailer_mt_1[MAX_NODE];

inline void extract_forward_2_path_mt_1() {
    int v = dfs_path_mt_1[2], x, y;
    for (int i=sorted_backward_3_path_from_mt_1[v]; i<sorted_backward_3_path_to_mt_1[v]; ++i) {
        x = sorted_backward_3_path_fin_mt_1[i][0];
        y = sorted_backward_3_path_fin_mt_1[i][1];
        if (visit_mt_1[x] == mask_mt_1 || visit_mt_1[y] == mask_mt_1) continue;
        memcpy(answer_5_mt_1[answer_5_num_mt_1], dfs_path_mt_1, sizeof(int) * 3);
        answer_5_mt_1[answer_5_num_mt_1][3] = y;
        answer_5_mt_1[answer_5_num_mt_1][4] = x;
        answer_5_num_mt_1 ++;
    }
}


inline void extract_forward_3_path_mt_1() {
    int v = dfs_path_mt_1[3], x, y;
    for (int i=sorted_backward_3_path_from_mt_1[v]; i<sorted_backward_3_path_to_mt_1[v]; ++i) {
        x = sorted_backward_3_path_fin_mt_1[i][0];
        y = sorted_backward_3_path_fin_mt_1[i][1];
        if (visit_mt_1[x] == mask_mt_1 || visit_mt_1[y] == mask_mt_1) continue;
        memcpy(answer_6_mt_1[answer_6_num_mt_1], dfs_path_mt_1, sizeof(int) * 4);
        answer_6_mt_1[answer_6_num_mt_1][4] = y;
        answer_6_mt_1[answer_6_num_mt_1][5] = x;
        answer_6_num_mt_1 ++;
    }
}

inline void extract_forward_4_path_mt_1() {
    int v = dfs_path_mt_1[4], x, y;
    for (int i=sorted_backward_3_path_from_mt_1[v]; i<sorted_backward_3_path_to_mt_1[v]; ++i) {
        x = sorted_backward_3_path_fin_mt_1[i][0];
        y = sorted_backward_3_path_fin_mt_1[i][1];
        if (visit_mt_1[x] == mask_mt_1 || visit_mt_1[y] == mask_mt_1) continue;
        memcpy(answer_7_mt_1[answer_7_num_mt_1], dfs_path_mt_1, sizeof(int) * 5);
        answer_7_mt_1[answer_7_num_mt_1][5] = y;
        answer_7_mt_1[answer_7_num_mt_1][6] = x;
        answer_7_num_mt_1 ++;
    }
}

void forward_dfs_mt_1() {
    int u, v, original_path_num = dfs_path_num_mt_1;
    u = dfs_path_mt_1[dfs_path_num_mt_1 - 1];  dfs_path_num_mt_1 ++;
    while (edge_topo_starter_mt_1[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_1[u]] < dfs_path_mt_1[0]) edge_topo_starter_mt_1[u] ++;
    for (int i=edge_topo_starter_mt_1[u]; i<edge_topo_header[u+1]; ++i) {
        v = edge_topo_edges[i];
        if (v == dfs_path_mt_1[0]) {
            if (original_path_num == 3) extract_3_answer_mt_1();
            if (original_path_num == 4) extract_4_answer_mt_1();
            continue;
        }
        if (visit_mt_1[v] == mask_mt_1) continue;
        dfs_path_mt_1[original_path_num] = v; 
        if (backward_3_path_header_mt_1[v] != -1) {
            switch (original_path_num)
            {
            case 2:
                extract_forward_2_path_mt_1();
                break;
            case 3:
                extract_forward_3_path_mt_1();
                break;
            case 4:
                extract_forward_4_path_mt_1();
                break;
            default:
                break;
            }
        }
        if (original_path_num == 4) continue;
        visit_mt_1[v] = mask_mt_1;
        forward_dfs_mt_1();
        visit_mt_1[v] = 0;
    }
    dfs_path_num_mt_1 --;
}

void do_search_mim_mt_1(int begin_with) {
    backward_3_num_mt_1 = 0;
    backward_contains_num_mt_1 = 0;
    // 可以不用memset，用一个visit就好了
    memset(backward_3_path_header_mt_1 + begin_with, -1, sizeof(int) * (node_num - begin_with));
    // printf("started with: %d %d\n", begin_with, data_rev_mapping[begin_with]); fflush(stdout);
    dfs_path_mt_1[0] = begin_with; dfs_path_num_mt_1 = 1;
    mask_mt_1 ++; visit_mt_1[begin_with] = mask_mt_1;
    backward_dfs_mt_1();
    if (backward_3_num_mt_1 == 0) return;
    sort_out_mt_1(begin_with);
    dfs_path_mt_1[0] = begin_with; dfs_path_num_mt_1 = 1;
    mask_mt_1 ++; visit_mt_1[begin_with] = mask_mt_1;

    answer_3_header_mt_1[begin_with] = answer_3_buffer_num_mt_1;
    answer_4_header_mt_1[begin_with] = answer_4_buffer_num_mt_1;
    answer_5_header_mt_1[begin_with] = answer_5_num_mt_1;
    answer_6_header_mt_1[begin_with] = answer_6_num_mt_1;
    answer_7_header_mt_1[begin_with] = answer_7_num_mt_1;

    forward_dfs_mt_1();
    
    answer_3_tailer_mt_1[begin_with] = answer_3_buffer_num_mt_1;
    answer_4_tailer_mt_1[begin_with] = answer_4_buffer_num_mt_1;
    answer_5_tailer_mt_1[begin_with] = answer_5_num_mt_1;
    answer_6_tailer_mt_1[begin_with] = answer_6_num_mt_1;
    answer_7_tailer_mt_1[begin_with] = answer_7_num_mt_1;    
}

void* search_mt_1(void* args) {
    memcpy(edge_topo_starter_mt_1, edge_topo_header, sizeof_int_mul_node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_search_flag, 1);
        if (u >= node_num) break;
        // NOTE: this should be replaced
        global_search_assignment[u] = 1;
        if (unlikely(edge_header[u] == edge_header[u+1])) continue;
        if (unlikely(!searchable_nodes[0][u])) continue;
        do_search_mim_mt_1(u);
    }
    return NULL;
}

// search thread 1

// search thread 2

int CACHE_LINE_ALIGN visit_mt_2[MAX_NODE], mask_mt_2 = 1111;
int CACHE_LINE_ALIGN edge_topo_starter_mt_2[MAX_NODE];
int dfs_path_mt_2[5], dfs_path_num_mt_2;

char CACHE_LINE_ALIGN answer_3_mt_2[MAX_3_ANSW * ONE_INT_CHAR_SIZE * 3];
char CACHE_LINE_ALIGN answer_4_mt_2[MAX_4_ANSW * ONE_INT_CHAR_SIZE * 4];
int answer_3_buffer_num_mt_2, answer_4_buffer_num_mt_2;
int answer_3_num_mt_2 = 0, answer_4_num_mt_2 = 0;

inline void extract_3_answer_mt_2() {
    answer_3_num_mt_2 ++;
    deserialize_id(answer_3_mt_2, answer_3_buffer_num_mt_2, dfs_path_mt_2[0]);
    answer_3_mt_2[answer_3_buffer_num_mt_2 ++] = ',';
    deserialize_id(answer_3_mt_2, answer_3_buffer_num_mt_2, dfs_path_mt_2[1]);
    answer_3_mt_2[answer_3_buffer_num_mt_2 ++] = ',';
    deserialize_id(answer_3_mt_2, answer_3_buffer_num_mt_2, dfs_path_mt_2[2]);
    answer_3_mt_2[answer_3_buffer_num_mt_2 ++] = '\n';
}

inline void extract_4_answer_mt_2() {
    answer_4_num_mt_2 ++;
    deserialize_id(answer_4_mt_2, answer_4_buffer_num_mt_2, dfs_path_mt_2[0]);
    answer_4_mt_2[answer_4_buffer_num_mt_2 ++] = ',';
    deserialize_id(answer_4_mt_2, answer_4_buffer_num_mt_2, dfs_path_mt_2[1]);
    answer_4_mt_2[answer_4_buffer_num_mt_2 ++] = ',';
    deserialize_id(answer_4_mt_2, answer_4_buffer_num_mt_2, dfs_path_mt_2[2]);
    answer_4_mt_2[answer_4_buffer_num_mt_2 ++] = ',';
    deserialize_id(answer_4_mt_2, answer_4_buffer_num_mt_2, dfs_path_mt_2[3]);
    answer_4_mt_2[answer_4_buffer_num_mt_2 ++] = '\n';
}


int backward_3_path_header_mt_2[MAX_NODE];
int backward_3_path_ptr_mt_2[MAX_NODE];
int backward_3_path_value_mt_2[PATH_PAR_3][2];
int backward_3_num_mt_2 = 0;
int backward_contains_mt_2[MAX_NODE], backward_contains_num_mt_2 = 0;

inline void extract_backward_3_path_mt_2() {
    if (backward_3_path_header_mt_2[dfs_path_mt_2[3]] == -1) backward_contains_mt_2[backward_contains_num_mt_2 ++] = dfs_path_mt_2[3];
    backward_3_path_value_mt_2[backward_3_num_mt_2][0] = dfs_path_mt_2[1];
    backward_3_path_value_mt_2[backward_3_num_mt_2][1] = dfs_path_mt_2[2];
    backward_3_path_ptr_mt_2[backward_3_num_mt_2] = backward_3_path_header_mt_2[dfs_path_mt_2[3]];
    backward_3_path_header_mt_2[dfs_path_mt_2[3]] = backward_3_num_mt_2 ++;
}



void backward_dfs_mt_2() {
    int u, v, original_path_num = dfs_path_num_mt_2;
    u = dfs_path_mt_2[dfs_path_num_mt_2 - 1]; dfs_path_num_mt_2 ++;
    for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
        v = rev_edge_topo_edges[i];
        if (v <= dfs_path_mt_2[0]) break;
        if (visit_mt_2[v] == mask_mt_2) continue;
        dfs_path_mt_2[original_path_num] = v; 
        if (dfs_path_num_mt_2 == 4) {
            extract_backward_3_path_mt_2();
            continue;
        }
        visit_mt_2[v] = mask_mt_2;
        backward_dfs_mt_2();
        visit_mt_2[v] = 0;
    }
    dfs_path_num_mt_2 --;
}

int sorted_backward_3_path_mt_2[PATH_PAR_3];
int sorted_backward_3_path_fin_mt_2[PATH_PAR_3][2];
int sorted_backward_3_path_from_mt_2[MAX_NODE];
int sorted_backward_3_path_to_mt_2[MAX_NODE];
int sorted_backward_id_mt_2[PATH_PAR_3];

void sort_out_mt_2(int begin_with) {
    int i, u, v, num = 0, from, to;
    for (i=0; i<backward_contains_num_mt_2; ++i) {
        u = backward_contains_mt_2[i];
        from = sorted_backward_3_path_from_mt_2[u] = num;
        for (int j=backward_3_path_header_mt_2[u]; j!=-1; j=backward_3_path_ptr_mt_2[j]) {
            sorted_backward_3_path_mt_2[num ++] = j;
        }
        to = sorted_backward_3_path_to_mt_2[u] = num;
        std::sort(sorted_backward_3_path_mt_2 + from, sorted_backward_3_path_mt_2 + to, [](int x, int y) -> bool {
            if (backward_3_path_value_mt_2[x][1] != backward_3_path_value_mt_2[y][1]) return backward_3_path_value_mt_2[x][1] < backward_3_path_value_mt_2[y][1];
            else return backward_3_path_value_mt_2[x][0] < backward_3_path_value_mt_2[y][0];
        });
        for (int j=from; j<to; ++j) {
            sorted_backward_3_path_fin_mt_2[j][0] = backward_3_path_value_mt_2[sorted_backward_3_path_mt_2[j]][0];
            sorted_backward_3_path_fin_mt_2[j][1] = backward_3_path_value_mt_2[sorted_backward_3_path_mt_2[j]][1];
        }
    }
} 


int answer_5_mt_2[MAX_ANSW][5];
int answer_6_mt_2[MAX_ANSW][6];
int answer_7_mt_2[MAX_ANSW][7];
int answer_5_num_mt_2 = 0, answer_6_num_mt_2 = 0, answer_7_num_mt_2 = 0;

int answer_3_header_mt_2[MAX_NODE], answer_3_tailer_mt_2[MAX_NODE];
int answer_4_header_mt_2[MAX_NODE], answer_4_tailer_mt_2[MAX_NODE];
int answer_5_header_mt_2[MAX_NODE], answer_5_tailer_mt_2[MAX_NODE];
int answer_6_header_mt_2[MAX_NODE], answer_6_tailer_mt_2[MAX_NODE];
int answer_7_header_mt_2[MAX_NODE], answer_7_tailer_mt_2[MAX_NODE];

inline void extract_forward_2_path_mt_2() {
    int v = dfs_path_mt_2[2], x, y;
    for (int i=sorted_backward_3_path_from_mt_2[v]; i<sorted_backward_3_path_to_mt_2[v]; ++i) {
        x = sorted_backward_3_path_fin_mt_2[i][0];
        y = sorted_backward_3_path_fin_mt_2[i][1];
        if (visit_mt_2[x] == mask_mt_2 || visit_mt_2[y] == mask_mt_2) continue;
        memcpy(answer_5_mt_2[answer_5_num_mt_2], dfs_path_mt_2, sizeof(int) * 3);
        answer_5_mt_2[answer_5_num_mt_2][3] = y;
        answer_5_mt_2[answer_5_num_mt_2][4] = x;
        answer_5_num_mt_2 ++;
    }
}


inline void extract_forward_3_path_mt_2() {
    int v = dfs_path_mt_2[3], x, y;
    for (int i=sorted_backward_3_path_from_mt_2[v]; i<sorted_backward_3_path_to_mt_2[v]; ++i) {
        x = sorted_backward_3_path_fin_mt_2[i][0];
        y = sorted_backward_3_path_fin_mt_2[i][1];
        if (visit_mt_2[x] == mask_mt_2 || visit_mt_2[y] == mask_mt_2) continue;
        memcpy(answer_6_mt_2[answer_6_num_mt_2], dfs_path_mt_2, sizeof(int) * 4);
        answer_6_mt_2[answer_6_num_mt_2][4] = y;
        answer_6_mt_2[answer_6_num_mt_2][5] = x;
        answer_6_num_mt_2 ++;
    }
}

inline void extract_forward_4_path_mt_2() {
    int v = dfs_path_mt_2[4], x, y;
    for (int i=sorted_backward_3_path_from_mt_2[v]; i<sorted_backward_3_path_to_mt_2[v]; ++i) {
        x = sorted_backward_3_path_fin_mt_2[i][0];
        y = sorted_backward_3_path_fin_mt_2[i][1];
        if (visit_mt_2[x] == mask_mt_2 || visit_mt_2[y] == mask_mt_2) continue;
        memcpy(answer_7_mt_2[answer_7_num_mt_2], dfs_path_mt_2, sizeof(int) * 5);
        answer_7_mt_2[answer_7_num_mt_2][5] = y;
        answer_7_mt_2[answer_7_num_mt_2][6] = x;
        answer_7_num_mt_2 ++;
    }
}

void forward_dfs_mt_2() {
    int u, v, original_path_num = dfs_path_num_mt_2;
    u = dfs_path_mt_2[dfs_path_num_mt_2 - 1];  dfs_path_num_mt_2 ++;
    while (edge_topo_starter_mt_2[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_2[u]] < dfs_path_mt_2[0]) edge_topo_starter_mt_2[u] ++;
    for (int i=edge_topo_starter_mt_2[u]; i<edge_topo_header[u+1]; ++i) {
        v = edge_topo_edges[i];
        if (v == dfs_path_mt_2[0]) {
            if (original_path_num == 3) extract_3_answer_mt_2();
            if (original_path_num == 4) extract_4_answer_mt_2();
            continue;
        }
        if (visit_mt_2[v] == mask_mt_2) continue;
        dfs_path_mt_2[original_path_num] = v; 
        if (backward_3_path_header_mt_2[v] != -1) {
            switch (original_path_num)
            {
            case 2:
                extract_forward_2_path_mt_2();
                break;
            case 3:
                extract_forward_3_path_mt_2();
                break;
            case 4:
                extract_forward_4_path_mt_2();
                break;
            default:
                break;
            }
        }
        if (original_path_num == 4) continue;
        visit_mt_2[v] = mask_mt_2;
        forward_dfs_mt_2();
        visit_mt_2[v] = 0;
    }
    dfs_path_num_mt_2 --;
}

void do_search_mim_mt_2(int begin_with) {
    backward_3_num_mt_2 = 0;
    backward_contains_num_mt_2 = 0;
    // 可以不用memset，用一个visit就好了
    memset(backward_3_path_header_mt_2 + begin_with, -1, sizeof(int) * (node_num - begin_with));
    // printf("started with: %d %d\n", begin_with, data_rev_mapping[begin_with]); fflush(stdout);
    dfs_path_mt_2[0] = begin_with; dfs_path_num_mt_2 = 1;
    mask_mt_2 ++; visit_mt_2[begin_with] = mask_mt_2;
    backward_dfs_mt_2();
    if (backward_3_num_mt_2 == 0) return;
    sort_out_mt_2(begin_with);
    dfs_path_mt_2[0] = begin_with; dfs_path_num_mt_2 = 1;
    mask_mt_2 ++; visit_mt_2[begin_with] = mask_mt_2;

    answer_3_header_mt_2[begin_with] = answer_3_buffer_num_mt_2;
    answer_4_header_mt_2[begin_with] = answer_4_buffer_num_mt_2;
    answer_5_header_mt_2[begin_with] = answer_5_num_mt_2;
    answer_6_header_mt_2[begin_with] = answer_6_num_mt_2;
    answer_7_header_mt_2[begin_with] = answer_7_num_mt_2;

    forward_dfs_mt_2();
    
    answer_3_tailer_mt_2[begin_with] = answer_3_buffer_num_mt_2;
    answer_4_tailer_mt_2[begin_with] = answer_4_buffer_num_mt_2;
    answer_5_tailer_mt_2[begin_with] = answer_5_num_mt_2;
    answer_6_tailer_mt_2[begin_with] = answer_6_num_mt_2;
    answer_7_tailer_mt_2[begin_with] = answer_7_num_mt_2;    
}

void* search_mt_2(void* args) {
    memcpy(edge_topo_starter_mt_2, edge_topo_header, sizeof_int_mul_node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_search_flag, 1);
        if (u >= node_num) break;
        // NOTE: this should be replaced
        global_search_assignment[u] = 2;
        if (unlikely(edge_header[u] == edge_header[u+1])) continue;
        if (unlikely(!searchable_nodes[0][u])) continue;
        do_search_mim_mt_2(u);
    }
    return NULL;
}

// search thread 2

// search thread 3

int CACHE_LINE_ALIGN visit_mt_3[MAX_NODE], mask_mt_3 = 1111;
int CACHE_LINE_ALIGN edge_topo_starter_mt_3[MAX_NODE];
int dfs_path_mt_3[5], dfs_path_num_mt_3;

char CACHE_LINE_ALIGN answer_3_mt_3[MAX_3_ANSW * ONE_INT_CHAR_SIZE * 3];
char CACHE_LINE_ALIGN answer_4_mt_3[MAX_4_ANSW * ONE_INT_CHAR_SIZE * 4];
int answer_3_buffer_num_mt_3, answer_4_buffer_num_mt_3;
int answer_3_num_mt_3 = 0, answer_4_num_mt_3 = 0;

inline void extract_3_answer_mt_3() {
    answer_3_num_mt_3 ++;
    deserialize_id(answer_3_mt_3, answer_3_buffer_num_mt_3, dfs_path_mt_3[0]);
    answer_3_mt_3[answer_3_buffer_num_mt_3 ++] = ',';
    deserialize_id(answer_3_mt_3, answer_3_buffer_num_mt_3, dfs_path_mt_3[1]);
    answer_3_mt_3[answer_3_buffer_num_mt_3 ++] = ',';
    deserialize_id(answer_3_mt_3, answer_3_buffer_num_mt_3, dfs_path_mt_3[2]);
    answer_3_mt_3[answer_3_buffer_num_mt_3 ++] = '\n';
}

inline void extract_4_answer_mt_3() {
    answer_4_num_mt_3 ++;
    deserialize_id(answer_4_mt_3, answer_4_buffer_num_mt_3, dfs_path_mt_3[0]);
    answer_4_mt_3[answer_4_buffer_num_mt_3 ++] = ',';
    deserialize_id(answer_4_mt_3, answer_4_buffer_num_mt_3, dfs_path_mt_3[1]);
    answer_4_mt_3[answer_4_buffer_num_mt_3 ++] = ',';
    deserialize_id(answer_4_mt_3, answer_4_buffer_num_mt_3, dfs_path_mt_3[2]);
    answer_4_mt_3[answer_4_buffer_num_mt_3 ++] = ',';
    deserialize_id(answer_4_mt_3, answer_4_buffer_num_mt_3, dfs_path_mt_3[3]);
    answer_4_mt_3[answer_4_buffer_num_mt_3 ++] = '\n';
}


int backward_3_path_header_mt_3[MAX_NODE];
int backward_3_path_ptr_mt_3[MAX_NODE];
int backward_3_path_value_mt_3[PATH_PAR_3][2];
int backward_3_num_mt_3 = 0;
int backward_contains_mt_3[MAX_NODE], backward_contains_num_mt_3 = 0;

inline void extract_backward_3_path_mt_3() {
    if (backward_3_path_header_mt_3[dfs_path_mt_3[3]] == -1) backward_contains_mt_3[backward_contains_num_mt_3 ++] = dfs_path_mt_3[3];
    backward_3_path_value_mt_3[backward_3_num_mt_3][0] = dfs_path_mt_3[1];
    backward_3_path_value_mt_3[backward_3_num_mt_3][1] = dfs_path_mt_3[2];
    backward_3_path_ptr_mt_3[backward_3_num_mt_3] = backward_3_path_header_mt_3[dfs_path_mt_3[3]];
    backward_3_path_header_mt_3[dfs_path_mt_3[3]] = backward_3_num_mt_3 ++;
}



void backward_dfs_mt_3() {
    int u, v, original_path_num = dfs_path_num_mt_3;
    u = dfs_path_mt_3[dfs_path_num_mt_3 - 1]; dfs_path_num_mt_3 ++;
    for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
        v = rev_edge_topo_edges[i];
        if (v <= dfs_path_mt_3[0]) break;
        if (visit_mt_3[v] == mask_mt_3) continue;
        dfs_path_mt_3[original_path_num] = v; 
        if (dfs_path_num_mt_3 == 4) {
            extract_backward_3_path_mt_3();
            continue;
        }
        visit_mt_3[v] = mask_mt_3;
        backward_dfs_mt_3();
        visit_mt_3[v] = 0;
    }
    dfs_path_num_mt_3 --;
}

int sorted_backward_3_path_mt_3[PATH_PAR_3];
int sorted_backward_3_path_fin_mt_3[PATH_PAR_3][2];
int sorted_backward_3_path_from_mt_3[MAX_NODE];
int sorted_backward_3_path_to_mt_3[MAX_NODE];
int sorted_backward_id_mt_3[PATH_PAR_3];

void sort_out_mt_3(int begin_with) {
    int i, u, v, num = 0, from, to;
    for (i=0; i<backward_contains_num_mt_3; ++i) {
        u = backward_contains_mt_3[i];
        from = sorted_backward_3_path_from_mt_3[u] = num;
        for (int j=backward_3_path_header_mt_3[u]; j!=-1; j=backward_3_path_ptr_mt_3[j]) {
            sorted_backward_3_path_mt_3[num ++] = j;
        }
        to = sorted_backward_3_path_to_mt_3[u] = num;
        std::sort(sorted_backward_3_path_mt_3 + from, sorted_backward_3_path_mt_3 + to, [](int x, int y) -> bool {
            if (backward_3_path_value_mt_3[x][1] != backward_3_path_value_mt_3[y][1]) return backward_3_path_value_mt_3[x][1] < backward_3_path_value_mt_3[y][1];
            else return backward_3_path_value_mt_3[x][0] < backward_3_path_value_mt_3[y][0];
        });
        for (int j=from; j<to; ++j) {
            sorted_backward_3_path_fin_mt_3[j][0] = backward_3_path_value_mt_3[sorted_backward_3_path_mt_3[j]][0];
            sorted_backward_3_path_fin_mt_3[j][1] = backward_3_path_value_mt_3[sorted_backward_3_path_mt_3[j]][1];
        }
    }
} 


int answer_5_mt_3[MAX_ANSW][5];
int answer_6_mt_3[MAX_ANSW][6];
int answer_7_mt_3[MAX_ANSW][7];
int answer_5_num_mt_3 = 0, answer_6_num_mt_3 = 0, answer_7_num_mt_3 = 0;

int answer_3_header_mt_3[MAX_NODE], answer_3_tailer_mt_3[MAX_NODE];
int answer_4_header_mt_3[MAX_NODE], answer_4_tailer_mt_3[MAX_NODE];
int answer_5_header_mt_3[MAX_NODE], answer_5_tailer_mt_3[MAX_NODE];
int answer_6_header_mt_3[MAX_NODE], answer_6_tailer_mt_3[MAX_NODE];
int answer_7_header_mt_3[MAX_NODE], answer_7_tailer_mt_3[MAX_NODE];

inline void extract_forward_2_path_mt_3() {
    int v = dfs_path_mt_3[2], x, y;
    for (int i=sorted_backward_3_path_from_mt_3[v]; i<sorted_backward_3_path_to_mt_3[v]; ++i) {
        x = sorted_backward_3_path_fin_mt_3[i][0];
        y = sorted_backward_3_path_fin_mt_3[i][1];
        if (visit_mt_3[x] == mask_mt_3 || visit_mt_3[y] == mask_mt_3) continue;
        memcpy(answer_5_mt_3[answer_5_num_mt_3], dfs_path_mt_3, sizeof(int) * 3);
        answer_5_mt_3[answer_5_num_mt_3][3] = y;
        answer_5_mt_3[answer_5_num_mt_3][4] = x;
        answer_5_num_mt_3 ++;
    }
}


inline void extract_forward_3_path_mt_3() {
    int v = dfs_path_mt_3[3], x, y;
    for (int i=sorted_backward_3_path_from_mt_3[v]; i<sorted_backward_3_path_to_mt_3[v]; ++i) {
        x = sorted_backward_3_path_fin_mt_3[i][0];
        y = sorted_backward_3_path_fin_mt_3[i][1];
        if (visit_mt_3[x] == mask_mt_3 || visit_mt_3[y] == mask_mt_3) continue;
        memcpy(answer_6_mt_3[answer_6_num_mt_3], dfs_path_mt_3, sizeof(int) * 4);
        answer_6_mt_3[answer_6_num_mt_3][4] = y;
        answer_6_mt_3[answer_6_num_mt_3][5] = x;
        answer_6_num_mt_3 ++;
    }
}

inline void extract_forward_4_path_mt_3() {
    int v = dfs_path_mt_3[4], x, y;
    for (int i=sorted_backward_3_path_from_mt_3[v]; i<sorted_backward_3_path_to_mt_3[v]; ++i) {
        x = sorted_backward_3_path_fin_mt_3[i][0];
        y = sorted_backward_3_path_fin_mt_3[i][1];
        if (visit_mt_3[x] == mask_mt_3 || visit_mt_3[y] == mask_mt_3) continue;
        memcpy(answer_7_mt_3[answer_7_num_mt_3], dfs_path_mt_3, sizeof(int) * 5);
        answer_7_mt_3[answer_7_num_mt_3][5] = y;
        answer_7_mt_3[answer_7_num_mt_3][6] = x;
        answer_7_num_mt_3 ++;
    }
}

void forward_dfs_mt_3() {
    int u, v, original_path_num = dfs_path_num_mt_3;
    u = dfs_path_mt_3[dfs_path_num_mt_3 - 1];  dfs_path_num_mt_3 ++;
    while (edge_topo_starter_mt_3[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_3[u]] < dfs_path_mt_3[0]) edge_topo_starter_mt_3[u] ++;
    for (int i=edge_topo_starter_mt_3[u]; i<edge_topo_header[u+1]; ++i) {
        v = edge_topo_edges[i];
        if (v == dfs_path_mt_3[0]) {
            if (original_path_num == 3) extract_3_answer_mt_3();
            if (original_path_num == 4) extract_4_answer_mt_3();
            continue;
        }
        if (visit_mt_3[v] == mask_mt_3) continue;
        dfs_path_mt_3[original_path_num] = v; 
        if (backward_3_path_header_mt_3[v] != -1) {
            switch (original_path_num)
            {
            case 2:
                extract_forward_2_path_mt_3();
                break;
            case 3:
                extract_forward_3_path_mt_3();
                break;
            case 4:
                extract_forward_4_path_mt_3();
                break;
            default:
                break;
            }
        }
        if (original_path_num == 4) continue;
        visit_mt_3[v] = mask_mt_3;
        forward_dfs_mt_3();
        visit_mt_3[v] = 0;
    }
    dfs_path_num_mt_3 --;
}

void do_search_mim_mt_3(int begin_with) {
    backward_3_num_mt_3 = 0;
    backward_contains_num_mt_3 = 0;
    // 可以不用memset，用一个visit就好了
    memset(backward_3_path_header_mt_3 + begin_with, -1, sizeof(int) * (node_num - begin_with));
    // printf("started with: %d %d\n", begin_with, data_rev_mapping[begin_with]); fflush(stdout);
    dfs_path_mt_3[0] = begin_with; dfs_path_num_mt_3 = 1;
    mask_mt_3 ++; visit_mt_3[begin_with] = mask_mt_3;
    backward_dfs_mt_3();
    if (backward_3_num_mt_3 == 0) return;
    sort_out_mt_3(begin_with);
    dfs_path_mt_3[0] = begin_with; dfs_path_num_mt_3 = 1;
    mask_mt_3 ++; visit_mt_3[begin_with] = mask_mt_3;

    answer_3_header_mt_3[begin_with] = answer_3_buffer_num_mt_3;
    answer_4_header_mt_3[begin_with] = answer_4_buffer_num_mt_3;
    answer_5_header_mt_3[begin_with] = answer_5_num_mt_3;
    answer_6_header_mt_3[begin_with] = answer_6_num_mt_3;
    answer_7_header_mt_3[begin_with] = answer_7_num_mt_3;

    forward_dfs_mt_3();
    
    answer_3_tailer_mt_3[begin_with] = answer_3_buffer_num_mt_3;
    answer_4_tailer_mt_3[begin_with] = answer_4_buffer_num_mt_3;
    answer_5_tailer_mt_3[begin_with] = answer_5_num_mt_3;
    answer_6_tailer_mt_3[begin_with] = answer_6_num_mt_3;
    answer_7_tailer_mt_3[begin_with] = answer_7_num_mt_3;    
}

void* search_mt_3(void* args) {
    memcpy(edge_topo_starter_mt_3, edge_topo_header, sizeof_int_mul_node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_search_flag, 1);
        if (u >= node_num) break;
        // NOTE: this should be replaced
        global_search_assignment[u] = 3;
        if (unlikely(edge_header[u] == edge_header[u+1])) continue;
        if (unlikely(!searchable_nodes[0][u])) continue;
        do_search_mim_mt_3(u);
    }
    return NULL;
}

// search thread 3

// search thread 4

int CACHE_LINE_ALIGN visit_mt_4[MAX_NODE], mask_mt_4 = 1111;
int CACHE_LINE_ALIGN edge_topo_starter_mt_4[MAX_NODE];
int dfs_path_mt_4[5], dfs_path_num_mt_4;

char CACHE_LINE_ALIGN answer_3_mt_4[MAX_3_ANSW * ONE_INT_CHAR_SIZE * 3];
char CACHE_LINE_ALIGN answer_4_mt_4[MAX_4_ANSW * ONE_INT_CHAR_SIZE * 4];
int answer_3_buffer_num_mt_4, answer_4_buffer_num_mt_4;
int answer_3_num_mt_4 = 0, answer_4_num_mt_4 = 0;

inline void extract_3_answer_mt_4() {
    answer_3_num_mt_4 ++;
    deserialize_id(answer_3_mt_4, answer_3_buffer_num_mt_4, dfs_path_mt_4[0]);
    answer_3_mt_4[answer_3_buffer_num_mt_4 ++] = ',';
    deserialize_id(answer_3_mt_4, answer_3_buffer_num_mt_4, dfs_path_mt_4[1]);
    answer_3_mt_4[answer_3_buffer_num_mt_4 ++] = ',';
    deserialize_id(answer_3_mt_4, answer_3_buffer_num_mt_4, dfs_path_mt_4[2]);
    answer_3_mt_4[answer_3_buffer_num_mt_4 ++] = '\n';
}

inline void extract_4_answer_mt_4() {
    answer_4_num_mt_4 ++;
    deserialize_id(answer_4_mt_4, answer_4_buffer_num_mt_4, dfs_path_mt_4[0]);
    answer_4_mt_4[answer_4_buffer_num_mt_4 ++] = ',';
    deserialize_id(answer_4_mt_4, answer_4_buffer_num_mt_4, dfs_path_mt_4[1]);
    answer_4_mt_4[answer_4_buffer_num_mt_4 ++] = ',';
    deserialize_id(answer_4_mt_4, answer_4_buffer_num_mt_4, dfs_path_mt_4[2]);
    answer_4_mt_4[answer_4_buffer_num_mt_4 ++] = ',';
    deserialize_id(answer_4_mt_4, answer_4_buffer_num_mt_4, dfs_path_mt_4[3]);
    answer_4_mt_4[answer_4_buffer_num_mt_4 ++] = '\n';
}


int backward_3_path_header_mt_4[MAX_NODE];
int backward_3_path_ptr_mt_4[MAX_NODE];
int backward_3_path_value_mt_4[PATH_PAR_3][2];
int backward_3_num_mt_4 = 0;
int backward_contains_mt_4[MAX_NODE], backward_contains_num_mt_4 = 0;

inline void extract_backward_3_path_mt_4() {
    if (backward_3_path_header_mt_4[dfs_path_mt_4[3]] == -1) backward_contains_mt_4[backward_contains_num_mt_4 ++] = dfs_path_mt_4[3];
    backward_3_path_value_mt_4[backward_3_num_mt_4][0] = dfs_path_mt_4[1];
    backward_3_path_value_mt_4[backward_3_num_mt_4][1] = dfs_path_mt_4[2];
    backward_3_path_ptr_mt_4[backward_3_num_mt_4] = backward_3_path_header_mt_4[dfs_path_mt_4[3]];
    backward_3_path_header_mt_4[dfs_path_mt_4[3]] = backward_3_num_mt_4 ++;
}



void backward_dfs_mt_4() {
    int u, v, original_path_num = dfs_path_num_mt_4;
    u = dfs_path_mt_4[dfs_path_num_mt_4 - 1]; dfs_path_num_mt_4 ++;
    for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
        v = rev_edge_topo_edges[i];
        if (v <= dfs_path_mt_4[0]) break;
        if (visit_mt_4[v] == mask_mt_4) continue;
        dfs_path_mt_4[original_path_num] = v; 
        if (dfs_path_num_mt_4 == 4) {
            extract_backward_3_path_mt_4();
            continue;
        }
        visit_mt_4[v] = mask_mt_4;
        backward_dfs_mt_4();
        visit_mt_4[v] = 0;
    }
    dfs_path_num_mt_4 --;
}

int sorted_backward_3_path_mt_4[PATH_PAR_3];
int sorted_backward_3_path_fin_mt_4[PATH_PAR_3][2];
int sorted_backward_3_path_from_mt_4[MAX_NODE];
int sorted_backward_3_path_to_mt_4[MAX_NODE];
int sorted_backward_id_mt_4[PATH_PAR_3];

void sort_out_mt_4(int begin_with) {
    int i, u, v, num = 0, from, to;
    for (i=0; i<backward_contains_num_mt_4; ++i) {
        u = backward_contains_mt_4[i];
        from = sorted_backward_3_path_from_mt_4[u] = num;
        for (int j=backward_3_path_header_mt_4[u]; j!=-1; j=backward_3_path_ptr_mt_4[j]) {
            sorted_backward_3_path_mt_4[num ++] = j;
        }
        to = sorted_backward_3_path_to_mt_4[u] = num;
        std::sort(sorted_backward_3_path_mt_4 + from, sorted_backward_3_path_mt_4 + to, [](int x, int y) -> bool {
            if (backward_3_path_value_mt_4[x][1] != backward_3_path_value_mt_4[y][1]) return backward_3_path_value_mt_4[x][1] < backward_3_path_value_mt_4[y][1];
            else return backward_3_path_value_mt_4[x][0] < backward_3_path_value_mt_4[y][0];
        });
        for (int j=from; j<to; ++j) {
            sorted_backward_3_path_fin_mt_4[j][0] = backward_3_path_value_mt_4[sorted_backward_3_path_mt_4[j]][0];
            sorted_backward_3_path_fin_mt_4[j][1] = backward_3_path_value_mt_4[sorted_backward_3_path_mt_4[j]][1];
        }
    }
} 


int answer_5_mt_4[MAX_ANSW][5];
int answer_6_mt_4[MAX_ANSW][6];
int answer_7_mt_4[MAX_ANSW][7];
int answer_5_num_mt_4 = 0, answer_6_num_mt_4 = 0, answer_7_num_mt_4 = 0;

int answer_3_header_mt_4[MAX_NODE], answer_3_tailer_mt_4[MAX_NODE];
int answer_4_header_mt_4[MAX_NODE], answer_4_tailer_mt_4[MAX_NODE];
int answer_5_header_mt_4[MAX_NODE], answer_5_tailer_mt_4[MAX_NODE];
int answer_6_header_mt_4[MAX_NODE], answer_6_tailer_mt_4[MAX_NODE];
int answer_7_header_mt_4[MAX_NODE], answer_7_tailer_mt_4[MAX_NODE];

inline void extract_forward_2_path_mt_4() {
    int v = dfs_path_mt_4[2], x, y;
    for (int i=sorted_backward_3_path_from_mt_4[v]; i<sorted_backward_3_path_to_mt_4[v]; ++i) {
        x = sorted_backward_3_path_fin_mt_4[i][0];
        y = sorted_backward_3_path_fin_mt_4[i][1];
        if (visit_mt_4[x] == mask_mt_4 || visit_mt_4[y] == mask_mt_4) continue;
        memcpy(answer_5_mt_4[answer_5_num_mt_4], dfs_path_mt_4, sizeof(int) * 3);
        answer_5_mt_4[answer_5_num_mt_4][3] = y;
        answer_5_mt_4[answer_5_num_mt_4][4] = x;
        answer_5_num_mt_4 ++;
    }
}


inline void extract_forward_3_path_mt_4() {
    int v = dfs_path_mt_4[3], x, y;
    for (int i=sorted_backward_3_path_from_mt_4[v]; i<sorted_backward_3_path_to_mt_4[v]; ++i) {
        x = sorted_backward_3_path_fin_mt_4[i][0];
        y = sorted_backward_3_path_fin_mt_4[i][1];
        if (visit_mt_4[x] == mask_mt_4 || visit_mt_4[y] == mask_mt_4) continue;
        memcpy(answer_6_mt_4[answer_6_num_mt_4], dfs_path_mt_4, sizeof(int) * 4);
        answer_6_mt_4[answer_6_num_mt_4][4] = y;
        answer_6_mt_4[answer_6_num_mt_4][5] = x;
        answer_6_num_mt_4 ++;
    }
}

inline void extract_forward_4_path_mt_4() {
    int v = dfs_path_mt_4[4], x, y;
    for (int i=sorted_backward_3_path_from_mt_4[v]; i<sorted_backward_3_path_to_mt_4[v]; ++i) {
        x = sorted_backward_3_path_fin_mt_4[i][0];
        y = sorted_backward_3_path_fin_mt_4[i][1];
        if (visit_mt_4[x] == mask_mt_4 || visit_mt_4[y] == mask_mt_4) continue;
        memcpy(answer_7_mt_4[answer_7_num_mt_4], dfs_path_mt_4, sizeof(int) * 5);
        answer_7_mt_4[answer_7_num_mt_4][5] = y;
        answer_7_mt_4[answer_7_num_mt_4][6] = x;
        answer_7_num_mt_4 ++;
    }
}

void forward_dfs_mt_4() {
    int u, v, original_path_num = dfs_path_num_mt_4;
    u = dfs_path_mt_4[dfs_path_num_mt_4 - 1];  dfs_path_num_mt_4 ++;
    while (edge_topo_starter_mt_4[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_4[u]] < dfs_path_mt_4[0]) edge_topo_starter_mt_4[u] ++;
    for (int i=edge_topo_starter_mt_4[u]; i<edge_topo_header[u+1]; ++i) {
        v = edge_topo_edges[i];
        if (v == dfs_path_mt_4[0]) {
            if (original_path_num == 3) extract_3_answer_mt_4();
            if (original_path_num == 4) extract_4_answer_mt_4();
            continue;
        }
        if (visit_mt_4[v] == mask_mt_4) continue;
        dfs_path_mt_4[original_path_num] = v; 
        if (backward_3_path_header_mt_4[v] != -1) {
            switch (original_path_num)
            {
            case 2:
                extract_forward_2_path_mt_4();
                break;
            case 3:
                extract_forward_3_path_mt_4();
                break;
            case 4:
                extract_forward_4_path_mt_4();
                break;
            default:
                break;
            }
        }
        if (original_path_num == 4) continue;
        visit_mt_4[v] = mask_mt_4;
        forward_dfs_mt_4();
        visit_mt_4[v] = 0;
    }
    dfs_path_num_mt_4 --;
}

void do_search_mim_mt_4(int begin_with) {
    backward_3_num_mt_4 = 0;
    backward_contains_num_mt_4 = 0;
    // 可以不用memset，用一个visit就好了
    memset(backward_3_path_header_mt_4 + begin_with, -1, sizeof(int) * (node_num - begin_with));
    // printf("started with: %d %d\n", begin_with, data_rev_mapping[begin_with]); fflush(stdout);
    dfs_path_mt_4[0] = begin_with; dfs_path_num_mt_4 = 1;
    mask_mt_4 ++; visit_mt_4[begin_with] = mask_mt_4;
    backward_dfs_mt_4();
    if (backward_3_num_mt_4 == 0) return;
    sort_out_mt_4(begin_with);
    dfs_path_mt_4[0] = begin_with; dfs_path_num_mt_4 = 1;
    mask_mt_4 ++; visit_mt_4[begin_with] = mask_mt_4;

    answer_3_header_mt_4[begin_with] = answer_3_buffer_num_mt_4;
    answer_4_header_mt_4[begin_with] = answer_4_buffer_num_mt_4;
    answer_5_header_mt_4[begin_with] = answer_5_num_mt_4;
    answer_6_header_mt_4[begin_with] = answer_6_num_mt_4;
    answer_7_header_mt_4[begin_with] = answer_7_num_mt_4;

    forward_dfs_mt_4();
    
    answer_3_tailer_mt_4[begin_with] = answer_3_buffer_num_mt_4;
    answer_4_tailer_mt_4[begin_with] = answer_4_buffer_num_mt_4;
    answer_5_tailer_mt_4[begin_with] = answer_5_num_mt_4;
    answer_6_tailer_mt_4[begin_with] = answer_6_num_mt_4;
    answer_7_tailer_mt_4[begin_with] = answer_7_num_mt_4;    
}

void* search_mt_4(void* args) {
    memcpy(edge_topo_starter_mt_4, edge_topo_header, sizeof_int_mul_node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_search_flag, 1);
        if (u >= node_num) break;
        // NOTE: this should be replaced
        global_search_assignment[u] = 4;
        if (unlikely(edge_header[u] == edge_header[u+1])) continue;
        if (unlikely(!searchable_nodes[0][u])) continue;
        do_search_mim_mt_4(u);
    }
    return NULL;
}

// search thread 4

// search thread 5

int CACHE_LINE_ALIGN visit_mt_5[MAX_NODE], mask_mt_5 = 1111;
int CACHE_LINE_ALIGN edge_topo_starter_mt_5[MAX_NODE];
int dfs_path_mt_5[5], dfs_path_num_mt_5;

char CACHE_LINE_ALIGN answer_3_mt_5[MAX_3_ANSW * ONE_INT_CHAR_SIZE * 3];
char CACHE_LINE_ALIGN answer_4_mt_5[MAX_4_ANSW * ONE_INT_CHAR_SIZE * 4];
int answer_3_buffer_num_mt_5, answer_4_buffer_num_mt_5;
int answer_3_num_mt_5 = 0, answer_4_num_mt_5 = 0;

inline void extract_3_answer_mt_5() {
    answer_3_num_mt_5 ++;
    deserialize_id(answer_3_mt_5, answer_3_buffer_num_mt_5, dfs_path_mt_5[0]);
    answer_3_mt_5[answer_3_buffer_num_mt_5 ++] = ',';
    deserialize_id(answer_3_mt_5, answer_3_buffer_num_mt_5, dfs_path_mt_5[1]);
    answer_3_mt_5[answer_3_buffer_num_mt_5 ++] = ',';
    deserialize_id(answer_3_mt_5, answer_3_buffer_num_mt_5, dfs_path_mt_5[2]);
    answer_3_mt_5[answer_3_buffer_num_mt_5 ++] = '\n';
}

inline void extract_4_answer_mt_5() {
    answer_4_num_mt_5 ++;
    deserialize_id(answer_4_mt_5, answer_4_buffer_num_mt_5, dfs_path_mt_5[0]);
    answer_4_mt_5[answer_4_buffer_num_mt_5 ++] = ',';
    deserialize_id(answer_4_mt_5, answer_4_buffer_num_mt_5, dfs_path_mt_5[1]);
    answer_4_mt_5[answer_4_buffer_num_mt_5 ++] = ',';
    deserialize_id(answer_4_mt_5, answer_4_buffer_num_mt_5, dfs_path_mt_5[2]);
    answer_4_mt_5[answer_4_buffer_num_mt_5 ++] = ',';
    deserialize_id(answer_4_mt_5, answer_4_buffer_num_mt_5, dfs_path_mt_5[3]);
    answer_4_mt_5[answer_4_buffer_num_mt_5 ++] = '\n';
}


int backward_3_path_header_mt_5[MAX_NODE];
int backward_3_path_ptr_mt_5[MAX_NODE];
int backward_3_path_value_mt_5[PATH_PAR_3][2];
int backward_3_num_mt_5 = 0;
int backward_contains_mt_5[MAX_NODE], backward_contains_num_mt_5 = 0;

inline void extract_backward_3_path_mt_5() {
    if (backward_3_path_header_mt_5[dfs_path_mt_5[3]] == -1) backward_contains_mt_5[backward_contains_num_mt_5 ++] = dfs_path_mt_5[3];
    backward_3_path_value_mt_5[backward_3_num_mt_5][0] = dfs_path_mt_5[1];
    backward_3_path_value_mt_5[backward_3_num_mt_5][1] = dfs_path_mt_5[2];
    backward_3_path_ptr_mt_5[backward_3_num_mt_5] = backward_3_path_header_mt_5[dfs_path_mt_5[3]];
    backward_3_path_header_mt_5[dfs_path_mt_5[3]] = backward_3_num_mt_5 ++;
}



void backward_dfs_mt_5() {
    int u, v, original_path_num = dfs_path_num_mt_5;
    u = dfs_path_mt_5[dfs_path_num_mt_5 - 1]; dfs_path_num_mt_5 ++;
    for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
        v = rev_edge_topo_edges[i];
        if (v <= dfs_path_mt_5[0]) break;
        if (visit_mt_5[v] == mask_mt_5) continue;
        dfs_path_mt_5[original_path_num] = v; 
        if (dfs_path_num_mt_5 == 4) {
            extract_backward_3_path_mt_5();
            continue;
        }
        visit_mt_5[v] = mask_mt_5;
        backward_dfs_mt_5();
        visit_mt_5[v] = 0;
    }
    dfs_path_num_mt_5 --;
}

int sorted_backward_3_path_mt_5[PATH_PAR_3];
int sorted_backward_3_path_fin_mt_5[PATH_PAR_3][2];
int sorted_backward_3_path_from_mt_5[MAX_NODE];
int sorted_backward_3_path_to_mt_5[MAX_NODE];
int sorted_backward_id_mt_5[PATH_PAR_3];

void sort_out_mt_5(int begin_with) {
    int i, u, v, num = 0, from, to;
    for (i=0; i<backward_contains_num_mt_5; ++i) {
        u = backward_contains_mt_5[i];
        from = sorted_backward_3_path_from_mt_5[u] = num;
        for (int j=backward_3_path_header_mt_5[u]; j!=-1; j=backward_3_path_ptr_mt_5[j]) {
            sorted_backward_3_path_mt_5[num ++] = j;
        }
        to = sorted_backward_3_path_to_mt_5[u] = num;
        std::sort(sorted_backward_3_path_mt_5 + from, sorted_backward_3_path_mt_5 + to, [](int x, int y) -> bool {
            if (backward_3_path_value_mt_5[x][1] != backward_3_path_value_mt_5[y][1]) return backward_3_path_value_mt_5[x][1] < backward_3_path_value_mt_5[y][1];
            else return backward_3_path_value_mt_5[x][0] < backward_3_path_value_mt_5[y][0];
        });
        for (int j=from; j<to; ++j) {
            sorted_backward_3_path_fin_mt_5[j][0] = backward_3_path_value_mt_5[sorted_backward_3_path_mt_5[j]][0];
            sorted_backward_3_path_fin_mt_5[j][1] = backward_3_path_value_mt_5[sorted_backward_3_path_mt_5[j]][1];
        }
    }
} 


int answer_5_mt_5[MAX_ANSW][5];
int answer_6_mt_5[MAX_ANSW][6];
int answer_7_mt_5[MAX_ANSW][7];
int answer_5_num_mt_5 = 0, answer_6_num_mt_5 = 0, answer_7_num_mt_5 = 0;

int answer_3_header_mt_5[MAX_NODE], answer_3_tailer_mt_5[MAX_NODE];
int answer_4_header_mt_5[MAX_NODE], answer_4_tailer_mt_5[MAX_NODE];
int answer_5_header_mt_5[MAX_NODE], answer_5_tailer_mt_5[MAX_NODE];
int answer_6_header_mt_5[MAX_NODE], answer_6_tailer_mt_5[MAX_NODE];
int answer_7_header_mt_5[MAX_NODE], answer_7_tailer_mt_5[MAX_NODE];

inline void extract_forward_2_path_mt_5() {
    int v = dfs_path_mt_5[2], x, y;
    for (int i=sorted_backward_3_path_from_mt_5[v]; i<sorted_backward_3_path_to_mt_5[v]; ++i) {
        x = sorted_backward_3_path_fin_mt_5[i][0];
        y = sorted_backward_3_path_fin_mt_5[i][1];
        if (visit_mt_5[x] == mask_mt_5 || visit_mt_5[y] == mask_mt_5) continue;
        memcpy(answer_5_mt_5[answer_5_num_mt_5], dfs_path_mt_5, sizeof(int) * 3);
        answer_5_mt_5[answer_5_num_mt_5][3] = y;
        answer_5_mt_5[answer_5_num_mt_5][4] = x;
        answer_5_num_mt_5 ++;
    }
}


inline void extract_forward_3_path_mt_5() {
    int v = dfs_path_mt_5[3], x, y;
    for (int i=sorted_backward_3_path_from_mt_5[v]; i<sorted_backward_3_path_to_mt_5[v]; ++i) {
        x = sorted_backward_3_path_fin_mt_5[i][0];
        y = sorted_backward_3_path_fin_mt_5[i][1];
        if (visit_mt_5[x] == mask_mt_5 || visit_mt_5[y] == mask_mt_5) continue;
        memcpy(answer_6_mt_5[answer_6_num_mt_5], dfs_path_mt_5, sizeof(int) * 4);
        answer_6_mt_5[answer_6_num_mt_5][4] = y;
        answer_6_mt_5[answer_6_num_mt_5][5] = x;
        answer_6_num_mt_5 ++;
    }
}

inline void extract_forward_4_path_mt_5() {
    int v = dfs_path_mt_5[4], x, y;
    for (int i=sorted_backward_3_path_from_mt_5[v]; i<sorted_backward_3_path_to_mt_5[v]; ++i) {
        x = sorted_backward_3_path_fin_mt_5[i][0];
        y = sorted_backward_3_path_fin_mt_5[i][1];
        if (visit_mt_5[x] == mask_mt_5 || visit_mt_5[y] == mask_mt_5) continue;
        memcpy(answer_7_mt_5[answer_7_num_mt_5], dfs_path_mt_5, sizeof(int) * 5);
        answer_7_mt_5[answer_7_num_mt_5][5] = y;
        answer_7_mt_5[answer_7_num_mt_5][6] = x;
        answer_7_num_mt_5 ++;
    }
}

void forward_dfs_mt_5() {
    int u, v, original_path_num = dfs_path_num_mt_5;
    u = dfs_path_mt_5[dfs_path_num_mt_5 - 1];  dfs_path_num_mt_5 ++;
    while (edge_topo_starter_mt_5[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_5[u]] < dfs_path_mt_5[0]) edge_topo_starter_mt_5[u] ++;
    for (int i=edge_topo_starter_mt_5[u]; i<edge_topo_header[u+1]; ++i) {
        v = edge_topo_edges[i];
        if (v == dfs_path_mt_5[0]) {
            if (original_path_num == 3) extract_3_answer_mt_5();
            if (original_path_num == 4) extract_4_answer_mt_5();
            continue;
        }
        if (visit_mt_5[v] == mask_mt_5) continue;
        dfs_path_mt_5[original_path_num] = v; 
        if (backward_3_path_header_mt_5[v] != -1) {
            switch (original_path_num)
            {
            case 2:
                extract_forward_2_path_mt_5();
                break;
            case 3:
                extract_forward_3_path_mt_5();
                break;
            case 4:
                extract_forward_4_path_mt_5();
                break;
            default:
                break;
            }
        }
        if (original_path_num == 4) continue;
        visit_mt_5[v] = mask_mt_5;
        forward_dfs_mt_5();
        visit_mt_5[v] = 0;
    }
    dfs_path_num_mt_5 --;
}

void do_search_mim_mt_5(int begin_with) {
    backward_3_num_mt_5 = 0;
    backward_contains_num_mt_5 = 0;
    // 可以不用memset，用一个visit就好了
    memset(backward_3_path_header_mt_5 + begin_with, -1, sizeof(int) * (node_num - begin_with));
    // printf("started with: %d %d\n", begin_with, data_rev_mapping[begin_with]); fflush(stdout);
    dfs_path_mt_5[0] = begin_with; dfs_path_num_mt_5 = 1;
    mask_mt_5 ++; visit_mt_5[begin_with] = mask_mt_5;
    backward_dfs_mt_5();
    if (backward_3_num_mt_5 == 0) return;
    sort_out_mt_5(begin_with);
    dfs_path_mt_5[0] = begin_with; dfs_path_num_mt_5 = 1;
    mask_mt_5 ++; visit_mt_5[begin_with] = mask_mt_5;

    answer_3_header_mt_5[begin_with] = answer_3_buffer_num_mt_5;
    answer_4_header_mt_5[begin_with] = answer_4_buffer_num_mt_5;
    answer_5_header_mt_5[begin_with] = answer_5_num_mt_5;
    answer_6_header_mt_5[begin_with] = answer_6_num_mt_5;
    answer_7_header_mt_5[begin_with] = answer_7_num_mt_5;

    forward_dfs_mt_5();
    
    answer_3_tailer_mt_5[begin_with] = answer_3_buffer_num_mt_5;
    answer_4_tailer_mt_5[begin_with] = answer_4_buffer_num_mt_5;
    answer_5_tailer_mt_5[begin_with] = answer_5_num_mt_5;
    answer_6_tailer_mt_5[begin_with] = answer_6_num_mt_5;
    answer_7_tailer_mt_5[begin_with] = answer_7_num_mt_5;    
}

void* search_mt_5(void* args) {
    memcpy(edge_topo_starter_mt_5, edge_topo_header, sizeof_int_mul_node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_search_flag, 1);
        if (u >= node_num) break;
        // NOTE: this should be replaced
        global_search_assignment[u] = 5;
        if (unlikely(edge_header[u] == edge_header[u+1])) continue;
        if (unlikely(!searchable_nodes[0][u])) continue;
        do_search_mim_mt_5(u);
    }
    return NULL;
}

// search thread 5


// ********************************************************* IO THREAD
// ********************************************************* IO THREAD
// ********************************************************* IO THREAD
// ********************************************************* IO THREAD
// ********************************************************* IO THREAD
// ********************************************************* IO THREAD
// ********************************************************* IO THREAD
// ********************************************************* IO THREAD
// ********************************************************* IO THREAD



// io 0
char output_buffer_io_0[MAX_ANSW * ONE_INT_CHAR_SIZE * 3];
int output_buffer_num_io_0 = 0;
void* write_to_disk_io_0(void* args) {
    int size;
    deserialize_int(output_buffer_io_0, output_buffer_num_io_0, answer_num);
    output_buffer_io_0[output_buffer_num_io_0 ++] = '\n';

    for (int i=0; i<node_num; ++i) {
        switch (global_search_assignment[i])
        {
        case 0:
            size = answer_3_tailer_mt_0[i] - answer_3_header_mt_0[i];
            memcpy(output_buffer_io_0 + output_buffer_num_io_0, answer_3_mt_0 + answer_3_header_mt_0[i], size << 2);
            output_buffer_num_io_0 += size;
            break;
        case 1:
            size = answer_3_tailer_mt_1[i] - answer_3_header_mt_1[i];
            memcpy(output_buffer_io_0 + output_buffer_num_io_0, answer_3_mt_1 + answer_3_header_mt_1[i], size << 2);
            output_buffer_num_io_0 += size;
            break;
        case 2:
            size = answer_3_tailer_mt_2[i] - answer_3_header_mt_2[i];
            memcpy(output_buffer_io_0 + output_buffer_num_io_0, answer_3_mt_2 + answer_3_header_mt_2[i], size << 2);
            output_buffer_num_io_0 += size;
            break;
        case 3:
            size = answer_3_tailer_mt_3[i] - answer_3_header_mt_3[i];
            memcpy(output_buffer_io_0 + output_buffer_num_io_0, answer_3_mt_3 + answer_3_header_mt_3[i], size << 2);
            output_buffer_num_io_0 += size;
            break;
        case 4:
            size = answer_3_tailer_mt_4[i] - answer_3_header_mt_4[i];
            memcpy(output_buffer_io_0 + output_buffer_num_io_0, answer_3_mt_4 + answer_3_header_mt_4[i], size << 2);
            output_buffer_num_io_0 += size;
            break;
        case 5:
            size = answer_3_tailer_mt_5[i] - answer_3_header_mt_5[i];
            memcpy(output_buffer_io_0 + output_buffer_num_io_0, answer_3_mt_5 + answer_3_header_mt_5[i], size << 2);
            output_buffer_num_io_0 += size;
            break;
        default:
            break;
        }
    }

    for (int i=0; i<node_num; ++i) {
        switch (global_search_assignment[i])
        {
        case 0:
            size = answer_4_tailer_mt_0[i] - answer_4_header_mt_0[i];
            memcpy(output_buffer_io_0 + output_buffer_num_io_0, answer_4_mt_0 + answer_4_header_mt_0[i], size << 2);
            output_buffer_num_io_0 += size;
            break;
        case 1:
            size = answer_4_tailer_mt_1[i] - answer_4_header_mt_1[i];
            memcpy(output_buffer_io_0 + output_buffer_num_io_0, answer_4_mt_1 + answer_4_header_mt_1[i], size << 2);
            output_buffer_num_io_0 += size;
            break;
        case 2:
            size = answer_4_tailer_mt_2[i] - answer_4_header_mt_2[i];
            memcpy(output_buffer_io_0 + output_buffer_num_io_0, answer_4_mt_2 + answer_4_header_mt_2[i], size << 2);
            output_buffer_num_io_0 += size;
            break;
        case 3:
            size = answer_4_tailer_mt_3[i] - answer_4_header_mt_3[i];
            memcpy(output_buffer_io_0 + output_buffer_num_io_0, answer_4_mt_3 + answer_4_header_mt_3[i], size << 2);
            output_buffer_num_io_0 += size;
            break;
        case 4:
            size = answer_4_tailer_mt_4[i] - answer_4_header_mt_4[i];
            memcpy(output_buffer_io_0 + output_buffer_num_io_0, answer_4_mt_4 + answer_4_header_mt_4[i], size << 2);
            output_buffer_num_io_0 += size;
            break;
        case 5:
            size = answer_4_tailer_mt_5[i] - answer_4_header_mt_5[i];
            memcpy(output_buffer_io_0 + output_buffer_num_io_0, answer_4_mt_5 + answer_4_header_mt_5[i], size << 2);
            output_buffer_num_io_0 += size;
            break;
        default:
            break;
        }
    }
    return NULL;
}
// end io 0

// io 1

char output_buffer_io_1[MAX_ANSW * ONE_INT_CHAR_SIZE * 3];
int output_buffer_num_io_1 = 0;
void* write_to_disk_io_1(void* args) {
    int from, to, u, i;
    from = 0; to = node_num;
    for (u=from; u<to; ++u) {
        switch (global_search_assignment[u])
        {
        case 0:
            for (i=answer_5_header_mt_0[u]; i<answer_5_tailer_mt_0[u]; ++i) {
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_0[i][0]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_0[i][1]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_0[i][2]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_0[i][3]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_0[i][4]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = '\n';
            }
            break;
        case 1:
            for (i=answer_5_header_mt_1[u]; i<answer_5_tailer_mt_1[u]; ++i) {
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_1[i][0]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_1[i][1]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_1[i][2]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_1[i][3]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_1[i][4]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = '\n';
            }
            break;
        case 2:
            for (i=answer_5_header_mt_2[u]; i<answer_5_tailer_mt_2[u]; ++i) {
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_2[i][0]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_2[i][1]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_2[i][2]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_2[i][3]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_2[i][4]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = '\n';
            }
            break;
        case 3:
            for (i=answer_5_header_mt_3[u]; i<answer_5_tailer_mt_3[u]; ++i) {
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_3[i][0]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_3[i][1]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_3[i][2]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_3[i][3]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_3[i][4]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = '\n';
            }
            break;
        case 4:
            for (i=answer_5_header_mt_4[u]; i<answer_5_tailer_mt_4[u]; ++i) {
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_4[i][0]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_4[i][1]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_4[i][2]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_4[i][3]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_4[i][4]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = '\n';
            }
            break;
        case 5:
            for (i=answer_5_header_mt_5[u]; i<answer_5_tailer_mt_5[u]; ++i) {
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_5[i][0]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_5[i][1]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_5[i][2]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_5[i][3]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = ',';
                deserialize_id(output_buffer_io_1, output_buffer_num_io_1, answer_5_mt_5[i][4]);
                output_buffer_io_1[output_buffer_num_io_1 ++] = '\n';
            }
            break;
        default:
            break;
        }
    }

    return NULL;
}

// end io 1

// io 2

char output_buffer_io_2[MAX_ANSW * ONE_INT_CHAR_SIZE * 3];
int output_buffer_num_io_2 = 0;
void* write_to_disk_io_2(void* args) {
    int from, to, u, i;
    from = 0; to = node_num;
    for (u=from; u<to; ++u) {
        switch (global_search_assignment[u])
        {
        case 0:
            for (i=answer_6_header_mt_0[u]; i<answer_6_tailer_mt_0[u]; ++i) {
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_0[i][0]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_0[i][1]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_0[i][2]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_0[i][3]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_0[i][4]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_0[i][5]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = '\n';
            }
            break;
        case 1:
            for (i=answer_6_header_mt_1[u]; i<answer_6_tailer_mt_1[u]; ++i) {
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_1[i][0]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_1[i][1]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_1[i][2]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_1[i][3]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_1[i][4]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_1[i][5]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = '\n';
            }
            break;
        case 2:
            for (i=answer_6_header_mt_2[u]; i<answer_6_tailer_mt_2[u]; ++i) {
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_2[i][0]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_2[i][1]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_2[i][2]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_2[i][3]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_2[i][4]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_2[i][5]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = '\n';
            }
            break;
        case 3:
            for (i=answer_6_header_mt_3[u]; i<answer_6_tailer_mt_3[u]; ++i) {
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_3[i][0]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_3[i][1]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_3[i][2]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_3[i][3]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_3[i][4]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_3[i][5]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = '\n';
            }
            break;
        case 4:
            for (i=answer_6_header_mt_4[u]; i<answer_6_tailer_mt_4[u]; ++i) {
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_4[i][0]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_4[i][1]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_4[i][2]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_4[i][3]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_4[i][4]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_4[i][5]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = '\n';
            }
            break;
        case 5:
            for (i=answer_6_header_mt_5[u]; i<answer_6_tailer_mt_5[u]; ++i) {
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_5[i][0]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_5[i][1]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_5[i][2]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_5[i][3]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_5[i][4]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = ',';
                deserialize_id(output_buffer_io_2, output_buffer_num_io_2, answer_6_mt_5[i][5]);
                output_buffer_io_2[output_buffer_num_io_2 ++] = '\n';
            }
            break;
        default:
            break;
        }
    }

    return NULL;
}

// end io 2

// io io_3_0

char output_buffer_io_3_0[MAX_ANSW * ONE_INT_CHAR_SIZE * 3];
int output_buffer_num_io_3_0 = 0;
void* write_to_disk_io_3_0(void* args) {
    int from, to, u, i;
    from = 0; to = node_num*3/200;
    for (u=from; u<to; ++u) {
        switch (global_search_assignment[u])
        {
        case 0:
            for (i=answer_7_header_mt_0[u]; i<answer_7_tailer_mt_0[u]; ++i) {
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_0[i][0]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_0[i][1]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_0[i][2]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_0[i][3]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_0[i][4]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_0[i][5]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_0[i][6]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = '\n';
            }
            break;
        case 1:
            for (i=answer_7_header_mt_1[u]; i<answer_7_tailer_mt_1[u]; ++i) {
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_1[i][0]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_1[i][1]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_1[i][2]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_1[i][3]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_1[i][4]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_1[i][5]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_1[i][6]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = '\n';
            }
            break;
        case 2:
            for (i=answer_7_header_mt_2[u]; i<answer_7_tailer_mt_2[u]; ++i) {
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_2[i][0]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_2[i][1]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_2[i][2]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_2[i][3]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_2[i][4]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_2[i][5]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_2[i][6]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = '\n';
            }
            break;
        case 3:
            for (i=answer_7_header_mt_3[u]; i<answer_7_tailer_mt_3[u]; ++i) {
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_3[i][0]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_3[i][1]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_3[i][2]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_3[i][3]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_3[i][4]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_3[i][5]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_3[i][6]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = '\n';
            }
            break;
        case 4:
            for (i=answer_7_header_mt_4[u]; i<answer_7_tailer_mt_4[u]; ++i) {
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_4[i][0]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_4[i][1]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_4[i][2]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_4[i][3]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_4[i][4]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_4[i][5]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_4[i][6]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = '\n';
            }
            break;
        case 5:
            for (i=answer_7_header_mt_5[u]; i<answer_7_tailer_mt_5[u]; ++i) {
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_5[i][0]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_5[i][1]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_5[i][2]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_5[i][3]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_5[i][4]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_5[i][5]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = ',';
                deserialize_id(output_buffer_io_3_0, output_buffer_num_io_3_0, answer_7_mt_5[i][6]);
                output_buffer_io_3_0[output_buffer_num_io_3_0 ++] = '\n';
            }
            break;
        default:
            break;
        }
    }

    return NULL;
}

// end io io_3_0

// io io_3_1

char output_buffer_io_3_1[MAX_ANSW * ONE_INT_CHAR_SIZE * 3];
int output_buffer_num_io_3_1 = 0;
void* write_to_disk_io_3_1(void* args) {
    int from, to, u, i;
    from = node_num*3/200; to = node_num*7/200;
    for (u=from; u<to; ++u) {
        switch (global_search_assignment[u])
        {
        case 0:
            for (i=answer_7_header_mt_0[u]; i<answer_7_tailer_mt_0[u]; ++i) {
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_0[i][0]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_0[i][1]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_0[i][2]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_0[i][3]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_0[i][4]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_0[i][5]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_0[i][6]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = '\n';
            }
            break;
        case 1:
            for (i=answer_7_header_mt_1[u]; i<answer_7_tailer_mt_1[u]; ++i) {
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_1[i][0]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_1[i][1]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_1[i][2]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_1[i][3]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_1[i][4]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_1[i][5]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_1[i][6]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = '\n';
            }
            break;
        case 2:
            for (i=answer_7_header_mt_2[u]; i<answer_7_tailer_mt_2[u]; ++i) {
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_2[i][0]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_2[i][1]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_2[i][2]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_2[i][3]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_2[i][4]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_2[i][5]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_2[i][6]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = '\n';
            }
            break;
        case 3:
            for (i=answer_7_header_mt_3[u]; i<answer_7_tailer_mt_3[u]; ++i) {
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_3[i][0]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_3[i][1]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_3[i][2]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_3[i][3]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_3[i][4]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_3[i][5]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_3[i][6]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = '\n';
            }
            break;
        case 4:
            for (i=answer_7_header_mt_4[u]; i<answer_7_tailer_mt_4[u]; ++i) {
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_4[i][0]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_4[i][1]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_4[i][2]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_4[i][3]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_4[i][4]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_4[i][5]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_4[i][6]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = '\n';
            }
            break;
        case 5:
            for (i=answer_7_header_mt_5[u]; i<answer_7_tailer_mt_5[u]; ++i) {
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_5[i][0]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_5[i][1]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_5[i][2]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_5[i][3]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_5[i][4]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_5[i][5]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = ',';
                deserialize_id(output_buffer_io_3_1, output_buffer_num_io_3_1, answer_7_mt_5[i][6]);
                output_buffer_io_3_1[output_buffer_num_io_3_1 ++] = '\n';
            }
            break;
        default:
            break;
        }
    }

    return NULL;
}

// end io io_3_1

// io io_3_2

char output_buffer_io_3_2[MAX_ANSW * ONE_INT_CHAR_SIZE * 3];
int output_buffer_num_io_3_2 = 0;
void* write_to_disk_io_3_2(void* args) {
    int from, to, u, i;
    from = node_num*7/200; to = node_num*12/200;
    for (u=from; u<to; ++u) {
        switch (global_search_assignment[u])
        {
        case 0:
            for (i=answer_7_header_mt_0[u]; i<answer_7_tailer_mt_0[u]; ++i) {
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_0[i][0]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_0[i][1]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_0[i][2]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_0[i][3]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_0[i][4]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_0[i][5]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_0[i][6]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = '\n';
            }
            break;
        case 1:
            for (i=answer_7_header_mt_1[u]; i<answer_7_tailer_mt_1[u]; ++i) {
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_1[i][0]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_1[i][1]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_1[i][2]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_1[i][3]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_1[i][4]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_1[i][5]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_1[i][6]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = '\n';
            }
            break;
        case 2:
            for (i=answer_7_header_mt_2[u]; i<answer_7_tailer_mt_2[u]; ++i) {
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_2[i][0]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_2[i][1]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_2[i][2]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_2[i][3]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_2[i][4]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_2[i][5]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_2[i][6]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = '\n';
            }
            break;
        case 3:
            for (i=answer_7_header_mt_3[u]; i<answer_7_tailer_mt_3[u]; ++i) {
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_3[i][0]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_3[i][1]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_3[i][2]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_3[i][3]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_3[i][4]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_3[i][5]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_3[i][6]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = '\n';
            }
            break;
        case 4:
            for (i=answer_7_header_mt_4[u]; i<answer_7_tailer_mt_4[u]; ++i) {
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_4[i][0]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_4[i][1]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_4[i][2]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_4[i][3]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_4[i][4]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_4[i][5]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_4[i][6]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = '\n';
            }
            break;
        case 5:
            for (i=answer_7_header_mt_5[u]; i<answer_7_tailer_mt_5[u]; ++i) {
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_5[i][0]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_5[i][1]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_5[i][2]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_5[i][3]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_5[i][4]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_5[i][5]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = ',';
                deserialize_id(output_buffer_io_3_2, output_buffer_num_io_3_2, answer_7_mt_5[i][6]);
                output_buffer_io_3_2[output_buffer_num_io_3_2 ++] = '\n';
            }
            break;
        default:
            break;
        }
    }

    return NULL;
}

// end io io_3_2

// io io_3_3

char output_buffer_io_3_3[MAX_ANSW * ONE_INT_CHAR_SIZE * 3];
int output_buffer_num_io_3_3 = 0;
void* write_to_disk_io_3_3(void* args) {
    int from, to, u, i;
    from = node_num*12/200; to = node_num*20/200;
    for (u=from; u<to; ++u) {
        switch (global_search_assignment[u])
        {
        case 0:
            for (i=answer_7_header_mt_0[u]; i<answer_7_tailer_mt_0[u]; ++i) {
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_0[i][0]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_0[i][1]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_0[i][2]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_0[i][3]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_0[i][4]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_0[i][5]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_0[i][6]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = '\n';
            }
            break;
        case 1:
            for (i=answer_7_header_mt_1[u]; i<answer_7_tailer_mt_1[u]; ++i) {
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_1[i][0]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_1[i][1]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_1[i][2]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_1[i][3]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_1[i][4]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_1[i][5]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_1[i][6]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = '\n';
            }
            break;
        case 2:
            for (i=answer_7_header_mt_2[u]; i<answer_7_tailer_mt_2[u]; ++i) {
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_2[i][0]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_2[i][1]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_2[i][2]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_2[i][3]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_2[i][4]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_2[i][5]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_2[i][6]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = '\n';
            }
            break;
        case 3:
            for (i=answer_7_header_mt_3[u]; i<answer_7_tailer_mt_3[u]; ++i) {
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_3[i][0]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_3[i][1]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_3[i][2]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_3[i][3]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_3[i][4]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_3[i][5]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_3[i][6]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = '\n';
            }
            break;
        case 4:
            for (i=answer_7_header_mt_4[u]; i<answer_7_tailer_mt_4[u]; ++i) {
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_4[i][0]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_4[i][1]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_4[i][2]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_4[i][3]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_4[i][4]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_4[i][5]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_4[i][6]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = '\n';
            }
            break;
        case 5:
            for (i=answer_7_header_mt_5[u]; i<answer_7_tailer_mt_5[u]; ++i) {
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_5[i][0]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_5[i][1]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_5[i][2]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_5[i][3]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_5[i][4]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_5[i][5]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = ',';
                deserialize_id(output_buffer_io_3_3, output_buffer_num_io_3_3, answer_7_mt_5[i][6]);
                output_buffer_io_3_3[output_buffer_num_io_3_3 ++] = '\n';
            }
            break;
        default:
            break;
        }
    }

    return NULL;
}

// end io io_3_3

// io io_3_4

char output_buffer_io_3_4[MAX_ANSW * ONE_INT_CHAR_SIZE * 3];
int output_buffer_num_io_3_4 = 0;
void* write_to_disk_io_3_4(void* args) {
    int from, to, u, i;
    from = node_num*20/200; to = node_num*50/200;
    for (u=from; u<to; ++u) {
        switch (global_search_assignment[u])
        {
        case 0:
            for (i=answer_7_header_mt_0[u]; i<answer_7_tailer_mt_0[u]; ++i) {
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_0[i][0]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_0[i][1]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_0[i][2]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_0[i][3]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_0[i][4]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_0[i][5]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_0[i][6]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = '\n';
            }
            break;
        case 1:
            for (i=answer_7_header_mt_1[u]; i<answer_7_tailer_mt_1[u]; ++i) {
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_1[i][0]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_1[i][1]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_1[i][2]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_1[i][3]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_1[i][4]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_1[i][5]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_1[i][6]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = '\n';
            }
            break;
        case 2:
            for (i=answer_7_header_mt_2[u]; i<answer_7_tailer_mt_2[u]; ++i) {
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_2[i][0]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_2[i][1]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_2[i][2]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_2[i][3]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_2[i][4]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_2[i][5]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_2[i][6]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = '\n';
            }
            break;
        case 3:
            for (i=answer_7_header_mt_3[u]; i<answer_7_tailer_mt_3[u]; ++i) {
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_3[i][0]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_3[i][1]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_3[i][2]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_3[i][3]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_3[i][4]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_3[i][5]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_3[i][6]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = '\n';
            }
            break;
        case 4:
            for (i=answer_7_header_mt_4[u]; i<answer_7_tailer_mt_4[u]; ++i) {
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_4[i][0]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_4[i][1]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_4[i][2]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_4[i][3]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_4[i][4]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_4[i][5]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_4[i][6]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = '\n';
            }
            break;
        case 5:
            for (i=answer_7_header_mt_5[u]; i<answer_7_tailer_mt_5[u]; ++i) {
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_5[i][0]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_5[i][1]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_5[i][2]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_5[i][3]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_5[i][4]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_5[i][5]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = ',';
                deserialize_id(output_buffer_io_3_4, output_buffer_num_io_3_4, answer_7_mt_5[i][6]);
                output_buffer_io_3_4[output_buffer_num_io_3_4 ++] = '\n';
            }
            break;
        default:
            break;
        }
    }

    return NULL;
}

// end io io_3_4

// io io_3_5

char output_buffer_io_3_5[MAX_ANSW * ONE_INT_CHAR_SIZE * 3];
int output_buffer_num_io_3_5 = 0;
void* write_to_disk_io_3_5(void* args) {
    int from, to, u, i;
    from = node_num*50/200; to = node_num*100/200;
    for (u=from; u<to; ++u) {
        switch (global_search_assignment[u])
        {
        case 0:
            for (i=answer_7_header_mt_0[u]; i<answer_7_tailer_mt_0[u]; ++i) {
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_0[i][0]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_0[i][1]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_0[i][2]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_0[i][3]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_0[i][4]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_0[i][5]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_0[i][6]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = '\n';
            }
            break;
        case 1:
            for (i=answer_7_header_mt_1[u]; i<answer_7_tailer_mt_1[u]; ++i) {
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_1[i][0]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_1[i][1]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_1[i][2]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_1[i][3]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_1[i][4]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_1[i][5]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_1[i][6]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = '\n';
            }
            break;
        case 2:
            for (i=answer_7_header_mt_2[u]; i<answer_7_tailer_mt_2[u]; ++i) {
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_2[i][0]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_2[i][1]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_2[i][2]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_2[i][3]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_2[i][4]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_2[i][5]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_2[i][6]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = '\n';
            }
            break;
        case 3:
            for (i=answer_7_header_mt_3[u]; i<answer_7_tailer_mt_3[u]; ++i) {
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_3[i][0]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_3[i][1]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_3[i][2]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_3[i][3]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_3[i][4]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_3[i][5]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_3[i][6]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = '\n';
            }
            break;
        case 4:
            for (i=answer_7_header_mt_4[u]; i<answer_7_tailer_mt_4[u]; ++i) {
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_4[i][0]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_4[i][1]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_4[i][2]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_4[i][3]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_4[i][4]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_4[i][5]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_4[i][6]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = '\n';
            }
            break;
        case 5:
            for (i=answer_7_header_mt_5[u]; i<answer_7_tailer_mt_5[u]; ++i) {
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_5[i][0]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_5[i][1]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_5[i][2]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_5[i][3]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_5[i][4]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_5[i][5]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = ',';
                deserialize_id(output_buffer_io_3_5, output_buffer_num_io_3_5, answer_7_mt_5[i][6]);
                output_buffer_io_3_5[output_buffer_num_io_3_5 ++] = '\n';
            }
            break;
        default:
            break;
        }
    }

    return NULL;
}

// end io io_3_5

// io io_3_6

char output_buffer_io_3_6[MAX_ANSW * ONE_INT_CHAR_SIZE * 3];
int output_buffer_num_io_3_6 = 0;
void* write_to_disk_io_3_6(void* args) {
    int from, to, u, i;
    from = node_num*100/200; to = node_num*120/200;
    for (u=from; u<to; ++u) {
        switch (global_search_assignment[u])
        {
        case 0:
            for (i=answer_7_header_mt_0[u]; i<answer_7_tailer_mt_0[u]; ++i) {
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_0[i][0]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_0[i][1]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_0[i][2]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_0[i][3]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_0[i][4]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_0[i][5]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_0[i][6]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = '\n';
            }
            break;
        case 1:
            for (i=answer_7_header_mt_1[u]; i<answer_7_tailer_mt_1[u]; ++i) {
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_1[i][0]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_1[i][1]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_1[i][2]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_1[i][3]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_1[i][4]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_1[i][5]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_1[i][6]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = '\n';
            }
            break;
        case 2:
            for (i=answer_7_header_mt_2[u]; i<answer_7_tailer_mt_2[u]; ++i) {
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_2[i][0]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_2[i][1]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_2[i][2]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_2[i][3]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_2[i][4]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_2[i][5]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_2[i][6]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = '\n';
            }
            break;
        case 3:
            for (i=answer_7_header_mt_3[u]; i<answer_7_tailer_mt_3[u]; ++i) {
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_3[i][0]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_3[i][1]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_3[i][2]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_3[i][3]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_3[i][4]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_3[i][5]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_3[i][6]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = '\n';
            }
            break;
        case 4:
            for (i=answer_7_header_mt_4[u]; i<answer_7_tailer_mt_4[u]; ++i) {
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_4[i][0]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_4[i][1]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_4[i][2]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_4[i][3]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_4[i][4]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_4[i][5]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_4[i][6]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = '\n';
            }
            break;
        case 5:
            for (i=answer_7_header_mt_5[u]; i<answer_7_tailer_mt_5[u]; ++i) {
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_5[i][0]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_5[i][1]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_5[i][2]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_5[i][3]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_5[i][4]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_5[i][5]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = ',';
                deserialize_id(output_buffer_io_3_6, output_buffer_num_io_3_6, answer_7_mt_5[i][6]);
                output_buffer_io_3_6[output_buffer_num_io_3_6 ++] = '\n';
            }
            break;
        default:
            break;
        }
    }

    return NULL;
}

// end io io_3_6

// io io_3_7

char output_buffer_io_3_7[MAX_ANSW * ONE_INT_CHAR_SIZE * 3];
int output_buffer_num_io_3_7 = 0;
void* write_to_disk_io_3_7(void* args) {
    int from, to, u, i;
    from = node_num*120/200; to = node_num*140/200;
    for (u=from; u<to; ++u) {
        switch (global_search_assignment[u])
        {
        case 0:
            for (i=answer_7_header_mt_0[u]; i<answer_7_tailer_mt_0[u]; ++i) {
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_0[i][0]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_0[i][1]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_0[i][2]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_0[i][3]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_0[i][4]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_0[i][5]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_0[i][6]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = '\n';
            }
            break;
        case 1:
            for (i=answer_7_header_mt_1[u]; i<answer_7_tailer_mt_1[u]; ++i) {
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_1[i][0]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_1[i][1]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_1[i][2]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_1[i][3]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_1[i][4]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_1[i][5]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_1[i][6]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = '\n';
            }
            break;
        case 2:
            for (i=answer_7_header_mt_2[u]; i<answer_7_tailer_mt_2[u]; ++i) {
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_2[i][0]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_2[i][1]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_2[i][2]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_2[i][3]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_2[i][4]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_2[i][5]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_2[i][6]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = '\n';
            }
            break;
        case 3:
            for (i=answer_7_header_mt_3[u]; i<answer_7_tailer_mt_3[u]; ++i) {
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_3[i][0]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_3[i][1]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_3[i][2]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_3[i][3]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_3[i][4]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_3[i][5]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_3[i][6]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = '\n';
            }
            break;
        case 4:
            for (i=answer_7_header_mt_4[u]; i<answer_7_tailer_mt_4[u]; ++i) {
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_4[i][0]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_4[i][1]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_4[i][2]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_4[i][3]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_4[i][4]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_4[i][5]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_4[i][6]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = '\n';
            }
            break;
        case 5:
            for (i=answer_7_header_mt_5[u]; i<answer_7_tailer_mt_5[u]; ++i) {
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_5[i][0]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_5[i][1]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_5[i][2]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_5[i][3]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_5[i][4]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_5[i][5]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = ',';
                deserialize_id(output_buffer_io_3_7, output_buffer_num_io_3_7, answer_7_mt_5[i][6]);
                output_buffer_io_3_7[output_buffer_num_io_3_7 ++] = '\n';
            }
            break;
        default:
            break;
        }
    }

    return NULL;
}

// end io io_3_7

// io io_3_8

char output_buffer_io_3_8[MAX_ANSW * ONE_INT_CHAR_SIZE * 3];
int output_buffer_num_io_3_8 = 0;
void* write_to_disk_io_3_8(void* args) {
    int from, to, u, i;
    from = node_num*140/200; to = node_num*160/200;
    for (u=from; u<to; ++u) {
        switch (global_search_assignment[u])
        {
        case 0:
            for (i=answer_7_header_mt_0[u]; i<answer_7_tailer_mt_0[u]; ++i) {
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_0[i][0]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_0[i][1]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_0[i][2]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_0[i][3]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_0[i][4]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_0[i][5]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_0[i][6]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = '\n';
            }
            break;
        case 1:
            for (i=answer_7_header_mt_1[u]; i<answer_7_tailer_mt_1[u]; ++i) {
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_1[i][0]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_1[i][1]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_1[i][2]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_1[i][3]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_1[i][4]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_1[i][5]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_1[i][6]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = '\n';
            }
            break;
        case 2:
            for (i=answer_7_header_mt_2[u]; i<answer_7_tailer_mt_2[u]; ++i) {
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_2[i][0]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_2[i][1]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_2[i][2]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_2[i][3]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_2[i][4]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_2[i][5]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_2[i][6]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = '\n';
            }
            break;
        case 3:
            for (i=answer_7_header_mt_3[u]; i<answer_7_tailer_mt_3[u]; ++i) {
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_3[i][0]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_3[i][1]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_3[i][2]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_3[i][3]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_3[i][4]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_3[i][5]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_3[i][6]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = '\n';
            }
            break;
        case 4:
            for (i=answer_7_header_mt_4[u]; i<answer_7_tailer_mt_4[u]; ++i) {
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_4[i][0]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_4[i][1]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_4[i][2]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_4[i][3]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_4[i][4]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_4[i][5]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_4[i][6]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = '\n';
            }
            break;
        case 5:
            for (i=answer_7_header_mt_5[u]; i<answer_7_tailer_mt_5[u]; ++i) {
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_5[i][0]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_5[i][1]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_5[i][2]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_5[i][3]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_5[i][4]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_5[i][5]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = ',';
                deserialize_id(output_buffer_io_3_8, output_buffer_num_io_3_8, answer_7_mt_5[i][6]);
                output_buffer_io_3_8[output_buffer_num_io_3_8 ++] = '\n';
            }
            break;
        default:
            break;
        }
    }

    return NULL;
}

// end io io_3_8

// io io_3_9

char output_buffer_io_3_9[MAX_ANSW * ONE_INT_CHAR_SIZE * 3];
int output_buffer_num_io_3_9 = 0;
void* write_to_disk_io_3_9(void* args) {
    int from, to, u, i;
    from = node_num*160/200; to = node_num;
    for (u=from; u<to; ++u) {
        switch (global_search_assignment[u])
        {
        case 0:
            for (i=answer_7_header_mt_0[u]; i<answer_7_tailer_mt_0[u]; ++i) {
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_0[i][0]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_0[i][1]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_0[i][2]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_0[i][3]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_0[i][4]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_0[i][5]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_0[i][6]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = '\n';
            }
            break;
        case 1:
            for (i=answer_7_header_mt_1[u]; i<answer_7_tailer_mt_1[u]; ++i) {
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_1[i][0]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_1[i][1]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_1[i][2]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_1[i][3]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_1[i][4]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_1[i][5]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_1[i][6]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = '\n';
            }
            break;
        case 2:
            for (i=answer_7_header_mt_2[u]; i<answer_7_tailer_mt_2[u]; ++i) {
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_2[i][0]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_2[i][1]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_2[i][2]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_2[i][3]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_2[i][4]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_2[i][5]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_2[i][6]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = '\n';
            }
            break;
        case 3:
            for (i=answer_7_header_mt_3[u]; i<answer_7_tailer_mt_3[u]; ++i) {
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_3[i][0]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_3[i][1]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_3[i][2]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_3[i][3]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_3[i][4]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_3[i][5]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_3[i][6]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = '\n';
            }
            break;
        case 4:
            for (i=answer_7_header_mt_4[u]; i<answer_7_tailer_mt_4[u]; ++i) {
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_4[i][0]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_4[i][1]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_4[i][2]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_4[i][3]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_4[i][4]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_4[i][5]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_4[i][6]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = '\n';
            }
            break;
        case 5:
            for (i=answer_7_header_mt_5[u]; i<answer_7_tailer_mt_5[u]; ++i) {
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_5[i][0]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_5[i][1]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_5[i][2]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_5[i][3]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_5[i][4]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_5[i][5]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = ',';
                deserialize_id(output_buffer_io_3_9, output_buffer_num_io_3_9, answer_7_mt_5[i][6]);
                output_buffer_io_3_9[output_buffer_num_io_3_9 ++] = '\n';
            }
            break;
        default:
            break;
        }
    }

    return NULL;
}

// end io io_3_9





void write_to_disk() {
    int rub;

    pthread_t deserialize_thr[13];
    pthread_create(deserialize_thr + 0, NULL, write_to_disk_io_0, NULL);
    pthread_create(deserialize_thr + 1, NULL, write_to_disk_io_1, NULL);
    pthread_create(deserialize_thr + 2, NULL, write_to_disk_io_2, NULL);
    pthread_create(deserialize_thr + 3, NULL, write_to_disk_io_3_0, NULL);
    pthread_create(deserialize_thr + 4, NULL, write_to_disk_io_3_1, NULL);
    pthread_create(deserialize_thr + 5, NULL, write_to_disk_io_3_2, NULL);

    pthread_join(deserialize_thr[0], NULL);
    pthread_create(deserialize_thr + 6, NULL, write_to_disk_io_3_3, NULL);
    rub = write(writer_fd, output_buffer_io_0, output_buffer_num_io_0);

    pthread_join(deserialize_thr[1], NULL);
    pthread_create(deserialize_thr + 7, NULL, write_to_disk_io_3_4, NULL);
    rub = write(writer_fd, output_buffer_io_1, output_buffer_num_io_1);

    pthread_join(deserialize_thr[2], NULL);
    pthread_create(deserialize_thr + 8, NULL, write_to_disk_io_3_5, NULL);
    rub = write(writer_fd, output_buffer_io_2, output_buffer_num_io_2);

    pthread_join(deserialize_thr[3], NULL);
    pthread_create(deserialize_thr + 9, NULL, write_to_disk_io_3_6, NULL);
    rub = write(writer_fd, output_buffer_io_3_0, output_buffer_num_io_3_0);

    pthread_join(deserialize_thr[4], NULL);
    pthread_create(deserialize_thr + 10, NULL, write_to_disk_io_3_7, NULL);
    rub = write(writer_fd, output_buffer_io_3_1, output_buffer_num_io_3_1);

    pthread_join(deserialize_thr[5], NULL);
    pthread_create(deserialize_thr + 11, NULL, write_to_disk_io_3_8, NULL);
    rub = write(writer_fd, output_buffer_io_3_2, output_buffer_num_io_3_2);

    pthread_join(deserialize_thr[6], NULL);
    pthread_create(deserialize_thr + 12, NULL, write_to_disk_io_3_9, NULL);
    rub = write(writer_fd, output_buffer_io_3_3, output_buffer_num_io_3_3);

    pthread_join(deserialize_thr[7], NULL);
    rub = write(writer_fd, output_buffer_io_3_4, output_buffer_num_io_3_4);

    pthread_join(deserialize_thr[8], NULL);
    rub = write(writer_fd, output_buffer_io_3_5, output_buffer_num_io_3_5);

    pthread_join(deserialize_thr[9], NULL);
    rub = write(writer_fd, output_buffer_io_3_6, output_buffer_num_io_3_6);

    pthread_join(deserialize_thr[10], NULL);
    rub = write(writer_fd, output_buffer_io_3_7, output_buffer_num_io_3_7);

    pthread_join(deserialize_thr[11], NULL);
    rub = write(writer_fd, output_buffer_io_3_8, output_buffer_num_io_3_8);

    pthread_join(deserialize_thr[12], NULL);
    rub = write(writer_fd, output_buffer_io_3_9, output_buffer_num_io_3_9);
}




//------------------ MBODY ---------------------
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
    for (int i=0; i<PATH_PAR_3; ++i) std_id_arr[i] = i;

    end_time = clock();
    printf("build edge: %d ms\n", (int)(end_time - start_time)); fflush(stdout);

    mid_time = clock();
    
    pthread_t search_thr[6];
    pthread_create(search_thr + 0, NULL, search_mt_0, NULL);
    pthread_create(search_thr + 1, NULL, search_mt_1, NULL);
    pthread_create(search_thr + 2, NULL, search_mt_2, NULL);
    pthread_create(search_thr + 3, NULL, search_mt_3, NULL);
    pthread_create(search_thr + 4, NULL, search_mt_4, NULL);
    pthread_create(search_thr + 5, NULL, search_mt_5, NULL);

    answer_num = 0;
    pthread_join(search_thr[0], NULL);
    answer_num += answer_3_num_mt_0 + answer_4_num_mt_0 + answer_5_num_mt_0 + answer_6_num_mt_0 + answer_7_num_mt_0;

    pthread_join(search_thr[1], NULL);
    answer_num += answer_3_num_mt_1 + answer_4_num_mt_1 + answer_5_num_mt_1 + answer_6_num_mt_1 + answer_7_num_mt_1;

    pthread_join(search_thr[2], NULL);
    answer_num += answer_3_num_mt_2 + answer_4_num_mt_2 + answer_5_num_mt_2 + answer_6_num_mt_2 + answer_7_num_mt_2;

    pthread_join(search_thr[3], NULL);
    answer_num += answer_3_num_mt_3 + answer_4_num_mt_3 + answer_5_num_mt_3 + answer_6_num_mt_3 + answer_7_num_mt_3;

    pthread_join(search_thr[4], NULL);
    answer_num += answer_3_num_mt_4 + answer_4_num_mt_4 + answer_5_num_mt_4 + answer_6_num_mt_4 + answer_7_num_mt_4;

    pthread_join(search_thr[5], NULL);
    answer_num += answer_3_num_mt_5 + answer_4_num_mt_5 + answer_5_num_mt_5 + answer_6_num_mt_5 + answer_7_num_mt_5;

    

    int answer_3_num = 0, answer_4_num = 0, answer_5_num = 0, answer_6_num = 0, answer_7_num = 0;
    answer_3_num = answer_3_num_mt_0 + answer_3_num_mt_1 + answer_3_num_mt_2 + answer_3_num_mt_3 + answer_3_num_mt_4 + answer_3_num_mt_5;
    answer_4_num = answer_4_num_mt_0 + answer_4_num_mt_1 + answer_4_num_mt_2 + answer_4_num_mt_3 + answer_4_num_mt_4 + answer_4_num_mt_5;
    answer_5_num = answer_5_num_mt_0 + answer_5_num_mt_1 + answer_5_num_mt_2 + answer_5_num_mt_3 + answer_5_num_mt_4 + answer_5_num_mt_5;
    answer_6_num = answer_6_num_mt_0 + answer_6_num_mt_1 + answer_6_num_mt_2 + answer_6_num_mt_3 + answer_6_num_mt_4 + answer_6_num_mt_5;
    answer_7_num = answer_7_num_mt_0 + answer_7_num_mt_1 + answer_7_num_mt_2 + answer_7_num_mt_3 + answer_7_num_mt_4 + answer_7_num_mt_5;
    printf("answer: %d\n", answer_num);
    printf("answer 3: %d\n", answer_3_num);
    printf("answer 4: %d\n", answer_4_num);
    printf("answer 5: %d\n", answer_5_num);
    printf("answer 6: %d\n", answer_6_num);
    printf("answer 7: %d\n", answer_7_num);
    
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