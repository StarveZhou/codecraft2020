#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <pthread.h>

#include <string.h>


#include <algorithm>

//------------------ MACRO ---------------------



#define INPUT_PATH  "/data/test_data.txt"
#define OUTPUT_PATH "/projects/student/result.txt"

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

#define MAX_AVA                50000

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
int CACHE_LINE_ALIGN data_skip[MAX_EDGE];

int CACHE_LINE_ALIGN data_rev_mapping[MAX_NODE];
int CACHE_LINE_ALIGN node_num = 0;
c_hash_map_t mapping_t; c_hash_map mapping = &mapping_t;

void read_input() {
    int fd = open(INPUT_PATH, O_RDONLY);
    if (fd == -1) {
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
    int counter = 0;
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

    for (int i=0; i<data_num; ++i) {
        if (data[i][0] > MAX_AVA || data[i][1] > MAX_AVA) {
            data_skip[i] = 1;
            continue;
        }
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
        if (data_skip[i] == 1) continue;
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


int CACHE_LINE_ALIGN rehash_mapping[MAX_NODE];
int CACHE_LINE_ALIGN edge_topo_header[MAX_NODE];
int CACHE_LINE_ALIGN edge_topo_edges[MAX_EDGE];
int CACHE_LINE_ALIGN rev_edge_topo_header[MAX_NODE];
int CACHE_LINE_ALIGN rev_edge_topo_edges[MAX_NODE];

void rehash_nodes() {
    int u, v;
    edge_num = 1;
    for (int i=0; i<data_num; ++i) {
        if (data_skip[i] == 1) continue;
        u = data[i][0]; v = data[i][1];
        
        edge_v[edge_num] = v;
        edge_ptr[edge_num] = edge_header[u];
        edge_header[u] = edge_num ++; 
    }

    edge_num = 1;
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
    memset(edge_header, 0, sizeof(int) * node_num);
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

int global_counter = 0;
int global_assign[MAX_NODE];

// thread 0

int answer_mt_0[5][MAX_ANSW * 2];
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

void* search_mt_0(void* args) {
    memcpy(edge_topo_starter_mt_0, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_counter, 1);
        if (u >= node_num) break;
        global_assign[u] = -1;
        if (edge_topo_starter_mt_0[u] == edge_topo_header[u+1]) continue;
        if (!searchable_nodes[0][u]) continue;
        global_assign[u] = 0;
        do_search_mt_0(u);
    }
    return NULL;
}

// thread 0

// thread 1

int answer_mt_1[5][MAX_ANSW * 2];
int answer_num_mt_1[5];
int answer_header_mt_1[5][MAX_NODE], answer_tailer_mt_1[5][MAX_NODE];
int total_answer_num_mt_1 = 0;

int edge_topo_starter_mt_1[MAX_NODE];

int visit_mt_1[MAX_NODE], mask_mt_1;
int dfs_path_mt_1[8], dfs_path_num_mt_1;

int front_visit_mt_1[MAX_NODE], front_mask_mt_1;

int back_visit_mt_1[MAX_NODE], back_mask_mt_1;
int back_search_edges_mt_1[MAX_BACK][3];
int back_search_edges_num_mt_1;
int back_search_contains_mt_1[MAX_NODE], back_search_contains_num_mt_1;
int back_search_header_mt_1[MAX_NODE], back_search_ptr_mt_1[MAX_BACK];
int back_search_tailer_mt_1[MAX_NODE];

int back_search_answer_mt_1[2][MAX_BACK][3];
int back_search_answer_num_mt_1[2];

void back_dfs_mt_1() {
    int u, v, tid;
    u = dfs_path_mt_1[dfs_path_num_mt_1 - 1];
    for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
        v = rev_edge_topo_edges[i];
        if (v <= dfs_path_mt_1[0]) break;
        if (visit_mt_1[v] == mask_mt_1) continue;
        if (dfs_path_num_mt_1 == 2) {
            if (front_visit_mt_1[v] == front_mask_mt_1) {
                tid = back_search_answer_num_mt_1[0];
                back_search_answer_mt_1[0][tid][0] = v;
                back_search_answer_mt_1[0][tid][1] = dfs_path_mt_1[1];
                back_search_answer_num_mt_1[0] ++;
            }
        }
        if (dfs_path_num_mt_1 == 3) {
            if (front_visit_mt_1[v] == front_mask_mt_1) {
                tid = back_search_answer_num_mt_1[1];
                back_search_answer_mt_1[1][tid][0] = v;
                back_search_answer_mt_1[1][tid][1] = dfs_path_mt_1[2];
                back_search_answer_mt_1[1][tid][2] = dfs_path_mt_1[1];
                back_search_answer_num_mt_1[1] ++;
            }
            if (back_visit_mt_1[v] != back_mask_mt_1) {
                back_search_header_mt_1[v] = -1;
                back_search_contains_mt_1[back_search_contains_num_mt_1 ++] = v;
            }
            back_visit_mt_1[v] = back_mask_mt_1;

            tid = back_search_edges_num_mt_1;
            back_search_edges_mt_1[tid][0] = dfs_path_mt_1[2];
            back_search_edges_mt_1[tid][1] = dfs_path_mt_1[1];
            back_search_ptr_mt_1[tid] = back_search_header_mt_1[v];
            back_search_header_mt_1[v] = tid;
            back_search_edges_num_mt_1 ++;
            continue;
        }
        visit_mt_1[v] = mask_mt_1;
        dfs_path_mt_1[dfs_path_num_mt_1 ++] = v;
        back_dfs_mt_1();
        visit_mt_1[v] = -1;
        dfs_path_num_mt_1 --;
    }
}

int back_search_id_mt_1[MAX_BACK];

void sort_out_mt_1(int begin_with) {
    int tnum, id;
    if (back_search_answer_num_mt_1[0] > 0) {
        tnum = back_search_answer_num_mt_1[0];
        total_answer_num_mt_1 += tnum;
        memcpy(back_search_id_mt_1, std_id_arr, sizeof(int) * tnum);
        std::sort(back_search_id_mt_1, back_search_id_mt_1 + tnum, [](int i, int j) -> bool{
            if (back_search_answer_mt_1[0][i][0] != back_search_answer_mt_1[0][j][0]) return back_search_answer_mt_1[0][i][0] < back_search_answer_mt_1[0][j][0];
            return back_search_answer_mt_1[0][i][1] < back_search_answer_mt_1[0][j][1];
        });
        answer_header_mt_1[0][begin_with] = answer_num_mt_1[0];
        for (int i=0; i<tnum; ++i) {
            id = back_search_id_mt_1[i];
            answer_mt_1[0][answer_num_mt_1[0] ++] = back_search_answer_mt_1[0][id][0];
            answer_mt_1[0][answer_num_mt_1[0] ++] = back_search_answer_mt_1[0][id][1];
        }
        answer_tailer_mt_1[0][begin_with] = answer_num_mt_1[0];
    }
    // printf("X\n"); fflush(stdout);
    if (back_search_answer_mt_1[1] > 0) {
        tnum = back_search_answer_num_mt_1[1];
        total_answer_num_mt_1 += tnum;
        memcpy(back_search_id_mt_1, std_id_arr, sizeof(int) * tnum);
        std::sort(back_search_id_mt_1, back_search_id_mt_1 + tnum, [](int i, int j) -> bool{
            if (back_search_answer_mt_1[1][i][0] != back_search_answer_mt_1[1][j][0]) return back_search_answer_mt_1[1][i][0] < back_search_answer_mt_1[1][j][0];
            if (back_search_answer_mt_1[1][i][1] != back_search_answer_mt_1[1][j][1]) return back_search_answer_mt_1[1][i][1] < back_search_answer_mt_1[1][j][1];
            return back_search_answer_mt_1[1][i][2] < back_search_answer_mt_1[1][j][2];
        });
        answer_header_mt_1[1][begin_with] = answer_num_mt_1[1];
        for (int i=0; i<tnum; ++i) {
            id = back_search_id_mt_1[i];
            answer_mt_1[1][answer_num_mt_1[1] ++] = back_search_answer_mt_1[1][id][0];
            answer_mt_1[1][answer_num_mt_1[1] ++] = back_search_answer_mt_1[1][id][1];
            answer_mt_1[1][answer_num_mt_1[1] ++] = back_search_answer_mt_1[1][id][2];
        }
        answer_tailer_mt_1[1][begin_with] = answer_num_mt_1[1];
    }
    // printf("Y\n"); fflush(stdout);
    if (back_search_edges_num_mt_1 > 0) {
        int u, last_tnum; tnum = 0;
        for (int i=0; i<back_search_contains_num_mt_1; ++i) {
            u = back_search_contains_mt_1[i];
            last_tnum = tnum;
            for (int j=back_search_header_mt_1[u]; j!=-1; j=back_search_ptr_mt_1[j]) {
                back_search_id_mt_1[tnum ++] = j;
            }
            back_search_header_mt_1[u] = last_tnum;
            back_search_tailer_mt_1[u] = tnum;
            std::sort(back_search_id_mt_1 + last_tnum, back_search_id_mt_1 + tnum, [](int i, int j) -> bool{
                if (back_search_edges_mt_1[i][0] != back_search_edges_mt_1[j][0]) return back_search_edges_mt_1[i][0] < back_search_edges_mt_1[j][0];
                return back_search_edges_mt_1[i][1] < back_search_edges_mt_1[j][1];
            });
        }
    }
    // printf("Z\n"); fflush(stdout);
}

void inline extract_answer_mt_1(int u) {
    int id, x, y;
    for (int i=back_search_header_mt_1[u]; i<back_search_tailer_mt_1[u]; ++i) {
        id = back_search_id_mt_1[i];
        x = back_search_edges_mt_1[id][0]; y = back_search_edges_mt_1[id][1];
        if (visit_mt_1[x] == mask_mt_1 || visit_mt_1[y] == mask_mt_1) continue;
        for (int j=1; j<dfs_path_num_mt_1; ++j) {
            answer_mt_1[dfs_path_num_mt_1][answer_num_mt_1[dfs_path_num_mt_1] ++] = dfs_path_mt_1[j];
        }
        answer_mt_1[dfs_path_num_mt_1][answer_num_mt_1[dfs_path_num_mt_1] ++] = u;
        answer_mt_1[dfs_path_num_mt_1][answer_num_mt_1[dfs_path_num_mt_1] ++] = x;
        answer_mt_1[dfs_path_num_mt_1][answer_num_mt_1[dfs_path_num_mt_1] ++] = y;
        total_answer_num_mt_1 ++;
    }
}

void forw_dfs_mt_1() {
    int u, v;
    u = dfs_path_mt_1[dfs_path_num_mt_1 - 1];
    // printf("search %d =%d\n", u, dfs_path_num_mt_1); fflush(stdout);
    while (edge_topo_starter_mt_1[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_1[u]] <= dfs_path_mt_1[0]) edge_topo_starter_mt_1[u] ++;
    for (int i=edge_topo_starter_mt_1[u]; i<edge_topo_header[u+1]; ++i) {
        // printf("here: %d\n", i); fflush(stdout);
        v = edge_topo_edges[i];
        if (visit_mt_1[v] == mask_mt_1) continue;
        if (dfs_path_num_mt_1 >= 2 && dfs_path_num_mt_1 <= 4) {
            if (back_visit_mt_1[v] == back_mask_mt_1) {
                extract_answer_mt_1(v);
            }
        }
        
        if (dfs_path_num_mt_1 == 4) continue;

        visit_mt_1[v] = mask_mt_1;
        dfs_path_mt_1[dfs_path_num_mt_1 ++] = v;
        forw_dfs_mt_1();
        visit_mt_1[v] = -1;
        dfs_path_num_mt_1 --;
    }
}

void do_search_mt_1(int begin_with) {
    front_mask_mt_1 ++;
    while (edge_topo_starter_mt_1[begin_with] < edge_topo_header[begin_with+1] && edge_topo_edges[edge_topo_starter_mt_1[begin_with]] <= begin_with) edge_topo_starter_mt_1[begin_with] ++;
    for (int i=edge_topo_starter_mt_1[begin_with]; i<edge_topo_header[begin_with+1]; ++i) {
        front_visit_mt_1[edge_topo_edges[i]] = front_mask_mt_1;
    }

    back_search_edges_num_mt_1 = back_search_contains_num_mt_1 = 0;
    back_search_answer_num_mt_1[0] = back_search_answer_num_mt_1[1] = 0;
    back_mask_mt_1 ++; back_visit_mt_1[begin_with] = back_mask_mt_1;
    mask_mt_1 ++; visit_mt_1[begin_with] = mask_mt_1;
    dfs_path_mt_1[0] = begin_with; dfs_path_num_mt_1 = 1;
    back_dfs_mt_1();
    // printf("back search over\n"); fflush(stdout);
    sort_out_mt_1(begin_with);
    if (back_search_edges_num_mt_1 == 0) return;
    // printf("back over\n"); fflush(stdout);
    mask_mt_1 ++; visit_mt_1[begin_with] = mask_mt_1;
    dfs_path_mt_1[0] = begin_with; dfs_path_num_mt_1 = 1;

    answer_header_mt_1[2][begin_with] = answer_num_mt_1[2];
    answer_header_mt_1[3][begin_with] = answer_num_mt_1[3];
    answer_header_mt_1[4][begin_with] = answer_num_mt_1[4];
    forw_dfs_mt_1();
    answer_tailer_mt_1[2][begin_with] = answer_num_mt_1[2];
    answer_tailer_mt_1[3][begin_with] = answer_num_mt_1[3];
    answer_tailer_mt_1[4][begin_with] = answer_num_mt_1[4];
    // printf("all over %d\n", total_answer_num_mt_1); fflush(stdout);
}

void* search_mt_1(void* args) {
    memcpy(edge_topo_starter_mt_1, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_counter, 1);
        if (u >= node_num) break;
        global_assign[u] = -1;
        if (edge_topo_starter_mt_1[u] == edge_topo_header[u+1]) continue;
        if (!searchable_nodes[0][u]) continue;
        global_assign[u] = 1;
        do_search_mt_1(u);
    }
    return NULL;
}

// thread 1

// thread 2

int answer_mt_2[5][MAX_ANSW * 2];
int answer_num_mt_2[5];
int answer_header_mt_2[5][MAX_NODE], answer_tailer_mt_2[5][MAX_NODE];
int total_answer_num_mt_2 = 0;

int edge_topo_starter_mt_2[MAX_NODE];

int visit_mt_2[MAX_NODE], mask_mt_2;
int dfs_path_mt_2[8], dfs_path_num_mt_2;

int front_visit_mt_2[MAX_NODE], front_mask_mt_2;

int back_visit_mt_2[MAX_NODE], back_mask_mt_2;
int back_search_edges_mt_2[MAX_BACK][3];
int back_search_edges_num_mt_2;
int back_search_contains_mt_2[MAX_NODE], back_search_contains_num_mt_2;
int back_search_header_mt_2[MAX_NODE], back_search_ptr_mt_2[MAX_BACK];
int back_search_tailer_mt_2[MAX_NODE];

int back_search_answer_mt_2[2][MAX_BACK][3];
int back_search_answer_num_mt_2[2];

void back_dfs_mt_2() {
    int u, v, tid;
    u = dfs_path_mt_2[dfs_path_num_mt_2 - 1];
    for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
        v = rev_edge_topo_edges[i];
        if (v <= dfs_path_mt_2[0]) break;
        if (visit_mt_2[v] == mask_mt_2) continue;
        if (dfs_path_num_mt_2 == 2) {
            if (front_visit_mt_2[v] == front_mask_mt_2) {
                tid = back_search_answer_num_mt_2[0];
                back_search_answer_mt_2[0][tid][0] = v;
                back_search_answer_mt_2[0][tid][1] = dfs_path_mt_2[1];
                back_search_answer_num_mt_2[0] ++;
            }
        }
        if (dfs_path_num_mt_2 == 3) {
            if (front_visit_mt_2[v] == front_mask_mt_2) {
                tid = back_search_answer_num_mt_2[1];
                back_search_answer_mt_2[1][tid][0] = v;
                back_search_answer_mt_2[1][tid][1] = dfs_path_mt_2[2];
                back_search_answer_mt_2[1][tid][2] = dfs_path_mt_2[1];
                back_search_answer_num_mt_2[1] ++;
            }
            if (back_visit_mt_2[v] != back_mask_mt_2) {
                back_search_header_mt_2[v] = -1;
                back_search_contains_mt_2[back_search_contains_num_mt_2 ++] = v;
            }
            back_visit_mt_2[v] = back_mask_mt_2;

            tid = back_search_edges_num_mt_2;
            back_search_edges_mt_2[tid][0] = dfs_path_mt_2[2];
            back_search_edges_mt_2[tid][1] = dfs_path_mt_2[1];
            back_search_ptr_mt_2[tid] = back_search_header_mt_2[v];
            back_search_header_mt_2[v] = tid;
            back_search_edges_num_mt_2 ++;
            continue;
        }
        visit_mt_2[v] = mask_mt_2;
        dfs_path_mt_2[dfs_path_num_mt_2 ++] = v;
        back_dfs_mt_2();
        visit_mt_2[v] = -1;
        dfs_path_num_mt_2 --;
    }
}

int back_search_id_mt_2[MAX_BACK];

void sort_out_mt_2(int begin_with) {
    int tnum, id;
    if (back_search_answer_num_mt_2[0] > 0) {
        tnum = back_search_answer_num_mt_2[0];
        total_answer_num_mt_2 += tnum;
        memcpy(back_search_id_mt_2, std_id_arr, sizeof(int) * tnum);
        std::sort(back_search_id_mt_2, back_search_id_mt_2 + tnum, [](int i, int j) -> bool{
            if (back_search_answer_mt_2[0][i][0] != back_search_answer_mt_2[0][j][0]) return back_search_answer_mt_2[0][i][0] < back_search_answer_mt_2[0][j][0];
            return back_search_answer_mt_2[0][i][1] < back_search_answer_mt_2[0][j][1];
        });
        answer_header_mt_2[0][begin_with] = answer_num_mt_2[0];
        for (int i=0; i<tnum; ++i) {
            id = back_search_id_mt_2[i];
            answer_mt_2[0][answer_num_mt_2[0] ++] = back_search_answer_mt_2[0][id][0];
            answer_mt_2[0][answer_num_mt_2[0] ++] = back_search_answer_mt_2[0][id][1];
        }
        answer_tailer_mt_2[0][begin_with] = answer_num_mt_2[0];
    }
    // printf("X\n"); fflush(stdout);
    if (back_search_answer_mt_2[1] > 0) {
        tnum = back_search_answer_num_mt_2[1];
        total_answer_num_mt_2 += tnum;
        memcpy(back_search_id_mt_2, std_id_arr, sizeof(int) * tnum);
        std::sort(back_search_id_mt_2, back_search_id_mt_2 + tnum, [](int i, int j) -> bool{
            if (back_search_answer_mt_2[1][i][0] != back_search_answer_mt_2[1][j][0]) return back_search_answer_mt_2[1][i][0] < back_search_answer_mt_2[1][j][0];
            if (back_search_answer_mt_2[1][i][1] != back_search_answer_mt_2[1][j][1]) return back_search_answer_mt_2[1][i][1] < back_search_answer_mt_2[1][j][1];
            return back_search_answer_mt_2[1][i][2] < back_search_answer_mt_2[1][j][2];
        });
        answer_header_mt_2[1][begin_with] = answer_num_mt_2[1];
        for (int i=0; i<tnum; ++i) {
            id = back_search_id_mt_2[i];
            answer_mt_2[1][answer_num_mt_2[1] ++] = back_search_answer_mt_2[1][id][0];
            answer_mt_2[1][answer_num_mt_2[1] ++] = back_search_answer_mt_2[1][id][1];
            answer_mt_2[1][answer_num_mt_2[1] ++] = back_search_answer_mt_2[1][id][2];
        }
        answer_tailer_mt_2[1][begin_with] = answer_num_mt_2[1];
    }
    // printf("Y\n"); fflush(stdout);
    if (back_search_edges_num_mt_2 > 0) {
        int u, last_tnum; tnum = 0;
        for (int i=0; i<back_search_contains_num_mt_2; ++i) {
            u = back_search_contains_mt_2[i];
            last_tnum = tnum;
            for (int j=back_search_header_mt_2[u]; j!=-1; j=back_search_ptr_mt_2[j]) {
                back_search_id_mt_2[tnum ++] = j;
            }
            back_search_header_mt_2[u] = last_tnum;
            back_search_tailer_mt_2[u] = tnum;
            std::sort(back_search_id_mt_2 + last_tnum, back_search_id_mt_2 + tnum, [](int i, int j) -> bool{
                if (back_search_edges_mt_2[i][0] != back_search_edges_mt_2[j][0]) return back_search_edges_mt_2[i][0] < back_search_edges_mt_2[j][0];
                return back_search_edges_mt_2[i][1] < back_search_edges_mt_2[j][1];
            });
        }
    }
    // printf("Z\n"); fflush(stdout);
}

void inline extract_answer_mt_2(int u) {
    int id, x, y;
    for (int i=back_search_header_mt_2[u]; i<back_search_tailer_mt_2[u]; ++i) {
        id = back_search_id_mt_2[i];
        x = back_search_edges_mt_2[id][0]; y = back_search_edges_mt_2[id][1];
        if (visit_mt_2[x] == mask_mt_2 || visit_mt_2[y] == mask_mt_2) continue;
        for (int j=1; j<dfs_path_num_mt_2; ++j) {
            answer_mt_2[dfs_path_num_mt_2][answer_num_mt_2[dfs_path_num_mt_2] ++] = dfs_path_mt_2[j];
        }
        answer_mt_2[dfs_path_num_mt_2][answer_num_mt_2[dfs_path_num_mt_2] ++] = u;
        answer_mt_2[dfs_path_num_mt_2][answer_num_mt_2[dfs_path_num_mt_2] ++] = x;
        answer_mt_2[dfs_path_num_mt_2][answer_num_mt_2[dfs_path_num_mt_2] ++] = y;
        total_answer_num_mt_2 ++;
    }
}

void forw_dfs_mt_2() {
    int u, v;
    u = dfs_path_mt_2[dfs_path_num_mt_2 - 1];
    // printf("search %d =%d\n", u, dfs_path_num_mt_2); fflush(stdout);
    while (edge_topo_starter_mt_2[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_2[u]] <= dfs_path_mt_2[0]) edge_topo_starter_mt_2[u] ++;
    for (int i=edge_topo_starter_mt_2[u]; i<edge_topo_header[u+1]; ++i) {
        // printf("here: %d\n", i); fflush(stdout);
        v = edge_topo_edges[i];
        if (visit_mt_2[v] == mask_mt_2) continue;
        if (dfs_path_num_mt_2 >= 2 && dfs_path_num_mt_2 <= 4) {
            if (back_visit_mt_2[v] == back_mask_mt_2) {
                extract_answer_mt_2(v);
            }
        }
        
        if (dfs_path_num_mt_2 == 4) continue;

        visit_mt_2[v] = mask_mt_2;
        dfs_path_mt_2[dfs_path_num_mt_2 ++] = v;
        forw_dfs_mt_2();
        visit_mt_2[v] = -1;
        dfs_path_num_mt_2 --;
    }
}

void do_search_mt_2(int begin_with) {
    front_mask_mt_2 ++;
    while (edge_topo_starter_mt_2[begin_with] < edge_topo_header[begin_with+1] && edge_topo_edges[edge_topo_starter_mt_2[begin_with]] <= begin_with) edge_topo_starter_mt_2[begin_with] ++;
    for (int i=edge_topo_starter_mt_2[begin_with]; i<edge_topo_header[begin_with+1]; ++i) {
        front_visit_mt_2[edge_topo_edges[i]] = front_mask_mt_2;
    }

    back_search_edges_num_mt_2 = back_search_contains_num_mt_2 = 0;
    back_search_answer_num_mt_2[0] = back_search_answer_num_mt_2[1] = 0;
    back_mask_mt_2 ++; back_visit_mt_2[begin_with] = back_mask_mt_2;
    mask_mt_2 ++; visit_mt_2[begin_with] = mask_mt_2;
    dfs_path_mt_2[0] = begin_with; dfs_path_num_mt_2 = 1;
    back_dfs_mt_2();
    // printf("back search over\n"); fflush(stdout);
    sort_out_mt_2(begin_with);
    if (back_search_edges_num_mt_2 == 0) return;
    // printf("back over\n"); fflush(stdout);
    mask_mt_2 ++; visit_mt_2[begin_with] = mask_mt_2;
    dfs_path_mt_2[0] = begin_with; dfs_path_num_mt_2 = 1;

    answer_header_mt_2[2][begin_with] = answer_num_mt_2[2];
    answer_header_mt_2[3][begin_with] = answer_num_mt_2[3];
    answer_header_mt_2[4][begin_with] = answer_num_mt_2[4];
    forw_dfs_mt_2();
    answer_tailer_mt_2[2][begin_with] = answer_num_mt_2[2];
    answer_tailer_mt_2[3][begin_with] = answer_num_mt_2[3];
    answer_tailer_mt_2[4][begin_with] = answer_num_mt_2[4];
    // printf("all over %d\n", total_answer_num_mt_2); fflush(stdout);
}

void* search_mt_2(void* args) {
    memcpy(edge_topo_starter_mt_2, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_counter, 1);
        if (u >= node_num) break;
        global_assign[u] = -1;
        if (edge_topo_starter_mt_2[u] == edge_topo_header[u+1]) continue;
        if (!searchable_nodes[0][u]) continue;
        global_assign[u] = 2;
        do_search_mt_2(u);
    }
    return NULL;
}

// thread 2

// thread 3

int answer_mt_3[5][MAX_ANSW * 2];
int answer_num_mt_3[5];
int answer_header_mt_3[5][MAX_NODE], answer_tailer_mt_3[5][MAX_NODE];
int total_answer_num_mt_3 = 0;

int edge_topo_starter_mt_3[MAX_NODE];

int visit_mt_3[MAX_NODE], mask_mt_3;
int dfs_path_mt_3[8], dfs_path_num_mt_3;

int front_visit_mt_3[MAX_NODE], front_mask_mt_3;

int back_visit_mt_3[MAX_NODE], back_mask_mt_3;
int back_search_edges_mt_3[MAX_BACK][3];
int back_search_edges_num_mt_3;
int back_search_contains_mt_3[MAX_NODE], back_search_contains_num_mt_3;
int back_search_header_mt_3[MAX_NODE], back_search_ptr_mt_3[MAX_BACK];
int back_search_tailer_mt_3[MAX_NODE];

int back_search_answer_mt_3[2][MAX_BACK][3];
int back_search_answer_num_mt_3[2];

void back_dfs_mt_3() {
    int u, v, tid;
    u = dfs_path_mt_3[dfs_path_num_mt_3 - 1];
    for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
        v = rev_edge_topo_edges[i];
        if (v <= dfs_path_mt_3[0]) break;
        if (visit_mt_3[v] == mask_mt_3) continue;
        if (dfs_path_num_mt_3 == 2) {
            if (front_visit_mt_3[v] == front_mask_mt_3) {
                tid = back_search_answer_num_mt_3[0];
                back_search_answer_mt_3[0][tid][0] = v;
                back_search_answer_mt_3[0][tid][1] = dfs_path_mt_3[1];
                back_search_answer_num_mt_3[0] ++;
            }
        }
        if (dfs_path_num_mt_3 == 3) {
            if (front_visit_mt_3[v] == front_mask_mt_3) {
                tid = back_search_answer_num_mt_3[1];
                back_search_answer_mt_3[1][tid][0] = v;
                back_search_answer_mt_3[1][tid][1] = dfs_path_mt_3[2];
                back_search_answer_mt_3[1][tid][2] = dfs_path_mt_3[1];
                back_search_answer_num_mt_3[1] ++;
            }
            if (back_visit_mt_3[v] != back_mask_mt_3) {
                back_search_header_mt_3[v] = -1;
                back_search_contains_mt_3[back_search_contains_num_mt_3 ++] = v;
            }
            back_visit_mt_3[v] = back_mask_mt_3;

            tid = back_search_edges_num_mt_3;
            back_search_edges_mt_3[tid][0] = dfs_path_mt_3[2];
            back_search_edges_mt_3[tid][1] = dfs_path_mt_3[1];
            back_search_ptr_mt_3[tid] = back_search_header_mt_3[v];
            back_search_header_mt_3[v] = tid;
            back_search_edges_num_mt_3 ++;
            continue;
        }
        visit_mt_3[v] = mask_mt_3;
        dfs_path_mt_3[dfs_path_num_mt_3 ++] = v;
        back_dfs_mt_3();
        visit_mt_3[v] = -1;
        dfs_path_num_mt_3 --;
    }
}

int back_search_id_mt_3[MAX_BACK];

void sort_out_mt_3(int begin_with) {
    int tnum, id;
    if (back_search_answer_num_mt_3[0] > 0) {
        tnum = back_search_answer_num_mt_3[0];
        total_answer_num_mt_3 += tnum;
        memcpy(back_search_id_mt_3, std_id_arr, sizeof(int) * tnum);
        std::sort(back_search_id_mt_3, back_search_id_mt_3 + tnum, [](int i, int j) -> bool{
            if (back_search_answer_mt_3[0][i][0] != back_search_answer_mt_3[0][j][0]) return back_search_answer_mt_3[0][i][0] < back_search_answer_mt_3[0][j][0];
            return back_search_answer_mt_3[0][i][1] < back_search_answer_mt_3[0][j][1];
        });
        answer_header_mt_3[0][begin_with] = answer_num_mt_3[0];
        for (int i=0; i<tnum; ++i) {
            id = back_search_id_mt_3[i];
            answer_mt_3[0][answer_num_mt_3[0] ++] = back_search_answer_mt_3[0][id][0];
            answer_mt_3[0][answer_num_mt_3[0] ++] = back_search_answer_mt_3[0][id][1];
        }
        answer_tailer_mt_3[0][begin_with] = answer_num_mt_3[0];
    }
    // printf("X\n"); fflush(stdout);
    if (back_search_answer_mt_3[1] > 0) {
        tnum = back_search_answer_num_mt_3[1];
        total_answer_num_mt_3 += tnum;
        memcpy(back_search_id_mt_3, std_id_arr, sizeof(int) * tnum);
        std::sort(back_search_id_mt_3, back_search_id_mt_3 + tnum, [](int i, int j) -> bool{
            if (back_search_answer_mt_3[1][i][0] != back_search_answer_mt_3[1][j][0]) return back_search_answer_mt_3[1][i][0] < back_search_answer_mt_3[1][j][0];
            if (back_search_answer_mt_3[1][i][1] != back_search_answer_mt_3[1][j][1]) return back_search_answer_mt_3[1][i][1] < back_search_answer_mt_3[1][j][1];
            return back_search_answer_mt_3[1][i][2] < back_search_answer_mt_3[1][j][2];
        });
        answer_header_mt_3[1][begin_with] = answer_num_mt_3[1];
        for (int i=0; i<tnum; ++i) {
            id = back_search_id_mt_3[i];
            answer_mt_3[1][answer_num_mt_3[1] ++] = back_search_answer_mt_3[1][id][0];
            answer_mt_3[1][answer_num_mt_3[1] ++] = back_search_answer_mt_3[1][id][1];
            answer_mt_3[1][answer_num_mt_3[1] ++] = back_search_answer_mt_3[1][id][2];
        }
        answer_tailer_mt_3[1][begin_with] = answer_num_mt_3[1];
    }
    // printf("Y\n"); fflush(stdout);
    if (back_search_edges_num_mt_3 > 0) {
        int u, last_tnum; tnum = 0;
        for (int i=0; i<back_search_contains_num_mt_3; ++i) {
            u = back_search_contains_mt_3[i];
            last_tnum = tnum;
            for (int j=back_search_header_mt_3[u]; j!=-1; j=back_search_ptr_mt_3[j]) {
                back_search_id_mt_3[tnum ++] = j;
            }
            back_search_header_mt_3[u] = last_tnum;
            back_search_tailer_mt_3[u] = tnum;
            std::sort(back_search_id_mt_3 + last_tnum, back_search_id_mt_3 + tnum, [](int i, int j) -> bool{
                if (back_search_edges_mt_3[i][0] != back_search_edges_mt_3[j][0]) return back_search_edges_mt_3[i][0] < back_search_edges_mt_3[j][0];
                return back_search_edges_mt_3[i][1] < back_search_edges_mt_3[j][1];
            });
        }
    }
    // printf("Z\n"); fflush(stdout);
}

void inline extract_answer_mt_3(int u) {
    int id, x, y;
    for (int i=back_search_header_mt_3[u]; i<back_search_tailer_mt_3[u]; ++i) {
        id = back_search_id_mt_3[i];
        x = back_search_edges_mt_3[id][0]; y = back_search_edges_mt_3[id][1];
        if (visit_mt_3[x] == mask_mt_3 || visit_mt_3[y] == mask_mt_3) continue;
        for (int j=1; j<dfs_path_num_mt_3; ++j) {
            answer_mt_3[dfs_path_num_mt_3][answer_num_mt_3[dfs_path_num_mt_3] ++] = dfs_path_mt_3[j];
        }
        answer_mt_3[dfs_path_num_mt_3][answer_num_mt_3[dfs_path_num_mt_3] ++] = u;
        answer_mt_3[dfs_path_num_mt_3][answer_num_mt_3[dfs_path_num_mt_3] ++] = x;
        answer_mt_3[dfs_path_num_mt_3][answer_num_mt_3[dfs_path_num_mt_3] ++] = y;
        total_answer_num_mt_3 ++;
    }
}

void forw_dfs_mt_3() {
    int u, v;
    u = dfs_path_mt_3[dfs_path_num_mt_3 - 1];
    // printf("search %d =%d\n", u, dfs_path_num_mt_3); fflush(stdout);
    while (edge_topo_starter_mt_3[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_3[u]] <= dfs_path_mt_3[0]) edge_topo_starter_mt_3[u] ++;
    for (int i=edge_topo_starter_mt_3[u]; i<edge_topo_header[u+1]; ++i) {
        // printf("here: %d\n", i); fflush(stdout);
        v = edge_topo_edges[i];
        if (visit_mt_3[v] == mask_mt_3) continue;
        if (dfs_path_num_mt_3 >= 2 && dfs_path_num_mt_3 <= 4) {
            if (back_visit_mt_3[v] == back_mask_mt_3) {
                extract_answer_mt_3(v);
            }
        }
        
        if (dfs_path_num_mt_3 == 4) continue;

        visit_mt_3[v] = mask_mt_3;
        dfs_path_mt_3[dfs_path_num_mt_3 ++] = v;
        forw_dfs_mt_3();
        visit_mt_3[v] = -1;
        dfs_path_num_mt_3 --;
    }
}

void do_search_mt_3(int begin_with) {
    front_mask_mt_3 ++;
    while (edge_topo_starter_mt_3[begin_with] < edge_topo_header[begin_with+1] && edge_topo_edges[edge_topo_starter_mt_3[begin_with]] <= begin_with) edge_topo_starter_mt_3[begin_with] ++;
    for (int i=edge_topo_starter_mt_3[begin_with]; i<edge_topo_header[begin_with+1]; ++i) {
        front_visit_mt_3[edge_topo_edges[i]] = front_mask_mt_3;
    }

    back_search_edges_num_mt_3 = back_search_contains_num_mt_3 = 0;
    back_search_answer_num_mt_3[0] = back_search_answer_num_mt_3[1] = 0;
    back_mask_mt_3 ++; back_visit_mt_3[begin_with] = back_mask_mt_3;
    mask_mt_3 ++; visit_mt_3[begin_with] = mask_mt_3;
    dfs_path_mt_3[0] = begin_with; dfs_path_num_mt_3 = 1;
    back_dfs_mt_3();
    // printf("back search over\n"); fflush(stdout);
    sort_out_mt_3(begin_with);
    if (back_search_edges_num_mt_3 == 0) return;
    // printf("back over\n"); fflush(stdout);
    mask_mt_3 ++; visit_mt_3[begin_with] = mask_mt_3;
    dfs_path_mt_3[0] = begin_with; dfs_path_num_mt_3 = 1;

    answer_header_mt_3[2][begin_with] = answer_num_mt_3[2];
    answer_header_mt_3[3][begin_with] = answer_num_mt_3[3];
    answer_header_mt_3[4][begin_with] = answer_num_mt_3[4];
    forw_dfs_mt_3();
    answer_tailer_mt_3[2][begin_with] = answer_num_mt_3[2];
    answer_tailer_mt_3[3][begin_with] = answer_num_mt_3[3];
    answer_tailer_mt_3[4][begin_with] = answer_num_mt_3[4];
    // printf("all over %d\n", total_answer_num_mt_3); fflush(stdout);
}

void* search_mt_3(void* args) {
    memcpy(edge_topo_starter_mt_3, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_counter, 1);
        if (u >= node_num) break;
        global_assign[u] = -1;
        if (edge_topo_starter_mt_3[u] == edge_topo_header[u+1]) continue;
        if (!searchable_nodes[0][u]) continue;
        global_assign[u] = 3;
        do_search_mt_3(u);
    }
    return NULL;
}

// thread 3

// thread 4

int answer_mt_4[5][MAX_ANSW * 2];
int answer_num_mt_4[5];
int answer_header_mt_4[5][MAX_NODE], answer_tailer_mt_4[5][MAX_NODE];
int total_answer_num_mt_4 = 0;

int edge_topo_starter_mt_4[MAX_NODE];

int visit_mt_4[MAX_NODE], mask_mt_4;
int dfs_path_mt_4[8], dfs_path_num_mt_4;

int front_visit_mt_4[MAX_NODE], front_mask_mt_4;

int back_visit_mt_4[MAX_NODE], back_mask_mt_4;
int back_search_edges_mt_4[MAX_BACK][3];
int back_search_edges_num_mt_4;
int back_search_contains_mt_4[MAX_NODE], back_search_contains_num_mt_4;
int back_search_header_mt_4[MAX_NODE], back_search_ptr_mt_4[MAX_BACK];
int back_search_tailer_mt_4[MAX_NODE];

int back_search_answer_mt_4[2][MAX_BACK][3];
int back_search_answer_num_mt_4[2];

void back_dfs_mt_4() {
    int u, v, tid;
    u = dfs_path_mt_4[dfs_path_num_mt_4 - 1];
    for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
        v = rev_edge_topo_edges[i];
        if (v <= dfs_path_mt_4[0]) break;
        if (visit_mt_4[v] == mask_mt_4) continue;
        if (dfs_path_num_mt_4 == 2) {
            if (front_visit_mt_4[v] == front_mask_mt_4) {
                tid = back_search_answer_num_mt_4[0];
                back_search_answer_mt_4[0][tid][0] = v;
                back_search_answer_mt_4[0][tid][1] = dfs_path_mt_4[1];
                back_search_answer_num_mt_4[0] ++;
            }
        }
        if (dfs_path_num_mt_4 == 3) {
            if (front_visit_mt_4[v] == front_mask_mt_4) {
                tid = back_search_answer_num_mt_4[1];
                back_search_answer_mt_4[1][tid][0] = v;
                back_search_answer_mt_4[1][tid][1] = dfs_path_mt_4[2];
                back_search_answer_mt_4[1][tid][2] = dfs_path_mt_4[1];
                back_search_answer_num_mt_4[1] ++;
            }
            if (back_visit_mt_4[v] != back_mask_mt_4) {
                back_search_header_mt_4[v] = -1;
                back_search_contains_mt_4[back_search_contains_num_mt_4 ++] = v;
            }
            back_visit_mt_4[v] = back_mask_mt_4;

            tid = back_search_edges_num_mt_4;
            back_search_edges_mt_4[tid][0] = dfs_path_mt_4[2];
            back_search_edges_mt_4[tid][1] = dfs_path_mt_4[1];
            back_search_ptr_mt_4[tid] = back_search_header_mt_4[v];
            back_search_header_mt_4[v] = tid;
            back_search_edges_num_mt_4 ++;
            continue;
        }
        visit_mt_4[v] = mask_mt_4;
        dfs_path_mt_4[dfs_path_num_mt_4 ++] = v;
        back_dfs_mt_4();
        visit_mt_4[v] = -1;
        dfs_path_num_mt_4 --;
    }
}

int back_search_id_mt_4[MAX_BACK];

void sort_out_mt_4(int begin_with) {
    int tnum, id;
    if (back_search_answer_num_mt_4[0] > 0) {
        tnum = back_search_answer_num_mt_4[0];
        total_answer_num_mt_4 += tnum;
        memcpy(back_search_id_mt_4, std_id_arr, sizeof(int) * tnum);
        std::sort(back_search_id_mt_4, back_search_id_mt_4 + tnum, [](int i, int j) -> bool{
            if (back_search_answer_mt_4[0][i][0] != back_search_answer_mt_4[0][j][0]) return back_search_answer_mt_4[0][i][0] < back_search_answer_mt_4[0][j][0];
            return back_search_answer_mt_4[0][i][1] < back_search_answer_mt_4[0][j][1];
        });
        answer_header_mt_4[0][begin_with] = answer_num_mt_4[0];
        for (int i=0; i<tnum; ++i) {
            id = back_search_id_mt_4[i];
            answer_mt_4[0][answer_num_mt_4[0] ++] = back_search_answer_mt_4[0][id][0];
            answer_mt_4[0][answer_num_mt_4[0] ++] = back_search_answer_mt_4[0][id][1];
        }
        answer_tailer_mt_4[0][begin_with] = answer_num_mt_4[0];
    }
    // printf("X\n"); fflush(stdout);
    if (back_search_answer_mt_4[1] > 0) {
        tnum = back_search_answer_num_mt_4[1];
        total_answer_num_mt_4 += tnum;
        memcpy(back_search_id_mt_4, std_id_arr, sizeof(int) * tnum);
        std::sort(back_search_id_mt_4, back_search_id_mt_4 + tnum, [](int i, int j) -> bool{
            if (back_search_answer_mt_4[1][i][0] != back_search_answer_mt_4[1][j][0]) return back_search_answer_mt_4[1][i][0] < back_search_answer_mt_4[1][j][0];
            if (back_search_answer_mt_4[1][i][1] != back_search_answer_mt_4[1][j][1]) return back_search_answer_mt_4[1][i][1] < back_search_answer_mt_4[1][j][1];
            return back_search_answer_mt_4[1][i][2] < back_search_answer_mt_4[1][j][2];
        });
        answer_header_mt_4[1][begin_with] = answer_num_mt_4[1];
        for (int i=0; i<tnum; ++i) {
            id = back_search_id_mt_4[i];
            answer_mt_4[1][answer_num_mt_4[1] ++] = back_search_answer_mt_4[1][id][0];
            answer_mt_4[1][answer_num_mt_4[1] ++] = back_search_answer_mt_4[1][id][1];
            answer_mt_4[1][answer_num_mt_4[1] ++] = back_search_answer_mt_4[1][id][2];
        }
        answer_tailer_mt_4[1][begin_with] = answer_num_mt_4[1];
    }
    // printf("Y\n"); fflush(stdout);
    if (back_search_edges_num_mt_4 > 0) {
        int u, last_tnum; tnum = 0;
        for (int i=0; i<back_search_contains_num_mt_4; ++i) {
            u = back_search_contains_mt_4[i];
            last_tnum = tnum;
            for (int j=back_search_header_mt_4[u]; j!=-1; j=back_search_ptr_mt_4[j]) {
                back_search_id_mt_4[tnum ++] = j;
            }
            back_search_header_mt_4[u] = last_tnum;
            back_search_tailer_mt_4[u] = tnum;
            std::sort(back_search_id_mt_4 + last_tnum, back_search_id_mt_4 + tnum, [](int i, int j) -> bool{
                if (back_search_edges_mt_4[i][0] != back_search_edges_mt_4[j][0]) return back_search_edges_mt_4[i][0] < back_search_edges_mt_4[j][0];
                return back_search_edges_mt_4[i][1] < back_search_edges_mt_4[j][1];
            });
        }
    }
    // printf("Z\n"); fflush(stdout);
}

void inline extract_answer_mt_4(int u) {
    int id, x, y;
    for (int i=back_search_header_mt_4[u]; i<back_search_tailer_mt_4[u]; ++i) {
        id = back_search_id_mt_4[i];
        x = back_search_edges_mt_4[id][0]; y = back_search_edges_mt_4[id][1];
        if (visit_mt_4[x] == mask_mt_4 || visit_mt_4[y] == mask_mt_4) continue;
        for (int j=1; j<dfs_path_num_mt_4; ++j) {
            answer_mt_4[dfs_path_num_mt_4][answer_num_mt_4[dfs_path_num_mt_4] ++] = dfs_path_mt_4[j];
        }
        answer_mt_4[dfs_path_num_mt_4][answer_num_mt_4[dfs_path_num_mt_4] ++] = u;
        answer_mt_4[dfs_path_num_mt_4][answer_num_mt_4[dfs_path_num_mt_4] ++] = x;
        answer_mt_4[dfs_path_num_mt_4][answer_num_mt_4[dfs_path_num_mt_4] ++] = y;
        total_answer_num_mt_4 ++;
    }
}

void forw_dfs_mt_4() {
    int u, v;
    u = dfs_path_mt_4[dfs_path_num_mt_4 - 1];
    // printf("search %d =%d\n", u, dfs_path_num_mt_4); fflush(stdout);
    while (edge_topo_starter_mt_4[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_4[u]] <= dfs_path_mt_4[0]) edge_topo_starter_mt_4[u] ++;
    for (int i=edge_topo_starter_mt_4[u]; i<edge_topo_header[u+1]; ++i) {
        // printf("here: %d\n", i); fflush(stdout);
        v = edge_topo_edges[i];
        if (visit_mt_4[v] == mask_mt_4) continue;
        if (dfs_path_num_mt_4 >= 2 && dfs_path_num_mt_4 <= 4) {
            if (back_visit_mt_4[v] == back_mask_mt_4) {
                extract_answer_mt_4(v);
            }
        }
        
        if (dfs_path_num_mt_4 == 4) continue;

        visit_mt_4[v] = mask_mt_4;
        dfs_path_mt_4[dfs_path_num_mt_4 ++] = v;
        forw_dfs_mt_4();
        visit_mt_4[v] = -1;
        dfs_path_num_mt_4 --;
    }
}

void do_search_mt_4(int begin_with) {
    front_mask_mt_4 ++;
    while (edge_topo_starter_mt_4[begin_with] < edge_topo_header[begin_with+1] && edge_topo_edges[edge_topo_starter_mt_4[begin_with]] <= begin_with) edge_topo_starter_mt_4[begin_with] ++;
    for (int i=edge_topo_starter_mt_4[begin_with]; i<edge_topo_header[begin_with+1]; ++i) {
        front_visit_mt_4[edge_topo_edges[i]] = front_mask_mt_4;
    }

    back_search_edges_num_mt_4 = back_search_contains_num_mt_4 = 0;
    back_search_answer_num_mt_4[0] = back_search_answer_num_mt_4[1] = 0;
    back_mask_mt_4 ++; back_visit_mt_4[begin_with] = back_mask_mt_4;
    mask_mt_4 ++; visit_mt_4[begin_with] = mask_mt_4;
    dfs_path_mt_4[0] = begin_with; dfs_path_num_mt_4 = 1;
    back_dfs_mt_4();
    // printf("back search over\n"); fflush(stdout);
    sort_out_mt_4(begin_with);
    if (back_search_edges_num_mt_4 == 0) return;
    // printf("back over\n"); fflush(stdout);
    mask_mt_4 ++; visit_mt_4[begin_with] = mask_mt_4;
    dfs_path_mt_4[0] = begin_with; dfs_path_num_mt_4 = 1;

    answer_header_mt_4[2][begin_with] = answer_num_mt_4[2];
    answer_header_mt_4[3][begin_with] = answer_num_mt_4[3];
    answer_header_mt_4[4][begin_with] = answer_num_mt_4[4];
    forw_dfs_mt_4();
    answer_tailer_mt_4[2][begin_with] = answer_num_mt_4[2];
    answer_tailer_mt_4[3][begin_with] = answer_num_mt_4[3];
    answer_tailer_mt_4[4][begin_with] = answer_num_mt_4[4];
    // printf("all over %d\n", total_answer_num_mt_4); fflush(stdout);
}

void* search_mt_4(void* args) {
    memcpy(edge_topo_starter_mt_4, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_counter, 1);
        if (u >= node_num) break;
        global_assign[u] = -1;
        if (edge_topo_starter_mt_4[u] == edge_topo_header[u+1]) continue;
        if (!searchable_nodes[0][u]) continue;
        global_assign[u] = 4;
        do_search_mt_4(u);
    }
    return NULL;
}

// thread 4

// thread 5

int answer_mt_5[5][MAX_ANSW * 2];
int answer_num_mt_5[5];
int answer_header_mt_5[5][MAX_NODE], answer_tailer_mt_5[5][MAX_NODE];
int total_answer_num_mt_5 = 0;

int edge_topo_starter_mt_5[MAX_NODE];

int visit_mt_5[MAX_NODE], mask_mt_5;
int dfs_path_mt_5[8], dfs_path_num_mt_5;

int front_visit_mt_5[MAX_NODE], front_mask_mt_5;

int back_visit_mt_5[MAX_NODE], back_mask_mt_5;
int back_search_edges_mt_5[MAX_BACK][3];
int back_search_edges_num_mt_5;
int back_search_contains_mt_5[MAX_NODE], back_search_contains_num_mt_5;
int back_search_header_mt_5[MAX_NODE], back_search_ptr_mt_5[MAX_BACK];
int back_search_tailer_mt_5[MAX_NODE];

int back_search_answer_mt_5[2][MAX_BACK][3];
int back_search_answer_num_mt_5[2];

void back_dfs_mt_5() {
    int u, v, tid;
    u = dfs_path_mt_5[dfs_path_num_mt_5 - 1];
    for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
        v = rev_edge_topo_edges[i];
        if (v <= dfs_path_mt_5[0]) break;
        if (visit_mt_5[v] == mask_mt_5) continue;
        if (dfs_path_num_mt_5 == 2) {
            if (front_visit_mt_5[v] == front_mask_mt_5) {
                tid = back_search_answer_num_mt_5[0];
                back_search_answer_mt_5[0][tid][0] = v;
                back_search_answer_mt_5[0][tid][1] = dfs_path_mt_5[1];
                back_search_answer_num_mt_5[0] ++;
            }
        }
        if (dfs_path_num_mt_5 == 3) {
            if (front_visit_mt_5[v] == front_mask_mt_5) {
                tid = back_search_answer_num_mt_5[1];
                back_search_answer_mt_5[1][tid][0] = v;
                back_search_answer_mt_5[1][tid][1] = dfs_path_mt_5[2];
                back_search_answer_mt_5[1][tid][2] = dfs_path_mt_5[1];
                back_search_answer_num_mt_5[1] ++;
            }
            if (back_visit_mt_5[v] != back_mask_mt_5) {
                back_search_header_mt_5[v] = -1;
                back_search_contains_mt_5[back_search_contains_num_mt_5 ++] = v;
            }
            back_visit_mt_5[v] = back_mask_mt_5;

            tid = back_search_edges_num_mt_5;
            back_search_edges_mt_5[tid][0] = dfs_path_mt_5[2];
            back_search_edges_mt_5[tid][1] = dfs_path_mt_5[1];
            back_search_ptr_mt_5[tid] = back_search_header_mt_5[v];
            back_search_header_mt_5[v] = tid;
            back_search_edges_num_mt_5 ++;
            continue;
        }
        visit_mt_5[v] = mask_mt_5;
        dfs_path_mt_5[dfs_path_num_mt_5 ++] = v;
        back_dfs_mt_5();
        visit_mt_5[v] = -1;
        dfs_path_num_mt_5 --;
    }
}

int back_search_id_mt_5[MAX_BACK];

void sort_out_mt_5(int begin_with) {
    int tnum, id;
    if (back_search_answer_num_mt_5[0] > 0) {
        tnum = back_search_answer_num_mt_5[0];
        total_answer_num_mt_5 += tnum;
        memcpy(back_search_id_mt_5, std_id_arr, sizeof(int) * tnum);
        std::sort(back_search_id_mt_5, back_search_id_mt_5 + tnum, [](int i, int j) -> bool{
            if (back_search_answer_mt_5[0][i][0] != back_search_answer_mt_5[0][j][0]) return back_search_answer_mt_5[0][i][0] < back_search_answer_mt_5[0][j][0];
            return back_search_answer_mt_5[0][i][1] < back_search_answer_mt_5[0][j][1];
        });
        answer_header_mt_5[0][begin_with] = answer_num_mt_5[0];
        for (int i=0; i<tnum; ++i) {
            id = back_search_id_mt_5[i];
            answer_mt_5[0][answer_num_mt_5[0] ++] = back_search_answer_mt_5[0][id][0];
            answer_mt_5[0][answer_num_mt_5[0] ++] = back_search_answer_mt_5[0][id][1];
        }
        answer_tailer_mt_5[0][begin_with] = answer_num_mt_5[0];
    }
    // printf("X\n"); fflush(stdout);
    if (back_search_answer_mt_5[1] > 0) {
        tnum = back_search_answer_num_mt_5[1];
        total_answer_num_mt_5 += tnum;
        memcpy(back_search_id_mt_5, std_id_arr, sizeof(int) * tnum);
        std::sort(back_search_id_mt_5, back_search_id_mt_5 + tnum, [](int i, int j) -> bool{
            if (back_search_answer_mt_5[1][i][0] != back_search_answer_mt_5[1][j][0]) return back_search_answer_mt_5[1][i][0] < back_search_answer_mt_5[1][j][0];
            if (back_search_answer_mt_5[1][i][1] != back_search_answer_mt_5[1][j][1]) return back_search_answer_mt_5[1][i][1] < back_search_answer_mt_5[1][j][1];
            return back_search_answer_mt_5[1][i][2] < back_search_answer_mt_5[1][j][2];
        });
        answer_header_mt_5[1][begin_with] = answer_num_mt_5[1];
        for (int i=0; i<tnum; ++i) {
            id = back_search_id_mt_5[i];
            answer_mt_5[1][answer_num_mt_5[1] ++] = back_search_answer_mt_5[1][id][0];
            answer_mt_5[1][answer_num_mt_5[1] ++] = back_search_answer_mt_5[1][id][1];
            answer_mt_5[1][answer_num_mt_5[1] ++] = back_search_answer_mt_5[1][id][2];
        }
        answer_tailer_mt_5[1][begin_with] = answer_num_mt_5[1];
    }
    // printf("Y\n"); fflush(stdout);
    if (back_search_edges_num_mt_5 > 0) {
        int u, last_tnum; tnum = 0;
        for (int i=0; i<back_search_contains_num_mt_5; ++i) {
            u = back_search_contains_mt_5[i];
            last_tnum = tnum;
            for (int j=back_search_header_mt_5[u]; j!=-1; j=back_search_ptr_mt_5[j]) {
                back_search_id_mt_5[tnum ++] = j;
            }
            back_search_header_mt_5[u] = last_tnum;
            back_search_tailer_mt_5[u] = tnum;
            std::sort(back_search_id_mt_5 + last_tnum, back_search_id_mt_5 + tnum, [](int i, int j) -> bool{
                if (back_search_edges_mt_5[i][0] != back_search_edges_mt_5[j][0]) return back_search_edges_mt_5[i][0] < back_search_edges_mt_5[j][0];
                return back_search_edges_mt_5[i][1] < back_search_edges_mt_5[j][1];
            });
        }
    }
    // printf("Z\n"); fflush(stdout);
}

void inline extract_answer_mt_5(int u) {
    int id, x, y;
    for (int i=back_search_header_mt_5[u]; i<back_search_tailer_mt_5[u]; ++i) {
        id = back_search_id_mt_5[i];
        x = back_search_edges_mt_5[id][0]; y = back_search_edges_mt_5[id][1];
        if (visit_mt_5[x] == mask_mt_5 || visit_mt_5[y] == mask_mt_5) continue;
        for (int j=1; j<dfs_path_num_mt_5; ++j) {
            answer_mt_5[dfs_path_num_mt_5][answer_num_mt_5[dfs_path_num_mt_5] ++] = dfs_path_mt_5[j];
        }
        answer_mt_5[dfs_path_num_mt_5][answer_num_mt_5[dfs_path_num_mt_5] ++] = u;
        answer_mt_5[dfs_path_num_mt_5][answer_num_mt_5[dfs_path_num_mt_5] ++] = x;
        answer_mt_5[dfs_path_num_mt_5][answer_num_mt_5[dfs_path_num_mt_5] ++] = y;
        total_answer_num_mt_5 ++;
    }
}

void forw_dfs_mt_5() {
    int u, v;
    u = dfs_path_mt_5[dfs_path_num_mt_5 - 1];
    // printf("search %d =%d\n", u, dfs_path_num_mt_5); fflush(stdout);
    while (edge_topo_starter_mt_5[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_5[u]] <= dfs_path_mt_5[0]) edge_topo_starter_mt_5[u] ++;
    for (int i=edge_topo_starter_mt_5[u]; i<edge_topo_header[u+1]; ++i) {
        // printf("here: %d\n", i); fflush(stdout);
        v = edge_topo_edges[i];
        if (visit_mt_5[v] == mask_mt_5) continue;
        if (dfs_path_num_mt_5 >= 2 && dfs_path_num_mt_5 <= 4) {
            if (back_visit_mt_5[v] == back_mask_mt_5) {
                extract_answer_mt_5(v);
            }
        }
        
        if (dfs_path_num_mt_5 == 4) continue;

        visit_mt_5[v] = mask_mt_5;
        dfs_path_mt_5[dfs_path_num_mt_5 ++] = v;
        forw_dfs_mt_5();
        visit_mt_5[v] = -1;
        dfs_path_num_mt_5 --;
    }
}

void do_search_mt_5(int begin_with) {
    front_mask_mt_5 ++;
    while (edge_topo_starter_mt_5[begin_with] < edge_topo_header[begin_with+1] && edge_topo_edges[edge_topo_starter_mt_5[begin_with]] <= begin_with) edge_topo_starter_mt_5[begin_with] ++;
    for (int i=edge_topo_starter_mt_5[begin_with]; i<edge_topo_header[begin_with+1]; ++i) {
        front_visit_mt_5[edge_topo_edges[i]] = front_mask_mt_5;
    }

    back_search_edges_num_mt_5 = back_search_contains_num_mt_5 = 0;
    back_search_answer_num_mt_5[0] = back_search_answer_num_mt_5[1] = 0;
    back_mask_mt_5 ++; back_visit_mt_5[begin_with] = back_mask_mt_5;
    mask_mt_5 ++; visit_mt_5[begin_with] = mask_mt_5;
    dfs_path_mt_5[0] = begin_with; dfs_path_num_mt_5 = 1;
    back_dfs_mt_5();
    // printf("back search over\n"); fflush(stdout);
    sort_out_mt_5(begin_with);
    if (back_search_edges_num_mt_5 == 0) return;
    // printf("back over\n"); fflush(stdout);
    mask_mt_5 ++; visit_mt_5[begin_with] = mask_mt_5;
    dfs_path_mt_5[0] = begin_with; dfs_path_num_mt_5 = 1;

    answer_header_mt_5[2][begin_with] = answer_num_mt_5[2];
    answer_header_mt_5[3][begin_with] = answer_num_mt_5[3];
    answer_header_mt_5[4][begin_with] = answer_num_mt_5[4];
    forw_dfs_mt_5();
    answer_tailer_mt_5[2][begin_with] = answer_num_mt_5[2];
    answer_tailer_mt_5[3][begin_with] = answer_num_mt_5[3];
    answer_tailer_mt_5[4][begin_with] = answer_num_mt_5[4];
    // printf("all over %d\n", total_answer_num_mt_5); fflush(stdout);
}

void* search_mt_5(void* args) {
    memcpy(edge_topo_starter_mt_5, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_counter, 1);
        if (u >= node_num) break;
        global_assign[u] = -1;
        if (edge_topo_starter_mt_5[u] == edge_topo_header[u+1]) continue;
        if (!searchable_nodes[0][u]) continue;
        global_assign[u] = 5;
        do_search_mt_5(u);
    }
    return NULL;
}

// thread 5




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
        if (global_assign[u] == -1) continue;
        switch (global_assign[u])
        {
        case 0:
            for (int i=answer_header_mt_0[id][u]; i<answer_tailer_mt_0[id][u]; i+=unit_size) {
                deserialize_id(buffer, buffer_num, u);
                for (int j=i; j<i+unit_size; ++j) {
                    buffer[buffer_num ++] = ',';
                    deserialize_id(buffer, buffer_num, answer_mt_0[id][j]);
                }
                buffer[buffer_num ++] = '\n';
            }
            break;
        case 1:
            for (int i=answer_header_mt_1[id][u]; i<answer_tailer_mt_1[id][u]; i+=unit_size) {
                deserialize_id(buffer, buffer_num, u);
                for (int j=i; j<i+unit_size; ++j) {
                    buffer[buffer_num ++] = ',';
                    deserialize_id(buffer, buffer_num, answer_mt_1[id][j]);
                }
                buffer[buffer_num ++] = '\n';
            }
            break;
        case 2:
            for (int i=answer_header_mt_2[id][u]; i<answer_tailer_mt_2[id][u]; i+=unit_size) {
                deserialize_id(buffer, buffer_num, u);
                for (int j=i; j<i+unit_size; ++j) {
                    buffer[buffer_num ++] = ',';
                    deserialize_id(buffer, buffer_num, answer_mt_2[id][j]);
                }
                buffer[buffer_num ++] = '\n';
            }
            break;
        case 3:
            for (int i=answer_header_mt_3[id][u]; i<answer_tailer_mt_3[id][u]; i+=unit_size) {
                deserialize_id(buffer, buffer_num, u);
                for (int j=i; j<i+unit_size; ++j) {
                    buffer[buffer_num ++] = ',';
                    deserialize_id(buffer, buffer_num, answer_mt_3[id][j]);
                }
                buffer[buffer_num ++] = '\n';
            }
            break;
        case 4:
            for (int i=answer_header_mt_4[id][u]; i<answer_tailer_mt_4[id][u]; i+=unit_size) {
                deserialize_id(buffer, buffer_num, u);
                for (int j=i; j<i+unit_size; ++j) {
                    buffer[buffer_num ++] = ',';
                    deserialize_id(buffer, buffer_num, answer_mt_4[id][j]);
                }
                buffer[buffer_num ++] = '\n';
            }
            break;
        case 5:
            for (int i=answer_header_mt_5[id][u]; i<answer_tailer_mt_5[id][u]; i+=unit_size) {
                deserialize_id(buffer, buffer_num, u);
                for (int j=i; j<i+unit_size; ++j) {
                    buffer[buffer_num ++] = ',';
                    deserialize_id(buffer, buffer_num, answer_mt_5[id][j]);
                }
                buffer[buffer_num ++] = '\n';
            }
            break;
        default:
            break;
        }
    }
}

char buffer_io_0[ONE_INT_CHAR_SIZE * MAX_ANSW];
int buffer_num_io_0;

void* write_to_disk_io_0(void* args) {
    deserialize_int(buffer_io_0, buffer_num_io_0, answer_num);
    buffer_io_0[buffer_num_io_0 ++] = '\n';
    write_to_disk(buffer_io_0, buffer_num_io_0, 0, 0, node_num);
    write_to_disk(buffer_io_0, buffer_num_io_0, 1, 0, node_num);
    write_to_disk(buffer_io_0, buffer_num_io_0, 2, 0, node_num);
    return NULL;
}

char buffer_io_1[ONE_INT_CHAR_SIZE * MAX_ANSW * 2];
int buffer_num_io_1;

void* write_to_disk_io_1(void* args) {
    write_to_disk(buffer_io_1, buffer_num_io_1, 3, 0, node_num);
    return NULL;
}

char buffer_io_2[ONE_INT_CHAR_SIZE * MAX_ANSW * 3];
int buffer_num_io_2;

void* write_to_disk_io_2(void* args) {
    write_to_disk(buffer_io_2, buffer_num_io_2, 4, 0, node_num / 10);
    return NULL;
}

char buffer_io_3[ONE_INT_CHAR_SIZE * MAX_ANSW * 3];
int buffer_num_io_3;

void* write_to_disk_io_3(void* args) {
    write_to_disk(buffer_io_3, buffer_num_io_3, 4, node_num / 10, node_num/4);
    return NULL;
}

char buffer_io_4[ONE_INT_CHAR_SIZE * MAX_ANSW * 3];
int buffer_num_io_4;

void* write_to_disk_io_4(void* args) {
    write_to_disk(buffer_io_4, buffer_num_io_4, 4, node_num/4, node_num/2);
    return NULL;
}

char buffer_io_5[ONE_INT_CHAR_SIZE * MAX_ANSW * 3];
int buffer_num_io_5;

void* write_to_disk_io_5(void* args) {
    write_to_disk(buffer_io_5, buffer_num_io_5, 4, node_num/2, node_num);
    return NULL;
}

void write_to_disk() {
    size_t rub;
    pthread_t io_thr[6];
    pthread_create(io_thr + 0, NULL, write_to_disk_io_0, NULL);
    pthread_create(io_thr + 1, NULL, write_to_disk_io_1, NULL);
    pthread_create(io_thr + 2, NULL, write_to_disk_io_2, NULL);
    pthread_create(io_thr + 3, NULL, write_to_disk_io_3, NULL);
    pthread_create(io_thr + 4, NULL, write_to_disk_io_4, NULL);
    pthread_create(io_thr + 5, NULL, write_to_disk_io_5, NULL);

    pthread_join(io_thr[0], NULL);    
    rub = write(writer_fd, buffer_io_0, buffer_num_io_0);

    pthread_join(io_thr[1], NULL);    
    rub = write(writer_fd, buffer_io_1, buffer_num_io_1);

    pthread_join(io_thr[2], NULL);    
    rub = write(writer_fd, buffer_io_2, buffer_num_io_2);

    pthread_join(io_thr[3], NULL);    
    rub = write(writer_fd, buffer_io_3, buffer_num_io_3);

    pthread_join(io_thr[4], NULL);    
    rub = write(writer_fd, buffer_io_4, buffer_num_io_4);

    pthread_join(io_thr[5], NULL);    
    rub = write(writer_fd, buffer_io_5, buffer_num_io_5);
}




//------------------ MBODY ---------------------
// 答案是错的
int main() {

    read_input();
    
    rehash_nodes();
    

    filter_searchable_nodes();
    create_integer_buffer();

    sizeof_int_mul_node_num = sizeof(int) * node_num;
    for (int i=0; i<MAX_BACK; ++i) std_id_arr[i] = i;

    pthread_t search_thr[6];
    pthread_create(search_thr + 0, NULL, search_mt_0, NULL);
    pthread_create(search_thr + 1, NULL, search_mt_1, NULL);
    pthread_create(search_thr + 2, NULL, search_mt_2, NULL);
    pthread_create(search_thr + 3, NULL, search_mt_3, NULL);
    pthread_create(search_thr + 4, NULL, search_mt_4, NULL);
    pthread_create(search_thr + 5, NULL, search_mt_5, NULL);
    for (int i=0; i<6; ++i) pthread_join(search_thr[i], NULL);

    answer_num = total_answer_num_mt_0 + total_answer_num_mt_1 + total_answer_num_mt_2 + total_answer_num_mt_3 + total_answer_num_mt_4 + total_answer_num_mt_5;


    create_writer_fd();
    write_to_disk();
    close(writer_fd);

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