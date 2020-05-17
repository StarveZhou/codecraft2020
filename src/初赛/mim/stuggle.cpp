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
// #define INPUT_PATH   "resources/2755223.txt"
// #define INPUT_PATH   "resources/test_data.txt"
// #define INPUT_PATH   "resources/pre_test.txt"
#define OUTPUT_PATH  "strug.txt"


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

#define MAX_ANS_EACH     1000000
#define MAX_BACK         1000000

int answer_mt_0[5][MAX_ANSW * 7];
int answer_num_mt_0[5];
int answer_header_mt_0[5][MAX_NODE], answer_tailer_mt_0[5][MAX_NODE];
int total_answer_num_mt_0 = 0;

int edge_topo_starter_mt_0[MAX_NODE];

int visit_mt_0[MAX_NODE], mask_mt_0;
int dfs_path_mt_0[8], dfs_path_num_mt_0;

int front_visit_mt_0[MAX_NODE], front_mask_mt_0;

int back_visit_mt_0[MAX_NODE], back_mask_mt_0;
int back_search_edges_mt_0[MAX_BACK][3];
int back_search_edges_num_mt_0;
int back_search_contains_mt_0[MAX_NODE], back_search_contains_num_mt_0;
int back_search_header_mt_0[MAX_NODE], back_search_ptr_mt_0[MAX_BACK];
int back_search_tailer_mt_0[MAX_NODE];

int back_search_answer_mt_0[2][MAX_BACK][3];
int back_search_answer_num_mt_0[2];

void back_dfs_mt_0() {
    int u, v, tid;
    u = dfs_path_mt_0[dfs_path_num_mt_0 - 1];
    for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
        v = rev_edge_topo_edges[i];
        if (v <= dfs_path_mt_0[0]) break;
        if (visit_mt_0[v] == mask_mt_0) continue;
        if (dfs_path_num_mt_0 == 2) {
            if (front_visit_mt_0[v] == front_mask_mt_0) {
                tid = back_search_answer_num_mt_0[0];
                back_search_answer_mt_0[0][tid][0] = v;
                back_search_answer_mt_0[0][tid][1] = dfs_path_mt_0[1];
                back_search_answer_num_mt_0[0] ++;
            }
        }
        if (dfs_path_num_mt_0 == 3) {
            if (front_visit_mt_0[v] == front_mask_mt_0) {
                tid = back_search_answer_num_mt_0[1];
                back_search_answer_mt_0[1][tid][0] = v;
                back_search_answer_mt_0[1][tid][1] = dfs_path_mt_0[2];
                back_search_answer_mt_0[1][tid][2] = dfs_path_mt_0[1];
                back_search_answer_num_mt_0[1] ++;
            }
            if (back_visit_mt_0[v] != back_mask_mt_0) {
                back_search_header_mt_0[v] = -1;
                back_search_contains_mt_0[back_search_contains_num_mt_0 ++] = v;
            }
            back_visit_mt_0[v] = back_mask_mt_0;

            tid = back_search_edges_num_mt_0;
            back_search_edges_mt_0[tid][0] = dfs_path_mt_0[2];
            back_search_edges_mt_0[tid][1] = dfs_path_mt_0[1];
            back_search_ptr_mt_0[tid] = back_search_header_mt_0[v];
            back_search_header_mt_0[v] = tid;
            back_search_edges_num_mt_0 ++;
            continue;
        }
        visit_mt_0[v] = mask_mt_0;
        dfs_path_mt_0[dfs_path_num_mt_0 ++] = v;
        back_dfs_mt_0();
        visit_mt_0[v] = -1;
        dfs_path_num_mt_0 --;
    }
}

int back_search_id_mt_0[MAX_BACK];

void sort_out_mt_0(int begin_with) {
    int tnum, id;
    if (back_search_answer_num_mt_0[0] > 0) {
        tnum = back_search_answer_num_mt_0[0];
        total_answer_num_mt_0 += tnum;
        memcpy(back_search_id_mt_0, std_id_arr, sizeof(int) * tnum);
        std::sort(back_search_id_mt_0, back_search_id_mt_0 + tnum, [](int i, int j) -> bool{
            if (back_search_answer_mt_0[0][i][0] != back_search_answer_mt_0[0][j][0]) return back_search_answer_mt_0[0][i][0] < back_search_answer_mt_0[0][j][0];
            return back_search_answer_mt_0[0][i][1] < back_search_answer_mt_0[0][j][1];
        });
        answer_header_mt_0[0][begin_with] = answer_num_mt_0[0];
        for (int i=0; i<tnum; ++i) {
            id = back_search_id_mt_0[i];
            answer_mt_0[0][answer_num_mt_0[0] ++] = back_search_answer_mt_0[0][id][0];
            answer_mt_0[0][answer_num_mt_0[0] ++] = back_search_answer_mt_0[0][id][1];
        }
        answer_tailer_mt_0[0][begin_with] = answer_num_mt_0[0];
    }
    // printf("X\n"); fflush(stdout);
    if (back_search_answer_mt_0[1] > 0) {
        tnum = back_search_answer_num_mt_0[1];
        total_answer_num_mt_0 += tnum;
        memcpy(back_search_id_mt_0, std_id_arr, sizeof(int) * tnum);
        std::sort(back_search_id_mt_0, back_search_id_mt_0 + tnum, [](int i, int j) -> bool{
            if (back_search_answer_mt_0[1][i][0] != back_search_answer_mt_0[1][j][0]) return back_search_answer_mt_0[1][i][0] < back_search_answer_mt_0[1][j][0];
            if (back_search_answer_mt_0[1][i][1] != back_search_answer_mt_0[1][j][1]) return back_search_answer_mt_0[1][i][1] < back_search_answer_mt_0[1][j][1];
            return back_search_answer_mt_0[1][i][2] < back_search_answer_mt_0[1][j][2];
        });
        answer_header_mt_0[1][begin_with] = answer_num_mt_0[1];
        for (int i=0; i<tnum; ++i) {
            id = back_search_id_mt_0[i];
            answer_mt_0[1][answer_num_mt_0[1] ++] = back_search_answer_mt_0[1][id][0];
            answer_mt_0[1][answer_num_mt_0[1] ++] = back_search_answer_mt_0[1][id][1];
            answer_mt_0[1][answer_num_mt_0[1] ++] = back_search_answer_mt_0[1][id][2];
        }
        answer_tailer_mt_0[1][begin_with] = answer_num_mt_0[1];
    }
    // printf("Y\n"); fflush(stdout);
    if (back_search_edges_num_mt_0 > 0) {
        int u, last_tnum; tnum = 0;
        for (int i=0; i<back_search_contains_num_mt_0; ++i) {
            u = back_search_contains_mt_0[i];
            last_tnum = tnum;
            for (int j=back_search_header_mt_0[u]; j!=-1; j=back_search_ptr_mt_0[j]) {
                back_search_id_mt_0[tnum ++] = j;
            }
            back_search_header_mt_0[u] = last_tnum;
            back_search_tailer_mt_0[u] = tnum;
            std::sort(back_search_id_mt_0 + last_tnum, back_search_id_mt_0 + tnum, [](int i, int j) -> bool{
                if (back_search_edges_mt_0[i][0] != back_search_edges_mt_0[j][0]) return back_search_edges_mt_0[i][0] < back_search_edges_mt_0[j][0];
                return back_search_edges_mt_0[i][1] < back_search_edges_mt_0[j][1];
            });
        }
    }
    // printf("Z\n"); fflush(stdout);
}

void inline extract_answer_mt_0(int u) {
    int id, x, y;
    for (int i=back_search_header_mt_0[u]; i<back_search_tailer_mt_0[u]; ++i) {
        id = back_search_id_mt_0[i];
        x = back_search_edges_mt_0[id][0]; y = back_search_edges_mt_0[id][1];
        if (visit_mt_0[x] == mask_mt_0 || visit_mt_0[y] == mask_mt_0) continue;
        for (int j=1; j<dfs_path_num_mt_0; ++j) {
            answer_mt_0[dfs_path_num_mt_0][answer_num_mt_0[dfs_path_num_mt_0] ++] = dfs_path_mt_0[j];
        }
        answer_mt_0[dfs_path_num_mt_0][answer_num_mt_0[dfs_path_num_mt_0] ++] = u;
        answer_mt_0[dfs_path_num_mt_0][answer_num_mt_0[dfs_path_num_mt_0] ++] = x;
        answer_mt_0[dfs_path_num_mt_0][answer_num_mt_0[dfs_path_num_mt_0] ++] = y;
        total_answer_num_mt_0 ++;
    }
}

void forw_dfs_mt_0() {
    int u, v;
    u = dfs_path_mt_0[dfs_path_num_mt_0 - 1];
    // printf("search %d =%d\n", u, dfs_path_num_mt_0); fflush(stdout);
    while (edge_topo_starter_mt_0[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_0[u]] <= dfs_path_mt_0[0]) edge_topo_starter_mt_0[u] ++;
    for (int i=edge_topo_starter_mt_0[u]; i<edge_topo_header[u+1]; ++i) {
        // printf("here: %d\n", i); fflush(stdout);
        v = edge_topo_edges[i];
        if (visit_mt_0[v] == mask_mt_0) continue;
        if (dfs_path_num_mt_0 >= 2 && dfs_path_num_mt_0 <= 4) {
            if (back_visit_mt_0[v] == back_mask_mt_0) {
                extract_answer_mt_0(v);
            }
        }
        
        if (dfs_path_num_mt_0 == 4) continue;

        visit_mt_0[v] = mask_mt_0;
        dfs_path_mt_0[dfs_path_num_mt_0 ++] = v;
        forw_dfs_mt_0();
        visit_mt_0[v] = -1;
        dfs_path_num_mt_0 --;
    }
}

void do_search_mt_0(int begin_with) {
    front_mask_mt_0 ++;
    while (edge_topo_starter_mt_0[begin_with] < edge_topo_header[begin_with+1] && edge_topo_edges[edge_topo_starter_mt_0[begin_with]] <= begin_with) edge_topo_starter_mt_0[begin_with] ++;
    for (int i=edge_topo_starter_mt_0[begin_with]; i<edge_topo_header[begin_with+1]; ++i) {
        front_visit_mt_0[edge_topo_edges[i]] = front_mask_mt_0;
    }

    back_search_edges_num_mt_0 = back_search_contains_num_mt_0 = 0;
    back_search_answer_num_mt_0[0] = back_search_answer_num_mt_0[1] = 0;
    back_mask_mt_0 ++; back_visit_mt_0[begin_with] = back_mask_mt_0;
    mask_mt_0 ++; visit_mt_0[begin_with] = mask_mt_0;
    dfs_path_mt_0[0] = begin_with; dfs_path_num_mt_0 = 1;
    back_dfs_mt_0();
    // printf("back search over\n"); fflush(stdout);
    sort_out_mt_0(begin_with);
    if (back_search_edges_num_mt_0 == 0) return;
    // printf("back over\n"); fflush(stdout);
    mask_mt_0 ++; visit_mt_0[begin_with] = mask_mt_0;
    dfs_path_mt_0[0] = begin_with; dfs_path_num_mt_0 = 1;

    answer_header_mt_0[2][begin_with] = answer_num_mt_0[2];
    answer_header_mt_0[3][begin_with] = answer_num_mt_0[3];
    answer_header_mt_0[4][begin_with] = answer_num_mt_0[4];
    forw_dfs_mt_0();
    answer_tailer_mt_0[2][begin_with] = answer_num_mt_0[2];
    answer_tailer_mt_0[3][begin_with] = answer_num_mt_0[3];
    answer_tailer_mt_0[4][begin_with] = answer_num_mt_0[4];
    // printf("all over %d\n", total_answer_num_mt_0); fflush(stdout);
}

void search_mt_0() {
    memcpy(edge_topo_starter_mt_0, edge_topo_header, sizeof(int) * node_num);
    for (int u=0; u<node_num; ++u) {
        // printf("another: %d %d %d %d\n", u, edge_topo_starter_mt_0[u], edge_topo_header[u], edge_topo_header[u+1]); fflush(stdout);
        if (edge_topo_starter_mt_0[u] == edge_topo_header[u+1]) continue;
        // printf("A\n"); fflush(stdout);
        if (!searchable_nodes[0][u]) continue;
        // printf("search %d\n", u); fflush(stdout);
        do_search_mt_0(u);
        // printf("over search\n"); fflush(stdout);
    }
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

int answer_num = 0;

void write_to_disk(char* buffer, int& buffer_num, int id, int from, int to) {
    int unit_size = id + 2;
    for (int u=from; u<to; ++u) {
        for (int i=answer_header_mt_0[id][u]; i<answer_tailer_mt_0[id][u]; i+=unit_size) {
            deserialize_id(buffer, buffer_num, u);
            for (int j=i; j<i+unit_size; ++j) {
                buffer[buffer_num ++] = ',';
                deserialize_id(buffer, buffer_num, answer_mt_0[id][j]);
            }
            buffer[buffer_num ++] = '\n';
        }
    }
}

char buffer_io_0[ONE_INT_CHAR_SIZE * MAX_ANSW * 7];
int buffer_num_io_0;

void write_to_disk_io_0() {
    deserialize_int(buffer_io_0, buffer_num_io_0, answer_num);
    buffer_io_0[buffer_num_io_0 ++] = '\n';
    write_to_disk(buffer_io_0, buffer_num_io_0, 0, 0, node_num);
    write_to_disk(buffer_io_0, buffer_num_io_0, 1, 0, node_num);
    write_to_disk(buffer_io_0, buffer_num_io_0, 2, 0, node_num);
    write_to_disk(buffer_io_0, buffer_num_io_0, 3, 0, node_num);
    write_to_disk(buffer_io_0, buffer_num_io_0, 4, 0, node_num);
}

void write_to_disk() {
    size_t rub;
    write_to_disk_io_0();
    
    rub = write(writer_fd, buffer_io_0, buffer_num_io_0);
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
    for (int i=0; i<MAX_BACK; ++i) std_id_arr[i] = i;

    end_time = clock();
    printf("build edge: %d ms\n", (int)(end_time - start_time)); fflush(stdout);

    mid_time = clock();
    search_mt_0();
    answer_num = total_answer_num_mt_0;
    printf("answer: %d\n", answer_num);
    
    printf("total %d\n", answer_num);

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