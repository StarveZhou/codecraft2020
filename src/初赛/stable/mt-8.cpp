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

#define MAX_SP_DIST       (4)


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
int CACHE_LINE_ALIGN global_search_assignment[MAX_NODE];
bool CACHE_LINE_ALIGN global_search_finish[MAX_NODE];


// search 0

int CACHE_LINE_ALIGN sp_dist_mt_0[MAX_NODE];
int CACHE_LINE_ALIGN shortest_path_now_mt_0 = -1;

int CACHE_LINE_ALIGN depth_mt_0, dfs_path_mt_0[8];
int CACHE_LINE_ALIGN all_answer_mt_0[MAX_ANSW + 1][7];
int CACHE_LINE_ALIGN all_answer_ptr_mt_0[MAX_ANSW];
int CACHE_LINE_ALIGN answer_header_mt_0[5][MAX_NODE];
int CACHE_LINE_ALIGN answer_tail_mt_0[5][MAX_NODE];

int CACHE_LINE_ALIGN visit_mt_0[MAX_NODE];
int CACHE_LINE_ALIGN mask_mt_0 = 1111;

int CACHE_LINE_ALIGN node_queue_mt_0[MAX_NODE];

int CACHE_LINE_ALIGN answer_num_mt_0 = 1, answer_contain_num_mt_0 = 0;
// int CACHE_LINE_ALIGN answer_contain_mt_0[MAX_NODE];

void shortest_path_mt_0(int u) {
    memset(sp_dist_mt_0, -1, sizeof(int) * node_num);
    sp_dist_mt_0[u] = 0; shortest_path_now_mt_0 = u;

    int v, head = 0, tail = 0, height, orig_u = u;
    bool reach_max;
    node_queue_mt_0[tail ++] = u;
    while (unlikely(head < tail)) {
        u = node_queue_mt_0[head ++];
        height = sp_dist_mt_0[u];
        reach_max = height == MAX_SP_DIST-1;
        for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
            v = rev_edge_topo_edges[i];
            if (sp_dist_mt_0[v] != -1) continue;
            if (v <= orig_u) break;
            sp_dist_mt_0[v] = height + 1;
            if (reach_max) continue;
            node_queue_mt_0[tail ++] = v;
        }
    }
}

int CACHE_LINE_ALIGN edge_topo_starter_mt_0[MAX_NODE];
inline int decide_search_start_mt_0(int u) {
    while (edge_topo_starter_mt_0[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_0[u]] < dfs_path_mt_0[0]) {
        edge_topo_starter_mt_0[u] ++;
    }
    return edge_topo_starter_mt_0[u];
}

void extract_answer_mt_0() {
    for (int i=0; i<depth_mt_0; ++i) {
        all_answer_mt_0[answer_num_mt_0][i] = dfs_path_mt_0[i];
    }
    int x = depth_mt_0 - 3, y = dfs_path_mt_0[0];
    if (unlikely(answer_header_mt_0[x][y] == 0)) {
        answer_header_mt_0[x][y] = answer_tail_mt_0[x][y] = answer_num_mt_0;
    } else {
        all_answer_ptr_mt_0[answer_tail_mt_0[x][y]] = answer_num_mt_0;
        answer_tail_mt_0[x][y] = answer_num_mt_0;
    }
    answer_num_mt_0 ++;
}

void do_search_mt_0() {
    int u, v, mid; 
    u = dfs_path_mt_0[depth_mt_0-1];
    for (int i=decide_search_start_mt_0(u); i<edge_topo_header[u+1]; ++i) {
        v = edge_topo_edges[i];
        if (unlikely(v == dfs_path_mt_0[0])) {
            if (depth_mt_0 >= 3 && depth_mt_0 <= 7) extract_answer_mt_0();
            continue;
        }
        // if (v < dfs_path[0]) continue;
        if (visit_mt_0[v] == mask_mt_0) continue;
        if (depth_mt_0 == 7) continue;
        if (likely(depth_mt_0 + MAX_SP_DIST >= 7)) {
            if (unlikely(shortest_path_now_mt_0 != dfs_path_mt_0[0])) shortest_path_mt_0(dfs_path_mt_0[0]);
            if (likely(sp_dist_mt_0[v] == -1 || sp_dist_mt_0[v] + depth_mt_0 > 7)) continue;
        }
        dfs_path_mt_0[depth_mt_0 ++] = v; visit_mt_0[v] = mask_mt_0;
        do_search_mt_0();
        depth_mt_0 --; visit_mt_0[v] = 0;
    }
}

void* search_mt_0(void* args) {
    memcpy(edge_topo_starter_mt_0, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_search_flag, 1);
        if (u >= node_num) break;
        // answer_contain_mt_0[answer_contain_num_mt_0 ++] = u;
        global_search_assignment[u] = 0;
        if (unlikely(edge_header[u] == edge_header[u+1])) continue;
        if (unlikely(!searchable_nodes[0][u])) continue;
        mask_mt_0 ++; visit_mt_0[u] = mask_mt_0;
        depth_mt_0 = 1; dfs_path_mt_0[0] = u;
        do_search_mt_0();
    }
    return NULL;
}

// end thread 0

// search 1

int CACHE_LINE_ALIGN sp_dist_mt_1[MAX_NODE];
int CACHE_LINE_ALIGN shortest_path_now_mt_1 = -1;

int CACHE_LINE_ALIGN depth_mt_1, dfs_path_mt_1[8];
int CACHE_LINE_ALIGN all_answer_mt_1[MAX_ANSW + 1][7];
int CACHE_LINE_ALIGN all_answer_ptr_mt_1[MAX_ANSW];
int CACHE_LINE_ALIGN answer_header_mt_1[5][MAX_NODE];
int CACHE_LINE_ALIGN answer_tail_mt_1[5][MAX_NODE];

int CACHE_LINE_ALIGN visit_mt_1[MAX_NODE];
int CACHE_LINE_ALIGN mask_mt_1 = 1111;

int CACHE_LINE_ALIGN node_queue_mt_1[MAX_NODE];

int CACHE_LINE_ALIGN answer_num_mt_1 = 1, answer_contain_num_mt_1 = 0;
// int CACHE_LINE_ALIGN answer_contain_mt_1[MAX_NODE];

void shortest_path_mt_1(int u) {
    memset(sp_dist_mt_1, -1, sizeof(int) * node_num);
    sp_dist_mt_1[u] = 0; shortest_path_now_mt_1 = u;

    int v, head = 0, tail = 0, height, orig_u = u;
    bool reach_max;
    node_queue_mt_1[tail ++] = u;
    while (unlikely(head < tail)) {
        u = node_queue_mt_1[head ++];
        height = sp_dist_mt_1[u];
        reach_max = height == MAX_SP_DIST-1;
        for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
            v = rev_edge_topo_edges[i];
            if (sp_dist_mt_1[v] != -1) continue;
            if (v <= orig_u) break;
            sp_dist_mt_1[v] = height + 1;
            if (reach_max) continue;
            node_queue_mt_1[tail ++] = v;
        }
    }
}

int CACHE_LINE_ALIGN edge_topo_starter_mt_1[MAX_NODE];
inline int decide_search_start_mt_1(int u) {
    while (edge_topo_starter_mt_1[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_1[u]] < dfs_path_mt_1[0]) {
        edge_topo_starter_mt_1[u] ++;
    }
    return edge_topo_starter_mt_1[u];
}

void extract_answer_mt_1() {
    for (int i=0; i<depth_mt_1; ++i) {
        all_answer_mt_1[answer_num_mt_1][i] = dfs_path_mt_1[i];
    }
    int x = depth_mt_1 - 3, y = dfs_path_mt_1[0];
    if (unlikely(answer_header_mt_1[x][y] == 0)) {
        answer_header_mt_1[x][y] = answer_tail_mt_1[x][y] = answer_num_mt_1;
    } else {
        all_answer_ptr_mt_1[answer_tail_mt_1[x][y]] = answer_num_mt_1;
        answer_tail_mt_1[x][y] = answer_num_mt_1;
    }
    answer_num_mt_1 ++;
}

void do_search_mt_1() {
    int u, v, mid; 
    u = dfs_path_mt_1[depth_mt_1-1];
    for (int i=decide_search_start_mt_1(u); i<edge_topo_header[u+1]; ++i) {
        v = edge_topo_edges[i];
        if (unlikely(v == dfs_path_mt_1[0])) {
            if (depth_mt_1 >= 3 && depth_mt_1 <= 7) extract_answer_mt_1();
            continue;
        }
        // if (v < dfs_path[0]) continue;
        if (visit_mt_1[v] == mask_mt_1) continue;
        if (depth_mt_1 == 7) continue;
        if (likely(depth_mt_1 + MAX_SP_DIST >= 7)) {
            if (unlikely(shortest_path_now_mt_1 != dfs_path_mt_1[0])) shortest_path_mt_1(dfs_path_mt_1[0]);
            if (likely(sp_dist_mt_1[v] == -1 || sp_dist_mt_1[v] + depth_mt_1 > 7)) continue;
        }
        dfs_path_mt_1[depth_mt_1 ++] = v; visit_mt_1[v] = mask_mt_1;
        do_search_mt_1();
        depth_mt_1 --; visit_mt_1[v] = 0;
    }
}

void* search_mt_1(void* args) {
    memcpy(edge_topo_starter_mt_1, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_search_flag, 1);
        if (u >= node_num) break;
        // answer_contain_mt_1[answer_contain_num_mt_1 ++] = u;
        global_search_assignment[u] = 1;
        if (unlikely(edge_header[u] == edge_header[u+1])) continue;
        if (unlikely(!searchable_nodes[0][u])) continue;
        mask_mt_1 ++; visit_mt_1[u] = mask_mt_1;
        depth_mt_1 = 1; dfs_path_mt_1[0] = u;
        do_search_mt_1();
    }
    return NULL;
}

// end thread 1

// search 2

int CACHE_LINE_ALIGN sp_dist_mt_2[MAX_NODE];
int CACHE_LINE_ALIGN shortest_path_now_mt_2 = -1;

int CACHE_LINE_ALIGN depth_mt_2, dfs_path_mt_2[8];
int CACHE_LINE_ALIGN all_answer_mt_2[MAX_ANSW + 1][7];
int CACHE_LINE_ALIGN all_answer_ptr_mt_2[MAX_ANSW];
int CACHE_LINE_ALIGN answer_header_mt_2[5][MAX_NODE];
int CACHE_LINE_ALIGN answer_tail_mt_2[5][MAX_NODE];

int CACHE_LINE_ALIGN visit_mt_2[MAX_NODE];
int CACHE_LINE_ALIGN mask_mt_2 = 1111;

int CACHE_LINE_ALIGN node_queue_mt_2[MAX_NODE];

int CACHE_LINE_ALIGN answer_num_mt_2 = 1, answer_contain_num_mt_2 = 0;
// int CACHE_LINE_ALIGN answer_contain_mt_2[MAX_NODE];

void shortest_path_mt_2(int u) {
    memset(sp_dist_mt_2, -1, sizeof(int) * node_num);
    sp_dist_mt_2[u] = 0; shortest_path_now_mt_2 = u;

    int v, head = 0, tail = 0, height, orig_u = u;
    bool reach_max;
    node_queue_mt_2[tail ++] = u;
    while (unlikely(head < tail)) {
        u = node_queue_mt_2[head ++];
        height = sp_dist_mt_2[u];
        reach_max = height == MAX_SP_DIST-1;
        for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
            v = rev_edge_topo_edges[i];
            if (sp_dist_mt_2[v] != -1) continue;
            if (v <= orig_u) break;
            sp_dist_mt_2[v] = height + 1;
            if (reach_max) continue;
            node_queue_mt_2[tail ++] = v;
        }
    }
}

int CACHE_LINE_ALIGN edge_topo_starter_mt_2[MAX_NODE];
inline int decide_search_start_mt_2(int u) {
    while (edge_topo_starter_mt_2[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_2[u]] < dfs_path_mt_2[0]) {
        edge_topo_starter_mt_2[u] ++;
    }
    return edge_topo_starter_mt_2[u];
}

void extract_answer_mt_2() {
    for (int i=0; i<depth_mt_2; ++i) {
        all_answer_mt_2[answer_num_mt_2][i] = dfs_path_mt_2[i];
    }
    int x = depth_mt_2 - 3, y = dfs_path_mt_2[0];
    if (unlikely(answer_header_mt_2[x][y] == 0)) {
        answer_header_mt_2[x][y] = answer_tail_mt_2[x][y] = answer_num_mt_2;
    } else {
        all_answer_ptr_mt_2[answer_tail_mt_2[x][y]] = answer_num_mt_2;
        answer_tail_mt_2[x][y] = answer_num_mt_2;
    }
    answer_num_mt_2 ++;
}

void do_search_mt_2() {
    int u, v, mid; 
    u = dfs_path_mt_2[depth_mt_2-1];
    for (int i=decide_search_start_mt_2(u); i<edge_topo_header[u+1]; ++i) {
        v = edge_topo_edges[i];
        if (unlikely(v == dfs_path_mt_2[0])) {
            if (depth_mt_2 >= 3 && depth_mt_2 <= 7) extract_answer_mt_2();
            continue;
        }
        // if (v < dfs_path[0]) continue;
        if (visit_mt_2[v] == mask_mt_2) continue;
        if (depth_mt_2 == 7) continue;
        if (likely(depth_mt_2 + MAX_SP_DIST >= 7)) {
            if (unlikely(shortest_path_now_mt_2 != dfs_path_mt_2[0])) shortest_path_mt_2(dfs_path_mt_2[0]);
            if (likely(sp_dist_mt_2[v] == -1 || sp_dist_mt_2[v] + depth_mt_2 > 7)) continue;
        }
        dfs_path_mt_2[depth_mt_2 ++] = v; visit_mt_2[v] = mask_mt_2;
        do_search_mt_2();
        depth_mt_2 --; visit_mt_2[v] = 0;
    }
}

void* search_mt_2(void* args) {
    memcpy(edge_topo_starter_mt_2, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_search_flag, 1);
        if (u >= node_num) break;
        // answer_contain_mt_2[answer_contain_num_mt_2 ++] = u;
        global_search_assignment[u] = 2;
        if (unlikely(edge_header[u] == edge_header[u+1])) continue;
        if (unlikely(!searchable_nodes[0][u])) continue;
        mask_mt_2 ++; visit_mt_2[u] = mask_mt_2;
        depth_mt_2 = 1; dfs_path_mt_2[0] = u;
        do_search_mt_2();
    }
    return NULL;
}

// end thread 2

// search 3

int CACHE_LINE_ALIGN sp_dist_mt_3[MAX_NODE];
int CACHE_LINE_ALIGN shortest_path_now_mt_3 = -1;

int CACHE_LINE_ALIGN depth_mt_3, dfs_path_mt_3[8];
int CACHE_LINE_ALIGN all_answer_mt_3[MAX_ANSW + 1][7];
int CACHE_LINE_ALIGN all_answer_ptr_mt_3[MAX_ANSW];
int CACHE_LINE_ALIGN answer_header_mt_3[5][MAX_NODE];
int CACHE_LINE_ALIGN answer_tail_mt_3[5][MAX_NODE];

int CACHE_LINE_ALIGN visit_mt_3[MAX_NODE];
int CACHE_LINE_ALIGN mask_mt_3 = 1111;

int CACHE_LINE_ALIGN node_queue_mt_3[MAX_NODE];

int CACHE_LINE_ALIGN answer_num_mt_3 = 1, answer_contain_num_mt_3 = 0;
// int CACHE_LINE_ALIGN answer_contain_mt_3[MAX_NODE];

void shortest_path_mt_3(int u) {
    memset(sp_dist_mt_3, -1, sizeof(int) * node_num);
    sp_dist_mt_3[u] = 0; shortest_path_now_mt_3 = u;

    int v, head = 0, tail = 0, height, orig_u = u;
    bool reach_max;
    node_queue_mt_3[tail ++] = u;
    while (unlikely(head < tail)) {
        u = node_queue_mt_3[head ++];
        height = sp_dist_mt_3[u];
        reach_max = height == MAX_SP_DIST-1;
        for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
            v = rev_edge_topo_edges[i];
            if (sp_dist_mt_3[v] != -1) continue;
            if (v <= orig_u) break;
            sp_dist_mt_3[v] = height + 1;
            if (reach_max) continue;
            node_queue_mt_3[tail ++] = v;
        }
    }
}

int CACHE_LINE_ALIGN edge_topo_starter_mt_3[MAX_NODE];
inline int decide_search_start_mt_3(int u) {
    while (edge_topo_starter_mt_3[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_3[u]] < dfs_path_mt_3[0]) {
        edge_topo_starter_mt_3[u] ++;
    }
    return edge_topo_starter_mt_3[u];
}

void extract_answer_mt_3() {
    for (int i=0; i<depth_mt_3; ++i) {
        all_answer_mt_3[answer_num_mt_3][i] = dfs_path_mt_3[i];
    }
    int x = depth_mt_3 - 3, y = dfs_path_mt_3[0];
    if (unlikely(answer_header_mt_3[x][y] == 0)) {
        answer_header_mt_3[x][y] = answer_tail_mt_3[x][y] = answer_num_mt_3;
    } else {
        all_answer_ptr_mt_3[answer_tail_mt_3[x][y]] = answer_num_mt_3;
        answer_tail_mt_3[x][y] = answer_num_mt_3;
    }
    answer_num_mt_3 ++;
}

void do_search_mt_3() {
    int u, v, mid; 
    u = dfs_path_mt_3[depth_mt_3-1];
    for (int i=decide_search_start_mt_3(u); i<edge_topo_header[u+1]; ++i) {
        v = edge_topo_edges[i];
        if (unlikely(v == dfs_path_mt_3[0])) {
            if (depth_mt_3 >= 3 && depth_mt_3 <= 7) extract_answer_mt_3();
            continue;
        }
        // if (v < dfs_path[0]) continue;
        if (visit_mt_3[v] == mask_mt_3) continue;
        if (depth_mt_3 == 7) continue;
        if (likely(depth_mt_3 + MAX_SP_DIST >= 7)) {
            if (unlikely(shortest_path_now_mt_3 != dfs_path_mt_3[0])) shortest_path_mt_3(dfs_path_mt_3[0]);
            if (likely(sp_dist_mt_3[v] == -1 || sp_dist_mt_3[v] + depth_mt_3 > 7)) continue;
        }
        dfs_path_mt_3[depth_mt_3 ++] = v; visit_mt_3[v] = mask_mt_3;
        do_search_mt_3();
        depth_mt_3 --; visit_mt_3[v] = 0;
    }
}

void* search_mt_3(void* args) {
    memcpy(edge_topo_starter_mt_3, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_search_flag, 1);
        if (u >= node_num) break;
        // answer_contain_mt_3[answer_contain_num_mt_3 ++] = u;
        global_search_assignment[u] = 3;
        if (unlikely(edge_header[u] == edge_header[u+1])) continue;
        if (unlikely(!searchable_nodes[0][u])) continue;
        mask_mt_3 ++; visit_mt_3[u] = mask_mt_3;
        depth_mt_3 = 1; dfs_path_mt_3[0] = u;
        do_search_mt_3();
    }
    return NULL;
}

// end thread 3

// search 4

int CACHE_LINE_ALIGN sp_dist_mt_4[MAX_NODE];
int CACHE_LINE_ALIGN shortest_path_now_mt_4 = -1;

int CACHE_LINE_ALIGN depth_mt_4, dfs_path_mt_4[8];
int CACHE_LINE_ALIGN all_answer_mt_4[MAX_ANSW + 1][7];
int CACHE_LINE_ALIGN all_answer_ptr_mt_4[MAX_ANSW];
int CACHE_LINE_ALIGN answer_header_mt_4[5][MAX_NODE];
int CACHE_LINE_ALIGN answer_tail_mt_4[5][MAX_NODE];

int CACHE_LINE_ALIGN visit_mt_4[MAX_NODE];
int CACHE_LINE_ALIGN mask_mt_4 = 1111;

int CACHE_LINE_ALIGN node_queue_mt_4[MAX_NODE];

int CACHE_LINE_ALIGN answer_num_mt_4 = 1, answer_contain_num_mt_4 = 0;
// int CACHE_LINE_ALIGN answer_contain_mt_4[MAX_NODE];

void shortest_path_mt_4(int u) {
    memset(sp_dist_mt_4, -1, sizeof(int) * node_num);
    sp_dist_mt_4[u] = 0; shortest_path_now_mt_4 = u;

    int v, head = 0, tail = 0, height, orig_u = u;
    bool reach_max;
    node_queue_mt_4[tail ++] = u;
    while (unlikely(head < tail)) {
        u = node_queue_mt_4[head ++];
        height = sp_dist_mt_4[u];
        reach_max = height == MAX_SP_DIST-1;
        for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
            v = rev_edge_topo_edges[i];
            if (sp_dist_mt_4[v] != -1) continue;
            if (v <= orig_u) break;
            sp_dist_mt_4[v] = height + 1;
            if (reach_max) continue;
            node_queue_mt_4[tail ++] = v;
        }
    }
}

int CACHE_LINE_ALIGN edge_topo_starter_mt_4[MAX_NODE];
inline int decide_search_start_mt_4(int u) {
    while (edge_topo_starter_mt_4[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_4[u]] < dfs_path_mt_4[0]) {
        edge_topo_starter_mt_4[u] ++;
    }
    return edge_topo_starter_mt_4[u];
}

void extract_answer_mt_4() {
    for (int i=0; i<depth_mt_4; ++i) {
        all_answer_mt_4[answer_num_mt_4][i] = dfs_path_mt_4[i];
    }
    int x = depth_mt_4 - 3, y = dfs_path_mt_4[0];
    if (unlikely(answer_header_mt_4[x][y] == 0)) {
        answer_header_mt_4[x][y] = answer_tail_mt_4[x][y] = answer_num_mt_4;
    } else {
        all_answer_ptr_mt_4[answer_tail_mt_4[x][y]] = answer_num_mt_4;
        answer_tail_mt_4[x][y] = answer_num_mt_4;
    }
    answer_num_mt_4 ++;
}

void do_search_mt_4() {
    int u, v, mid; 
    u = dfs_path_mt_4[depth_mt_4-1];
    for (int i=decide_search_start_mt_4(u); i<edge_topo_header[u+1]; ++i) {
        v = edge_topo_edges[i];
        if (unlikely(v == dfs_path_mt_4[0])) {
            if (depth_mt_4 >= 3 && depth_mt_4 <= 7) extract_answer_mt_4();
            continue;
        }
        // if (v < dfs_path[0]) continue;
        if (visit_mt_4[v] == mask_mt_4) continue;
        if (depth_mt_4 == 7) continue;
        if (likely(depth_mt_4 + MAX_SP_DIST >= 7)) {
            if (unlikely(shortest_path_now_mt_4 != dfs_path_mt_4[0])) shortest_path_mt_4(dfs_path_mt_4[0]);
            if (likely(sp_dist_mt_4[v] == -1 || sp_dist_mt_4[v] + depth_mt_4 > 7)) continue;
        }
        dfs_path_mt_4[depth_mt_4 ++] = v; visit_mt_4[v] = mask_mt_4;
        do_search_mt_4();
        depth_mt_4 --; visit_mt_4[v] = 0;
    }
}

void* search_mt_4(void* args) {
    memcpy(edge_topo_starter_mt_4, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_search_flag, 1);
        if (u >= node_num) break;
        // answer_contain_mt_4[answer_contain_num_mt_4 ++] = u;
        global_search_assignment[u] = 4;
        if (unlikely(edge_header[u] == edge_header[u+1])) continue;
        if (unlikely(!searchable_nodes[0][u])) continue;
        mask_mt_4 ++; visit_mt_4[u] = mask_mt_4;
        depth_mt_4 = 1; dfs_path_mt_4[0] = u;
        do_search_mt_4();
    }
    return NULL;
}

// end thread 4

// search 5

int CACHE_LINE_ALIGN sp_dist_mt_5[MAX_NODE];
int CACHE_LINE_ALIGN shortest_path_now_mt_5 = -1;

int CACHE_LINE_ALIGN depth_mt_5, dfs_path_mt_5[8];
int CACHE_LINE_ALIGN all_answer_mt_5[MAX_ANSW + 1][7];
int CACHE_LINE_ALIGN all_answer_ptr_mt_5[MAX_ANSW];
int CACHE_LINE_ALIGN answer_header_mt_5[5][MAX_NODE];
int CACHE_LINE_ALIGN answer_tail_mt_5[5][MAX_NODE];

int CACHE_LINE_ALIGN visit_mt_5[MAX_NODE];
int CACHE_LINE_ALIGN mask_mt_5 = 1111;

int CACHE_LINE_ALIGN node_queue_mt_5[MAX_NODE];

int CACHE_LINE_ALIGN answer_num_mt_5 = 1, answer_contain_num_mt_5 = 0;
// int CACHE_LINE_ALIGN answer_contain_mt_5[MAX_NODE];

void shortest_path_mt_5(int u) {
    memset(sp_dist_mt_5, -1, sizeof(int) * node_num);
    sp_dist_mt_5[u] = 0; shortest_path_now_mt_5 = u;

    int v, head = 0, tail = 0, height, orig_u = u;
    bool reach_max;
    node_queue_mt_5[tail ++] = u;
    while (unlikely(head < tail)) {
        u = node_queue_mt_5[head ++];
        height = sp_dist_mt_5[u];
        reach_max = height == MAX_SP_DIST-1;
        for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
            v = rev_edge_topo_edges[i];
            if (sp_dist_mt_5[v] != -1) continue;
            if (v <= orig_u) break;
            sp_dist_mt_5[v] = height + 1;
            if (reach_max) continue;
            node_queue_mt_5[tail ++] = v;
        }
    }
}

int CACHE_LINE_ALIGN edge_topo_starter_mt_5[MAX_NODE];
inline int decide_search_start_mt_5(int u) {
    while (edge_topo_starter_mt_5[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_5[u]] < dfs_path_mt_5[0]) {
        edge_topo_starter_mt_5[u] ++;
    }
    return edge_topo_starter_mt_5[u];
}

void extract_answer_mt_5() {
    for (int i=0; i<depth_mt_5; ++i) {
        all_answer_mt_5[answer_num_mt_5][i] = dfs_path_mt_5[i];
    }
    int x = depth_mt_5 - 3, y = dfs_path_mt_5[0];
    if (unlikely(answer_header_mt_5[x][y] == 0)) {
        answer_header_mt_5[x][y] = answer_tail_mt_5[x][y] = answer_num_mt_5;
    } else {
        all_answer_ptr_mt_5[answer_tail_mt_5[x][y]] = answer_num_mt_5;
        answer_tail_mt_5[x][y] = answer_num_mt_5;
    }
    answer_num_mt_5 ++;
}

void do_search_mt_5() {
    int u, v, mid; 
    u = dfs_path_mt_5[depth_mt_5-1];
    for (int i=decide_search_start_mt_5(u); i<edge_topo_header[u+1]; ++i) {
        v = edge_topo_edges[i];
        if (unlikely(v == dfs_path_mt_5[0])) {
            if (depth_mt_5 >= 3 && depth_mt_5 <= 7) extract_answer_mt_5();
            continue;
        }
        // if (v < dfs_path[0]) continue;
        if (visit_mt_5[v] == mask_mt_5) continue;
        if (depth_mt_5 == 7) continue;
        if (likely(depth_mt_5 + MAX_SP_DIST >= 7)) {
            if (unlikely(shortest_path_now_mt_5 != dfs_path_mt_5[0])) shortest_path_mt_5(dfs_path_mt_5[0]);
            if (likely(sp_dist_mt_5[v] == -1 || sp_dist_mt_5[v] + depth_mt_5 > 7)) continue;
        }
        dfs_path_mt_5[depth_mt_5 ++] = v; visit_mt_5[v] = mask_mt_5;
        do_search_mt_5();
        depth_mt_5 --; visit_mt_5[v] = 0;
    }
}

void* search_mt_5(void* args) {
    memcpy(edge_topo_starter_mt_5, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_search_flag, 1);
        if (u >= node_num) break;
        // answer_contain_mt_5[answer_contain_num_mt_5 ++] = u;
        global_search_assignment[u] = 5;
        if (unlikely(edge_header[u] == edge_header[u+1])) continue;
        if (unlikely(!searchable_nodes[0][u])) continue;
        mask_mt_5 ++; visit_mt_5[u] = mask_mt_5;
        depth_mt_5 = 1; dfs_path_mt_5[0] = u;
        do_search_mt_5();
    }
    return NULL;
}

// end thread 5

// search 6

int CACHE_LINE_ALIGN sp_dist_mt_6[MAX_NODE];
int CACHE_LINE_ALIGN shortest_path_now_mt_6 = -1;

int CACHE_LINE_ALIGN depth_mt_6, dfs_path_mt_6[8];
int CACHE_LINE_ALIGN all_answer_mt_6[MAX_ANSW + 1][7];
int CACHE_LINE_ALIGN all_answer_ptr_mt_6[MAX_ANSW];
int CACHE_LINE_ALIGN answer_header_mt_6[5][MAX_NODE];
int CACHE_LINE_ALIGN answer_tail_mt_6[5][MAX_NODE];

int CACHE_LINE_ALIGN visit_mt_6[MAX_NODE];
int CACHE_LINE_ALIGN mask_mt_6 = 1111;

int CACHE_LINE_ALIGN node_queue_mt_6[MAX_NODE];

int CACHE_LINE_ALIGN answer_num_mt_6 = 1, answer_contain_num_mt_6 = 0;
// int CACHE_LINE_ALIGN answer_contain_mt_6[MAX_NODE];

void shortest_path_mt_6(int u) {
    memset(sp_dist_mt_6, -1, sizeof(int) * node_num);
    sp_dist_mt_6[u] = 0; shortest_path_now_mt_6 = u;

    int v, head = 0, tail = 0, height, orig_u = u;
    bool reach_max;
    node_queue_mt_6[tail ++] = u;
    while (unlikely(head < tail)) {
        u = node_queue_mt_6[head ++];
        height = sp_dist_mt_6[u];
        reach_max = height == MAX_SP_DIST-1;
        for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
            v = rev_edge_topo_edges[i];
            if (sp_dist_mt_6[v] != -1) continue;
            if (v <= orig_u) break;
            sp_dist_mt_6[v] = height + 1;
            if (reach_max) continue;
            node_queue_mt_6[tail ++] = v;
        }
    }
}

int CACHE_LINE_ALIGN edge_topo_starter_mt_6[MAX_NODE];
inline int decide_search_start_mt_6(int u) {
    while (edge_topo_starter_mt_6[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_6[u]] < dfs_path_mt_6[0]) {
        edge_topo_starter_mt_6[u] ++;
    }
    return edge_topo_starter_mt_6[u];
}

void extract_answer_mt_6() {
    for (int i=0; i<depth_mt_6; ++i) {
        all_answer_mt_6[answer_num_mt_6][i] = dfs_path_mt_6[i];
    }
    int x = depth_mt_6 - 3, y = dfs_path_mt_6[0];
    if (unlikely(answer_header_mt_6[x][y] == 0)) {
        answer_header_mt_6[x][y] = answer_tail_mt_6[x][y] = answer_num_mt_6;
    } else {
        all_answer_ptr_mt_6[answer_tail_mt_6[x][y]] = answer_num_mt_6;
        answer_tail_mt_6[x][y] = answer_num_mt_6;
    }
    answer_num_mt_6 ++;
}

void do_search_mt_6() {
    int u, v, mid; 
    u = dfs_path_mt_6[depth_mt_6-1];
    for (int i=decide_search_start_mt_6(u); i<edge_topo_header[u+1]; ++i) {
        v = edge_topo_edges[i];
        if (unlikely(v == dfs_path_mt_6[0])) {
            if (depth_mt_6 >= 3 && depth_mt_6 <= 7) extract_answer_mt_6();
            continue;
        }
        // if (v < dfs_path[0]) continue;
        if (visit_mt_6[v] == mask_mt_6) continue;
        if (depth_mt_6 == 7) continue;
        if (likely(depth_mt_6 + MAX_SP_DIST >= 7)) {
            if (unlikely(shortest_path_now_mt_6 != dfs_path_mt_6[0])) shortest_path_mt_6(dfs_path_mt_6[0]);
            if (likely(sp_dist_mt_6[v] == -1 || sp_dist_mt_6[v] + depth_mt_6 > 7)) continue;
        }
        dfs_path_mt_6[depth_mt_6 ++] = v; visit_mt_6[v] = mask_mt_6;
        do_search_mt_6();
        depth_mt_6 --; visit_mt_6[v] = 0;
    }
}

void* search_mt_6(void* args) {
    memcpy(edge_topo_starter_mt_6, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_search_flag, 1);
        if (u >= node_num) break;
        // answer_contain_mt_6[answer_contain_num_mt_6 ++] = u;
        global_search_assignment[u] = 6;
        if (unlikely(edge_header[u] == edge_header[u+1])) continue;
        if (unlikely(!searchable_nodes[0][u])) continue;
        mask_mt_6 ++; visit_mt_6[u] = mask_mt_6;
        depth_mt_6 = 1; dfs_path_mt_6[0] = u;
        do_search_mt_6();
    }
    return NULL;
}

// end thread 6

// search 7

int CACHE_LINE_ALIGN sp_dist_mt_7[MAX_NODE];
int CACHE_LINE_ALIGN shortest_path_now_mt_7 = -1;

int CACHE_LINE_ALIGN depth_mt_7, dfs_path_mt_7[8];
int CACHE_LINE_ALIGN all_answer_mt_7[MAX_ANSW + 1][7];
int CACHE_LINE_ALIGN all_answer_ptr_mt_7[MAX_ANSW];
int CACHE_LINE_ALIGN answer_header_mt_7[5][MAX_NODE];
int CACHE_LINE_ALIGN answer_tail_mt_7[5][MAX_NODE];

int CACHE_LINE_ALIGN visit_mt_7[MAX_NODE];
int CACHE_LINE_ALIGN mask_mt_7 = 1111;

int CACHE_LINE_ALIGN node_queue_mt_7[MAX_NODE];

int CACHE_LINE_ALIGN answer_num_mt_7 = 1, answer_contain_num_mt_7 = 0;
// int CACHE_LINE_ALIGN answer_contain_mt_7[MAX_NODE];

void shortest_path_mt_7(int u) {
    memset(sp_dist_mt_7, -1, sizeof(int) * node_num);
    sp_dist_mt_7[u] = 0; shortest_path_now_mt_7 = u;

    int v, head = 0, tail = 0, height, orig_u = u;
    bool reach_max;
    node_queue_mt_7[tail ++] = u;
    while (unlikely(head < tail)) {
        u = node_queue_mt_7[head ++];
        height = sp_dist_mt_7[u];
        reach_max = height == MAX_SP_DIST-1;
        for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
            v = rev_edge_topo_edges[i];
            if (sp_dist_mt_7[v] != -1) continue;
            if (v <= orig_u) break;
            sp_dist_mt_7[v] = height + 1;
            if (reach_max) continue;
            node_queue_mt_7[tail ++] = v;
        }
    }
}

int CACHE_LINE_ALIGN edge_topo_starter_mt_7[MAX_NODE];
inline int decide_search_start_mt_7(int u) {
    while (edge_topo_starter_mt_7[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_7[u]] < dfs_path_mt_7[0]) {
        edge_topo_starter_mt_7[u] ++;
    }
    return edge_topo_starter_mt_7[u];
}

void extract_answer_mt_7() {
    for (int i=0; i<depth_mt_7; ++i) {
        all_answer_mt_7[answer_num_mt_7][i] = dfs_path_mt_7[i];
    }
    int x = depth_mt_7 - 3, y = dfs_path_mt_7[0];
    if (unlikely(answer_header_mt_7[x][y] == 0)) {
        answer_header_mt_7[x][y] = answer_tail_mt_7[x][y] = answer_num_mt_7;
    } else {
        all_answer_ptr_mt_7[answer_tail_mt_7[x][y]] = answer_num_mt_7;
        answer_tail_mt_7[x][y] = answer_num_mt_7;
    }
    answer_num_mt_7 ++;
}

void do_search_mt_7() {
    int u, v, mid; 
    u = dfs_path_mt_7[depth_mt_7-1];
    for (int i=decide_search_start_mt_7(u); i<edge_topo_header[u+1]; ++i) {
        v = edge_topo_edges[i];
        if (unlikely(v == dfs_path_mt_7[0])) {
            if (depth_mt_7 >= 3 && depth_mt_7 <= 7) extract_answer_mt_7();
            continue;
        }
        // if (v < dfs_path[0]) continue;
        if (visit_mt_7[v] == mask_mt_7) continue;
        if (depth_mt_7 == 7) continue;
        if (likely(depth_mt_7 + MAX_SP_DIST >= 7)) {
            if (unlikely(shortest_path_now_mt_7 != dfs_path_mt_7[0])) shortest_path_mt_7(dfs_path_mt_7[0]);
            if (likely(sp_dist_mt_7[v] == -1 || sp_dist_mt_7[v] + depth_mt_7 > 7)) continue;
        }
        dfs_path_mt_7[depth_mt_7 ++] = v; visit_mt_7[v] = mask_mt_7;
        do_search_mt_7();
        depth_mt_7 --; visit_mt_7[v] = 0;
    }
}

void* search_mt_7(void* args) {
    memcpy(edge_topo_starter_mt_7, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_search_flag, 1);
        if (u >= node_num) break;
        // answer_contain_mt_7[answer_contain_num_mt_7 ++] = u;
        global_search_assignment[u] = 7;
        if (unlikely(edge_header[u] == edge_header[u+1])) continue;
        if (unlikely(!searchable_nodes[0][u])) continue;
        mask_mt_7 ++; visit_mt_7[u] = mask_mt_7;
        depth_mt_7 = 1; dfs_path_mt_7[0] = u;
        do_search_mt_7();
    }
    return NULL;
}

// end thread 7





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




int buffer_index = 0;
const int max_available = (MAX_EDGE * MAX_FOR_EACH_LINE) - 100;

inline void deserialize_int(int x) {
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

inline void deserialize_id(int x) {
    int orginal_index = buffer_index;
    memcpy(buffer + buffer_index, integer_buffer[x], integer_buffer_size[x] * sizeof(char));
    buffer_index += integer_buffer_size[x];
    if (unlikely(buffer_index >= max_available)) {
        int ret_size = write(writer_fd, buffer, buffer_index);
        buffer_index = 0;
    }
}

int answer_num;





// ********************************************************* IO THREAD
// ********************************************************* IO THREAD
// ********************************************************* IO THREAD
// ********************************************************* IO THREAD
// ********************************************************* IO THREAD
// ********************************************************* IO THREAD
// ********************************************************* IO THREAD
// ********************************************************* IO THREAD
// ********************************************************* IO THREAD








#define ONE_INT_CHAR_SIZE          11




// io thread 0
char CACHE_LINE_ALIGN output_buffer_mt_0[ONE_INT_CHAR_SIZE * MAX_ANSW * 4 + 100];
int CACHE_LINE_ALIGN buffer_index_mt_0 = 0;
inline void deserialize_int_mt_0(int x) {
    if (x == 0) {
        output_buffer_mt_0[buffer_index_mt_0 ++] = '0';
        return;
    }
    int orignial_index = buffer_index_mt_0;
    while (x) {
        output_buffer_mt_0[buffer_index_mt_0 ++] = (x % 10) + '0';
        x /= 10;
    }
    for (int i=orignial_index, j=buffer_index_mt_0-1; i<j; ++i, --j) {
        orignial_index = output_buffer_mt_0[i];
        output_buffer_mt_0[i] = output_buffer_mt_0[j];
        output_buffer_mt_0[j] = orignial_index;
    }
}
inline void deserialize_id_mt_0(int x) {
    int orginal_index = buffer_index_mt_0;
    memcpy(output_buffer_mt_0 + buffer_index_mt_0, integer_buffer[x], integer_buffer_size[x] * sizeof(char));
    buffer_index_mt_0 += integer_buffer_size[x];
}
void* deserialize_ans_mt_0(void* args) {
    deserialize_int_mt_0(answer_num);
    output_buffer_mt_0[buffer_index_mt_0 ++] = '\n';

    int x, x_2, x_3;

    x = 0;
    x_2 = x+2, x_3 = x+3;
    for (int y=0; y<node_num; ++y) {
        switch (global_search_assignment[y])
        {
        case 0:
            if (unlikely(answer_header_mt_0[x][y] == 0)) continue;
            for (int i=answer_header_mt_0[x][y]; i!=0; i=all_answer_ptr_mt_0[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_0(all_answer_mt_0[i][j]);
                    output_buffer_mt_0[buffer_index_mt_0 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 1:
            if (unlikely(answer_header_mt_1[x][y] == 0)) continue;
            for (int i=answer_header_mt_1[x][y]; i!=0; i=all_answer_ptr_mt_1[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_0(all_answer_mt_1[i][j]);
                    output_buffer_mt_0[buffer_index_mt_0 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 2:
            if (unlikely(answer_header_mt_2[x][y] == 0)) continue;
            for (int i=answer_header_mt_2[x][y]; i!=0; i=all_answer_ptr_mt_2[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_0(all_answer_mt_2[i][j]);
                    output_buffer_mt_0[buffer_index_mt_0 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 3:
            if (unlikely(answer_header_mt_3[x][y] == 0)) continue;
            for (int i=answer_header_mt_3[x][y]; i!=0; i=all_answer_ptr_mt_3[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_0(all_answer_mt_3[i][j]);
                    output_buffer_mt_0[buffer_index_mt_0 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 4:
            if (unlikely(answer_header_mt_4[x][y] == 0)) continue;
            for (int i=answer_header_mt_4[x][y]; i!=0; i=all_answer_ptr_mt_4[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_0(all_answer_mt_4[i][j]);
                    output_buffer_mt_0[buffer_index_mt_0 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 5:
            if (unlikely(answer_header_mt_5[x][y] == 0)) continue;
            for (int i=answer_header_mt_5[x][y]; i!=0; i=all_answer_ptr_mt_5[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_0(all_answer_mt_5[i][j]);
                    output_buffer_mt_0[buffer_index_mt_0 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 6:
            if (unlikely(answer_header_mt_6[x][y] == 0)) continue;
            for (int i=answer_header_mt_6[x][y]; i!=0; i=all_answer_ptr_mt_6[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_0(all_answer_mt_6[i][j]);
                    output_buffer_mt_0[buffer_index_mt_0 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 7:
            if (unlikely(answer_header_mt_7[x][y] == 0)) continue;
            for (int i=answer_header_mt_7[x][y]; i!=0; i=all_answer_ptr_mt_7[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_0(all_answer_mt_7[i][j]);
                    output_buffer_mt_0[buffer_index_mt_0 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;        
        default:
            break;
        }
    }

    x = 1;
    x_2 = x+2, x_3 = x+3;
    for (int y=0; y<node_num; ++y) {
        switch (global_search_assignment[y])
        {
        case 0:
            if (unlikely(answer_header_mt_0[x][y] == 0)) continue;
            for (int i=answer_header_mt_0[x][y]; i!=0; i=all_answer_ptr_mt_0[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_0(all_answer_mt_0[i][j]);
                    output_buffer_mt_0[buffer_index_mt_0 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 1:
            if (unlikely(answer_header_mt_1[x][y] == 0)) continue;
            for (int i=answer_header_mt_1[x][y]; i!=0; i=all_answer_ptr_mt_1[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_0(all_answer_mt_1[i][j]);
                    output_buffer_mt_0[buffer_index_mt_0 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 2:
            if (unlikely(answer_header_mt_2[x][y] == 0)) continue;
            for (int i=answer_header_mt_2[x][y]; i!=0; i=all_answer_ptr_mt_2[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_0(all_answer_mt_2[i][j]);
                    output_buffer_mt_0[buffer_index_mt_0 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 3:
            if (unlikely(answer_header_mt_3[x][y] == 0)) continue;
            for (int i=answer_header_mt_3[x][y]; i!=0; i=all_answer_ptr_mt_3[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_0(all_answer_mt_3[i][j]);
                    output_buffer_mt_0[buffer_index_mt_0 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 4:
            if (unlikely(answer_header_mt_4[x][y] == 0)) continue;
            for (int i=answer_header_mt_4[x][y]; i!=0; i=all_answer_ptr_mt_4[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_0(all_answer_mt_4[i][j]);
                    output_buffer_mt_0[buffer_index_mt_0 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 5:
            if (unlikely(answer_header_mt_5[x][y] == 0)) continue;
            for (int i=answer_header_mt_5[x][y]; i!=0; i=all_answer_ptr_mt_5[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_0(all_answer_mt_5[i][j]);
                    output_buffer_mt_0[buffer_index_mt_0 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 6:
            if (unlikely(answer_header_mt_6[x][y] == 0)) continue;
            for (int i=answer_header_mt_6[x][y]; i!=0; i=all_answer_ptr_mt_6[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_0(all_answer_mt_6[i][j]);
                    output_buffer_mt_0[buffer_index_mt_0 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 7:
            if (unlikely(answer_header_mt_7[x][y] == 0)) continue;
            for (int i=answer_header_mt_7[x][y]; i!=0; i=all_answer_ptr_mt_7[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_0(all_answer_mt_7[i][j]);
                    output_buffer_mt_0[buffer_index_mt_0 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;        
        default:
            break;
        }
    }
    return NULL;
}
// end io thread 0

// io thread 1
char CACHE_LINE_ALIGN output_buffer_mt_1[ONE_INT_CHAR_SIZE * MAX_ANSW * 4 + 100];
int CACHE_LINE_ALIGN buffer_index_mt_1 = 0;
inline void deserialize_int_mt_1(int x) {
    if (x == 0) {
        output_buffer_mt_1[buffer_index_mt_1 ++] = '0';
        return;
    }
    int orignial_index = buffer_index_mt_1;
    while (x) {
        output_buffer_mt_1[buffer_index_mt_1 ++] = (x % 10) + '0';
        x /= 10;
    }
    for (int i=orignial_index, j=buffer_index_mt_1-1; i<j; ++i, --j) {
        orignial_index = output_buffer_mt_1[i];
        output_buffer_mt_1[i] = output_buffer_mt_1[j];
        output_buffer_mt_1[j] = orignial_index;
    }
}
inline void deserialize_id_mt_1(int x) {
    int orginal_index = buffer_index_mt_1;
    memcpy(output_buffer_mt_1 + buffer_index_mt_1, integer_buffer[x], integer_buffer_size[x] * sizeof(char));
    buffer_index_mt_1 += integer_buffer_size[x];
}
void* deserialize_ans_mt_1(void* args) {
    int x, x_2, x_3;

    x = 2;
    x_2 = x+2, x_3 = x+3;
    for (int y=0; y<node_num; ++y) {
        switch (global_search_assignment[y])
        {
        case 0:
            if (unlikely(answer_header_mt_0[x][y] == 0)) continue;
            for (int i=answer_header_mt_0[x][y]; i!=0; i=all_answer_ptr_mt_0[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_1(all_answer_mt_0[i][j]);
                    output_buffer_mt_1[buffer_index_mt_1 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 1:
            if (unlikely(answer_header_mt_1[x][y] == 0)) continue;
            for (int i=answer_header_mt_1[x][y]; i!=0; i=all_answer_ptr_mt_1[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_1(all_answer_mt_1[i][j]);
                    output_buffer_mt_1[buffer_index_mt_1 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 2:
            if (unlikely(answer_header_mt_2[x][y] == 0)) continue;
            for (int i=answer_header_mt_2[x][y]; i!=0; i=all_answer_ptr_mt_2[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_1(all_answer_mt_2[i][j]);
                    output_buffer_mt_1[buffer_index_mt_1 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 3:
            if (unlikely(answer_header_mt_3[x][y] == 0)) continue;
            for (int i=answer_header_mt_3[x][y]; i!=0; i=all_answer_ptr_mt_3[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_1(all_answer_mt_3[i][j]);
                    output_buffer_mt_1[buffer_index_mt_1 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 4:
            if (unlikely(answer_header_mt_4[x][y] == 0)) continue;
            for (int i=answer_header_mt_4[x][y]; i!=0; i=all_answer_ptr_mt_4[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_1(all_answer_mt_4[i][j]);
                    output_buffer_mt_1[buffer_index_mt_1 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 5:
            if (unlikely(answer_header_mt_5[x][y] == 0)) continue;
            for (int i=answer_header_mt_5[x][y]; i!=0; i=all_answer_ptr_mt_5[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_1(all_answer_mt_5[i][j]);
                    output_buffer_mt_1[buffer_index_mt_1 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 6:
            if (unlikely(answer_header_mt_6[x][y] == 0)) continue;
            for (int i=answer_header_mt_6[x][y]; i!=0; i=all_answer_ptr_mt_6[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_1(all_answer_mt_6[i][j]);
                    output_buffer_mt_1[buffer_index_mt_1 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 7:
            if (unlikely(answer_header_mt_7[x][y] == 0)) continue;
            for (int i=answer_header_mt_7[x][y]; i!=0; i=all_answer_ptr_mt_7[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_1(all_answer_mt_7[i][j]);
                    output_buffer_mt_1[buffer_index_mt_1 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;        
        default:
            break;
        }
    }

    return NULL;
}
// end io thread 1

// io thread 2
char CACHE_LINE_ALIGN output_buffer_mt_2[ONE_INT_CHAR_SIZE * MAX_ANSW * 4 + 100];
int CACHE_LINE_ALIGN buffer_index_mt_2 = 0;
inline void deserialize_int_mt_2(int x) {
    if (x == 0) {
        output_buffer_mt_2[buffer_index_mt_2 ++] = '0';
        return;
    }
    int orignial_index = buffer_index_mt_2;
    while (x) {
        output_buffer_mt_2[buffer_index_mt_2 ++] = (x % 10) + '0';
        x /= 10;
    }
    for (int i=orignial_index, j=buffer_index_mt_2-1; i<j; ++i, --j) {
        orignial_index = output_buffer_mt_2[i];
        output_buffer_mt_2[i] = output_buffer_mt_2[j];
        output_buffer_mt_2[j] = orignial_index;
    }
}
inline void deserialize_id_mt_2(int x) {
    int orginal_index = buffer_index_mt_2;
    memcpy(output_buffer_mt_2 + buffer_index_mt_2, integer_buffer[x], integer_buffer_size[x] * sizeof(char));
    buffer_index_mt_2 += integer_buffer_size[x];
}
void* deserialize_ans_mt_2(void* args) {
    int x, x_2, x_3;
    int begin, end;

    x = 3;
    x_2 = x+2, x_3 = x+3;
    begin = 0; end = node_num/2;
    for (int y=begin; y<end; ++y) {
        switch (global_search_assignment[y])
        {
        case 0:
            if (unlikely(answer_header_mt_0[x][y] == 0)) continue;
            for (int i=answer_header_mt_0[x][y]; i!=0; i=all_answer_ptr_mt_0[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_2(all_answer_mt_0[i][j]);
                    output_buffer_mt_2[buffer_index_mt_2 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 1:
            if (unlikely(answer_header_mt_1[x][y] == 0)) continue;
            for (int i=answer_header_mt_1[x][y]; i!=0; i=all_answer_ptr_mt_1[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_2(all_answer_mt_1[i][j]);
                    output_buffer_mt_2[buffer_index_mt_2 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 2:
            if (unlikely(answer_header_mt_2[x][y] == 0)) continue;
            for (int i=answer_header_mt_2[x][y]; i!=0; i=all_answer_ptr_mt_2[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_2(all_answer_mt_2[i][j]);
                    output_buffer_mt_2[buffer_index_mt_2 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 3:
            if (unlikely(answer_header_mt_3[x][y] == 0)) continue;
            for (int i=answer_header_mt_3[x][y]; i!=0; i=all_answer_ptr_mt_3[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_2(all_answer_mt_3[i][j]);
                    output_buffer_mt_2[buffer_index_mt_2 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 4:
            if (unlikely(answer_header_mt_4[x][y] == 0)) continue;
            for (int i=answer_header_mt_4[x][y]; i!=0; i=all_answer_ptr_mt_4[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_2(all_answer_mt_4[i][j]);
                    output_buffer_mt_2[buffer_index_mt_2 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 5:
            if (unlikely(answer_header_mt_5[x][y] == 0)) continue;
            for (int i=answer_header_mt_5[x][y]; i!=0; i=all_answer_ptr_mt_5[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_2(all_answer_mt_5[i][j]);
                    output_buffer_mt_2[buffer_index_mt_2 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 6:
            if (unlikely(answer_header_mt_6[x][y] == 0)) continue;
            for (int i=answer_header_mt_6[x][y]; i!=0; i=all_answer_ptr_mt_6[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_2(all_answer_mt_6[i][j]);
                    output_buffer_mt_2[buffer_index_mt_2 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 7:
            if (unlikely(answer_header_mt_7[x][y] == 0)) continue;
            for (int i=answer_header_mt_7[x][y]; i!=0; i=all_answer_ptr_mt_7[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_2(all_answer_mt_7[i][j]);
                    output_buffer_mt_2[buffer_index_mt_2 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;        
        default:
            break;
        }
    }

    return NULL;
}
// end io thread 2

// io thread 3
char CACHE_LINE_ALIGN output_buffer_mt_3[ONE_INT_CHAR_SIZE * MAX_ANSW * 4 + 100];
int CACHE_LINE_ALIGN buffer_index_mt_3 = 0;
inline void deserialize_int_mt_3(int x) {
    if (x == 0) {
        output_buffer_mt_3[buffer_index_mt_3 ++] = '0';
        return;
    }
    int orignial_index = buffer_index_mt_3;
    while (x) {
        output_buffer_mt_3[buffer_index_mt_3 ++] = (x % 10) + '0';
        x /= 10;
    }
    for (int i=orignial_index, j=buffer_index_mt_3-1; i<j; ++i, --j) {
        orignial_index = output_buffer_mt_3[i];
        output_buffer_mt_3[i] = output_buffer_mt_3[j];
        output_buffer_mt_3[j] = orignial_index;
    }
}
inline void deserialize_id_mt_3(int x) {
    int orginal_index = buffer_index_mt_3;
    memcpy(output_buffer_mt_3 + buffer_index_mt_3, integer_buffer[x], integer_buffer_size[x] * sizeof(char));
    buffer_index_mt_3 += integer_buffer_size[x];
}
void* deserialize_ans_mt_3(void* args) {
    int x, x_2, x_3;
    int begin, end;

    x = 3;
    x_2 = x+2, x_3 = x+3;
    begin = node_num/2; end = node_num;
    for (int y=begin; y<end; ++y) {
        switch (global_search_assignment[y])
        {
        case 0:
            if (unlikely(answer_header_mt_0[x][y] == 0)) continue;
            for (int i=answer_header_mt_0[x][y]; i!=0; i=all_answer_ptr_mt_0[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_3(all_answer_mt_0[i][j]);
                    output_buffer_mt_3[buffer_index_mt_3 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 1:
            if (unlikely(answer_header_mt_1[x][y] == 0)) continue;
            for (int i=answer_header_mt_1[x][y]; i!=0; i=all_answer_ptr_mt_1[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_3(all_answer_mt_1[i][j]);
                    output_buffer_mt_3[buffer_index_mt_3 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 2:
            if (unlikely(answer_header_mt_2[x][y] == 0)) continue;
            for (int i=answer_header_mt_2[x][y]; i!=0; i=all_answer_ptr_mt_2[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_3(all_answer_mt_2[i][j]);
                    output_buffer_mt_3[buffer_index_mt_3 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 3:
            if (unlikely(answer_header_mt_3[x][y] == 0)) continue;
            for (int i=answer_header_mt_3[x][y]; i!=0; i=all_answer_ptr_mt_3[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_3(all_answer_mt_3[i][j]);
                    output_buffer_mt_3[buffer_index_mt_3 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 4:
            if (unlikely(answer_header_mt_4[x][y] == 0)) continue;
            for (int i=answer_header_mt_4[x][y]; i!=0; i=all_answer_ptr_mt_4[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_3(all_answer_mt_4[i][j]);
                    output_buffer_mt_3[buffer_index_mt_3 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 5:
            if (unlikely(answer_header_mt_5[x][y] == 0)) continue;
            for (int i=answer_header_mt_5[x][y]; i!=0; i=all_answer_ptr_mt_5[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_3(all_answer_mt_5[i][j]);
                    output_buffer_mt_3[buffer_index_mt_3 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 6:
            if (unlikely(answer_header_mt_6[x][y] == 0)) continue;
            for (int i=answer_header_mt_6[x][y]; i!=0; i=all_answer_ptr_mt_6[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_3(all_answer_mt_6[i][j]);
                    output_buffer_mt_3[buffer_index_mt_3 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 7:
            if (unlikely(answer_header_mt_7[x][y] == 0)) continue;
            for (int i=answer_header_mt_7[x][y]; i!=0; i=all_answer_ptr_mt_7[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_3(all_answer_mt_7[i][j]);
                    output_buffer_mt_3[buffer_index_mt_3 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;        
        default:
            break;
        }
    }

    return NULL;
}
// end io thread 3

// io thread 4
char CACHE_LINE_ALIGN output_buffer_mt_4[ONE_INT_CHAR_SIZE * MAX_ANSW * 4 + 100];
int CACHE_LINE_ALIGN buffer_index_mt_4 = 0;
inline void deserialize_int_mt_4(int x) {
    if (x == 0) {
        output_buffer_mt_4[buffer_index_mt_4 ++] = '0';
        return;
    }
    int orignial_index = buffer_index_mt_4;
    while (x) {
        output_buffer_mt_4[buffer_index_mt_4 ++] = (x % 10) + '0';
        x /= 10;
    }
    for (int i=orignial_index, j=buffer_index_mt_4-1; i<j; ++i, --j) {
        orignial_index = output_buffer_mt_4[i];
        output_buffer_mt_4[i] = output_buffer_mt_4[j];
        output_buffer_mt_4[j] = orignial_index;
    }
}
inline void deserialize_id_mt_4(int x) {
    int orginal_index = buffer_index_mt_4;
    memcpy(output_buffer_mt_4 + buffer_index_mt_4, integer_buffer[x], integer_buffer_size[x] * sizeof(char));
    buffer_index_mt_4 += integer_buffer_size[x];
}
void* deserialize_ans_mt_4(void* args) {
    int x, x_2, x_3;
    int begin, end;

    x = 4;
    x_2 = x+2, x_3 = x+3;
    begin = 0; end = node_num/8;
    for (int y=begin; y<end; ++y) {
        switch (global_search_assignment[y])
        {
        case 0:
            if (unlikely(answer_header_mt_0[x][y] == 0)) continue;
            for (int i=answer_header_mt_0[x][y]; i!=0; i=all_answer_ptr_mt_0[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_4(all_answer_mt_0[i][j]);
                    output_buffer_mt_4[buffer_index_mt_4 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 1:
            if (unlikely(answer_header_mt_1[x][y] == 0)) continue;
            for (int i=answer_header_mt_1[x][y]; i!=0; i=all_answer_ptr_mt_1[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_4(all_answer_mt_1[i][j]);
                    output_buffer_mt_4[buffer_index_mt_4 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 2:
            if (unlikely(answer_header_mt_2[x][y] == 0)) continue;
            for (int i=answer_header_mt_2[x][y]; i!=0; i=all_answer_ptr_mt_2[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_4(all_answer_mt_2[i][j]);
                    output_buffer_mt_4[buffer_index_mt_4 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 3:
            if (unlikely(answer_header_mt_3[x][y] == 0)) continue;
            for (int i=answer_header_mt_3[x][y]; i!=0; i=all_answer_ptr_mt_3[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_4(all_answer_mt_3[i][j]);
                    output_buffer_mt_4[buffer_index_mt_4 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 4:
            if (unlikely(answer_header_mt_4[x][y] == 0)) continue;
            for (int i=answer_header_mt_4[x][y]; i!=0; i=all_answer_ptr_mt_4[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_4(all_answer_mt_4[i][j]);
                    output_buffer_mt_4[buffer_index_mt_4 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 5:
            if (unlikely(answer_header_mt_5[x][y] == 0)) continue;
            for (int i=answer_header_mt_5[x][y]; i!=0; i=all_answer_ptr_mt_5[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_4(all_answer_mt_5[i][j]);
                    output_buffer_mt_4[buffer_index_mt_4 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 6:
            if (unlikely(answer_header_mt_6[x][y] == 0)) continue;
            for (int i=answer_header_mt_6[x][y]; i!=0; i=all_answer_ptr_mt_6[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_4(all_answer_mt_6[i][j]);
                    output_buffer_mt_4[buffer_index_mt_4 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 7:
            if (unlikely(answer_header_mt_7[x][y] == 0)) continue;
            for (int i=answer_header_mt_7[x][y]; i!=0; i=all_answer_ptr_mt_7[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_4(all_answer_mt_7[i][j]);
                    output_buffer_mt_4[buffer_index_mt_4 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;        
        default:
            break;
        }
    }

    return NULL;
}
// end io thread 4

// io thread 5
char CACHE_LINE_ALIGN output_buffer_mt_5[ONE_INT_CHAR_SIZE * MAX_ANSW * 4 + 100];
int CACHE_LINE_ALIGN buffer_index_mt_5 = 0;
inline void deserialize_int_mt_5(int x) {
    if (x == 0) {
        output_buffer_mt_5[buffer_index_mt_5 ++] = '0';
        return;
    }
    int orignial_index = buffer_index_mt_5;
    while (x) {
        output_buffer_mt_5[buffer_index_mt_5 ++] = (x % 10) + '0';
        x /= 10;
    }
    for (int i=orignial_index, j=buffer_index_mt_5-1; i<j; ++i, --j) {
        orignial_index = output_buffer_mt_5[i];
        output_buffer_mt_5[i] = output_buffer_mt_5[j];
        output_buffer_mt_5[j] = orignial_index;
    }
}
inline void deserialize_id_mt_5(int x) {
    int orginal_index = buffer_index_mt_5;
    memcpy(output_buffer_mt_5 + buffer_index_mt_5, integer_buffer[x], integer_buffer_size[x] * sizeof(char));
    buffer_index_mt_5 += integer_buffer_size[x];
}
void* deserialize_ans_mt_5(void* args) {
    int x, x_2, x_3;
    int begin, end;

    x = 4;
    x_2 = x+2, x_3 = x+3;
    begin = node_num/8; end = node_num*2/8;
    for (int y=begin; y<end; ++y) {
        switch (global_search_assignment[y])
        {
        case 0:
            if (unlikely(answer_header_mt_0[x][y] == 0)) continue;
            for (int i=answer_header_mt_0[x][y]; i!=0; i=all_answer_ptr_mt_0[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_5(all_answer_mt_0[i][j]);
                    output_buffer_mt_5[buffer_index_mt_5 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 1:
            if (unlikely(answer_header_mt_1[x][y] == 0)) continue;
            for (int i=answer_header_mt_1[x][y]; i!=0; i=all_answer_ptr_mt_1[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_5(all_answer_mt_1[i][j]);
                    output_buffer_mt_5[buffer_index_mt_5 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 2:
            if (unlikely(answer_header_mt_2[x][y] == 0)) continue;
            for (int i=answer_header_mt_2[x][y]; i!=0; i=all_answer_ptr_mt_2[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_5(all_answer_mt_2[i][j]);
                    output_buffer_mt_5[buffer_index_mt_5 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 3:
            if (unlikely(answer_header_mt_3[x][y] == 0)) continue;
            for (int i=answer_header_mt_3[x][y]; i!=0; i=all_answer_ptr_mt_3[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_5(all_answer_mt_3[i][j]);
                    output_buffer_mt_5[buffer_index_mt_5 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 4:
            if (unlikely(answer_header_mt_4[x][y] == 0)) continue;
            for (int i=answer_header_mt_4[x][y]; i!=0; i=all_answer_ptr_mt_4[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_5(all_answer_mt_4[i][j]);
                    output_buffer_mt_5[buffer_index_mt_5 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 5:
            if (unlikely(answer_header_mt_5[x][y] == 0)) continue;
            for (int i=answer_header_mt_5[x][y]; i!=0; i=all_answer_ptr_mt_5[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_5(all_answer_mt_5[i][j]);
                    output_buffer_mt_5[buffer_index_mt_5 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 6:
            if (unlikely(answer_header_mt_6[x][y] == 0)) continue;
            for (int i=answer_header_mt_6[x][y]; i!=0; i=all_answer_ptr_mt_6[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_5(all_answer_mt_6[i][j]);
                    output_buffer_mt_5[buffer_index_mt_5 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 7:
            if (unlikely(answer_header_mt_7[x][y] == 0)) continue;
            for (int i=answer_header_mt_7[x][y]; i!=0; i=all_answer_ptr_mt_7[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_5(all_answer_mt_7[i][j]);
                    output_buffer_mt_5[buffer_index_mt_5 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;        
        default:
            break;
        }
    }

    return NULL;
}
// end io thread 5

// io thread 6
char CACHE_LINE_ALIGN output_buffer_mt_6[ONE_INT_CHAR_SIZE * MAX_ANSW * 4 + 100];
int CACHE_LINE_ALIGN buffer_index_mt_6 = 0;
inline void deserialize_int_mt_6(int x) {
    if (x == 0) {
        output_buffer_mt_6[buffer_index_mt_6 ++] = '0';
        return;
    }
    int orignial_index = buffer_index_mt_6;
    while (x) {
        output_buffer_mt_6[buffer_index_mt_6 ++] = (x % 10) + '0';
        x /= 10;
    }
    for (int i=orignial_index, j=buffer_index_mt_6-1; i<j; ++i, --j) {
        orignial_index = output_buffer_mt_6[i];
        output_buffer_mt_6[i] = output_buffer_mt_6[j];
        output_buffer_mt_6[j] = orignial_index;
    }
}
inline void deserialize_id_mt_6(int x) {
    int orginal_index = buffer_index_mt_6;
    memcpy(output_buffer_mt_6 + buffer_index_mt_6, integer_buffer[x], integer_buffer_size[x] * sizeof(char));
    buffer_index_mt_6 += integer_buffer_size[x];
}
void* deserialize_ans_mt_6(void* args) {
    int x, x_2, x_3;
    int begin, end;

    x = 4;
    x_2 = x+2, x_3 = x+3;
    begin = node_num*2/8; end = node_num*3/8;
    for (int y=begin; y<end; ++y) {
        switch (global_search_assignment[y])
        {
        case 0:
            if (unlikely(answer_header_mt_0[x][y] == 0)) continue;
            for (int i=answer_header_mt_0[x][y]; i!=0; i=all_answer_ptr_mt_0[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_6(all_answer_mt_0[i][j]);
                    output_buffer_mt_6[buffer_index_mt_6 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 1:
            if (unlikely(answer_header_mt_1[x][y] == 0)) continue;
            for (int i=answer_header_mt_1[x][y]; i!=0; i=all_answer_ptr_mt_1[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_6(all_answer_mt_1[i][j]);
                    output_buffer_mt_6[buffer_index_mt_6 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 2:
            if (unlikely(answer_header_mt_2[x][y] == 0)) continue;
            for (int i=answer_header_mt_2[x][y]; i!=0; i=all_answer_ptr_mt_2[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_6(all_answer_mt_2[i][j]);
                    output_buffer_mt_6[buffer_index_mt_6 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 3:
            if (unlikely(answer_header_mt_3[x][y] == 0)) continue;
            for (int i=answer_header_mt_3[x][y]; i!=0; i=all_answer_ptr_mt_3[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_6(all_answer_mt_3[i][j]);
                    output_buffer_mt_6[buffer_index_mt_6 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 4:
            if (unlikely(answer_header_mt_4[x][y] == 0)) continue;
            for (int i=answer_header_mt_4[x][y]; i!=0; i=all_answer_ptr_mt_4[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_6(all_answer_mt_4[i][j]);
                    output_buffer_mt_6[buffer_index_mt_6 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 5:
            if (unlikely(answer_header_mt_5[x][y] == 0)) continue;
            for (int i=answer_header_mt_5[x][y]; i!=0; i=all_answer_ptr_mt_5[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_6(all_answer_mt_5[i][j]);
                    output_buffer_mt_6[buffer_index_mt_6 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 6:
            if (unlikely(answer_header_mt_6[x][y] == 0)) continue;
            for (int i=answer_header_mt_6[x][y]; i!=0; i=all_answer_ptr_mt_6[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_6(all_answer_mt_6[i][j]);
                    output_buffer_mt_6[buffer_index_mt_6 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 7:
            if (unlikely(answer_header_mt_7[x][y] == 0)) continue;
            for (int i=answer_header_mt_7[x][y]; i!=0; i=all_answer_ptr_mt_7[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_6(all_answer_mt_7[i][j]);
                    output_buffer_mt_6[buffer_index_mt_6 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;        
        default:
            break;
        }
    }

    return NULL;
}
// end io thread 6

// io thread 7
char CACHE_LINE_ALIGN output_buffer_mt_7[ONE_INT_CHAR_SIZE * MAX_ANSW * 4 + 100];
int CACHE_LINE_ALIGN buffer_index_mt_7 = 0;
inline void deserialize_int_mt_7(int x) {
    if (x == 0) {
        output_buffer_mt_7[buffer_index_mt_7 ++] = '0';
        return;
    }
    int orignial_index = buffer_index_mt_7;
    while (x) {
        output_buffer_mt_7[buffer_index_mt_7 ++] = (x % 10) + '0';
        x /= 10;
    }
    for (int i=orignial_index, j=buffer_index_mt_7-1; i<j; ++i, --j) {
        orignial_index = output_buffer_mt_7[i];
        output_buffer_mt_7[i] = output_buffer_mt_7[j];
        output_buffer_mt_7[j] = orignial_index;
    }
}
inline void deserialize_id_mt_7(int x) {
    int orginal_index = buffer_index_mt_7;
    memcpy(output_buffer_mt_7 + buffer_index_mt_7, integer_buffer[x], integer_buffer_size[x] * sizeof(char));
    buffer_index_mt_7 += integer_buffer_size[x];
}
void* deserialize_ans_mt_7(void* args) {
    int x, x_2, x_3;
    int begin, end;

    x = 4;
    x_2 = x+2, x_3 = x+3;
    begin = node_num*3/8; end = node_num*4/8;
    for (int y=begin; y<end; ++y) {
        switch (global_search_assignment[y])
        {
        case 0:
            if (unlikely(answer_header_mt_0[x][y] == 0)) continue;
            for (int i=answer_header_mt_0[x][y]; i!=0; i=all_answer_ptr_mt_0[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_7(all_answer_mt_0[i][j]);
                    output_buffer_mt_7[buffer_index_mt_7 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 1:
            if (unlikely(answer_header_mt_1[x][y] == 0)) continue;
            for (int i=answer_header_mt_1[x][y]; i!=0; i=all_answer_ptr_mt_1[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_7(all_answer_mt_1[i][j]);
                    output_buffer_mt_7[buffer_index_mt_7 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 2:
            if (unlikely(answer_header_mt_2[x][y] == 0)) continue;
            for (int i=answer_header_mt_2[x][y]; i!=0; i=all_answer_ptr_mt_2[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_7(all_answer_mt_2[i][j]);
                    output_buffer_mt_7[buffer_index_mt_7 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 3:
            if (unlikely(answer_header_mt_3[x][y] == 0)) continue;
            for (int i=answer_header_mt_3[x][y]; i!=0; i=all_answer_ptr_mt_3[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_7(all_answer_mt_3[i][j]);
                    output_buffer_mt_7[buffer_index_mt_7 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 4:
            if (unlikely(answer_header_mt_4[x][y] == 0)) continue;
            for (int i=answer_header_mt_4[x][y]; i!=0; i=all_answer_ptr_mt_4[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_7(all_answer_mt_4[i][j]);
                    output_buffer_mt_7[buffer_index_mt_7 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 5:
            if (unlikely(answer_header_mt_5[x][y] == 0)) continue;
            for (int i=answer_header_mt_5[x][y]; i!=0; i=all_answer_ptr_mt_5[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_7(all_answer_mt_5[i][j]);
                    output_buffer_mt_7[buffer_index_mt_7 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 6:
            if (unlikely(answer_header_mt_6[x][y] == 0)) continue;
            for (int i=answer_header_mt_6[x][y]; i!=0; i=all_answer_ptr_mt_6[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_7(all_answer_mt_6[i][j]);
                    output_buffer_mt_7[buffer_index_mt_7 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 7:
            if (unlikely(answer_header_mt_7[x][y] == 0)) continue;
            for (int i=answer_header_mt_7[x][y]; i!=0; i=all_answer_ptr_mt_7[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_7(all_answer_mt_7[i][j]);
                    output_buffer_mt_7[buffer_index_mt_7 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;        
        default:
            break;
        }
    }

    return NULL;
}
// end io thread 7

// io thread 8
char CACHE_LINE_ALIGN output_buffer_mt_8[ONE_INT_CHAR_SIZE * MAX_ANSW * 4 + 100];
int CACHE_LINE_ALIGN buffer_index_mt_8 = 0;
inline void deserialize_int_mt_8(int x) {
    if (x == 0) {
        output_buffer_mt_8[buffer_index_mt_8 ++] = '0';
        return;
    }
    int orignial_index = buffer_index_mt_8;
    while (x) {
        output_buffer_mt_8[buffer_index_mt_8 ++] = (x % 10) + '0';
        x /= 10;
    }
    for (int i=orignial_index, j=buffer_index_mt_8-1; i<j; ++i, --j) {
        orignial_index = output_buffer_mt_8[i];
        output_buffer_mt_8[i] = output_buffer_mt_8[j];
        output_buffer_mt_8[j] = orignial_index;
    }
}
inline void deserialize_id_mt_8(int x) {
    int orginal_index = buffer_index_mt_8;
    memcpy(output_buffer_mt_8 + buffer_index_mt_8, integer_buffer[x], integer_buffer_size[x] * sizeof(char));
    buffer_index_mt_8 += integer_buffer_size[x];
}
void* deserialize_ans_mt_8(void* args) {
    int x, x_2, x_3;
    int begin, end;

    x = 4;
    x_2 = x+2, x_3 = x+3;
    begin = node_num*4/8; end = node_num*5/8;
    for (int y=begin; y<end; ++y) {
        switch (global_search_assignment[y])
        {
        case 0:
            if (unlikely(answer_header_mt_0[x][y] == 0)) continue;
            for (int i=answer_header_mt_0[x][y]; i!=0; i=all_answer_ptr_mt_0[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_8(all_answer_mt_0[i][j]);
                    output_buffer_mt_8[buffer_index_mt_8 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 1:
            if (unlikely(answer_header_mt_1[x][y] == 0)) continue;
            for (int i=answer_header_mt_1[x][y]; i!=0; i=all_answer_ptr_mt_1[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_8(all_answer_mt_1[i][j]);
                    output_buffer_mt_8[buffer_index_mt_8 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 2:
            if (unlikely(answer_header_mt_2[x][y] == 0)) continue;
            for (int i=answer_header_mt_2[x][y]; i!=0; i=all_answer_ptr_mt_2[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_8(all_answer_mt_2[i][j]);
                    output_buffer_mt_8[buffer_index_mt_8 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 3:
            if (unlikely(answer_header_mt_3[x][y] == 0)) continue;
            for (int i=answer_header_mt_3[x][y]; i!=0; i=all_answer_ptr_mt_3[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_8(all_answer_mt_3[i][j]);
                    output_buffer_mt_8[buffer_index_mt_8 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 4:
            if (unlikely(answer_header_mt_4[x][y] == 0)) continue;
            for (int i=answer_header_mt_4[x][y]; i!=0; i=all_answer_ptr_mt_4[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_8(all_answer_mt_4[i][j]);
                    output_buffer_mt_8[buffer_index_mt_8 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 5:
            if (unlikely(answer_header_mt_5[x][y] == 0)) continue;
            for (int i=answer_header_mt_5[x][y]; i!=0; i=all_answer_ptr_mt_5[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_8(all_answer_mt_5[i][j]);
                    output_buffer_mt_8[buffer_index_mt_8 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 6:
            if (unlikely(answer_header_mt_6[x][y] == 0)) continue;
            for (int i=answer_header_mt_6[x][y]; i!=0; i=all_answer_ptr_mt_6[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_8(all_answer_mt_6[i][j]);
                    output_buffer_mt_8[buffer_index_mt_8 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 7:
            if (unlikely(answer_header_mt_7[x][y] == 0)) continue;
            for (int i=answer_header_mt_7[x][y]; i!=0; i=all_answer_ptr_mt_7[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_8(all_answer_mt_7[i][j]);
                    output_buffer_mt_8[buffer_index_mt_8 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;        
        default:
            break;
        }
    }

    return NULL;
}
// end io thread 8

// io thread 9
char CACHE_LINE_ALIGN output_buffer_mt_9[ONE_INT_CHAR_SIZE * MAX_ANSW * 4 + 100];
int CACHE_LINE_ALIGN buffer_index_mt_9 = 0;
inline void deserialize_int_mt_9(int x) {
    if (x == 0) {
        output_buffer_mt_9[buffer_index_mt_9 ++] = '0';
        return;
    }
    int orignial_index = buffer_index_mt_9;
    while (x) {
        output_buffer_mt_9[buffer_index_mt_9 ++] = (x % 10) + '0';
        x /= 10;
    }
    for (int i=orignial_index, j=buffer_index_mt_9-1; i<j; ++i, --j) {
        orignial_index = output_buffer_mt_9[i];
        output_buffer_mt_9[i] = output_buffer_mt_9[j];
        output_buffer_mt_9[j] = orignial_index;
    }
}
inline void deserialize_id_mt_9(int x) {
    int orginal_index = buffer_index_mt_9;
    memcpy(output_buffer_mt_9 + buffer_index_mt_9, integer_buffer[x], integer_buffer_size[x] * sizeof(char));
    buffer_index_mt_9 += integer_buffer_size[x];
}
void* deserialize_ans_mt_9(void* args) {
    int x, x_2, x_3;
    int begin, end;

    x = 4;
    x_2 = x+2, x_3 = x+3;
    begin = node_num*5/8; end = node_num*6/8;
    for (int y=begin; y<end; ++y) {
        switch (global_search_assignment[y])
        {
        case 0:
            if (unlikely(answer_header_mt_0[x][y] == 0)) continue;
            for (int i=answer_header_mt_0[x][y]; i!=0; i=all_answer_ptr_mt_0[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_9(all_answer_mt_0[i][j]);
                    output_buffer_mt_9[buffer_index_mt_9 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 1:
            if (unlikely(answer_header_mt_1[x][y] == 0)) continue;
            for (int i=answer_header_mt_1[x][y]; i!=0; i=all_answer_ptr_mt_1[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_9(all_answer_mt_1[i][j]);
                    output_buffer_mt_9[buffer_index_mt_9 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 2:
            if (unlikely(answer_header_mt_2[x][y] == 0)) continue;
            for (int i=answer_header_mt_2[x][y]; i!=0; i=all_answer_ptr_mt_2[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_9(all_answer_mt_2[i][j]);
                    output_buffer_mt_9[buffer_index_mt_9 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 3:
            if (unlikely(answer_header_mt_3[x][y] == 0)) continue;
            for (int i=answer_header_mt_3[x][y]; i!=0; i=all_answer_ptr_mt_3[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_9(all_answer_mt_3[i][j]);
                    output_buffer_mt_9[buffer_index_mt_9 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 4:
            if (unlikely(answer_header_mt_4[x][y] == 0)) continue;
            for (int i=answer_header_mt_4[x][y]; i!=0; i=all_answer_ptr_mt_4[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_9(all_answer_mt_4[i][j]);
                    output_buffer_mt_9[buffer_index_mt_9 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 5:
            if (unlikely(answer_header_mt_5[x][y] == 0)) continue;
            for (int i=answer_header_mt_5[x][y]; i!=0; i=all_answer_ptr_mt_5[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_9(all_answer_mt_5[i][j]);
                    output_buffer_mt_9[buffer_index_mt_9 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 6:
            if (unlikely(answer_header_mt_6[x][y] == 0)) continue;
            for (int i=answer_header_mt_6[x][y]; i!=0; i=all_answer_ptr_mt_6[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_9(all_answer_mt_6[i][j]);
                    output_buffer_mt_9[buffer_index_mt_9 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 7:
            if (unlikely(answer_header_mt_7[x][y] == 0)) continue;
            for (int i=answer_header_mt_7[x][y]; i!=0; i=all_answer_ptr_mt_7[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_9(all_answer_mt_7[i][j]);
                    output_buffer_mt_9[buffer_index_mt_9 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;        
        default:
            break;
        }
    }

    return NULL;
}
// end io thread 9

// io thread 10
char CACHE_LINE_ALIGN output_buffer_mt_10[ONE_INT_CHAR_SIZE * MAX_ANSW * 4 + 100];
int CACHE_LINE_ALIGN buffer_index_mt_10 = 0;
inline void deserialize_int_mt_10(int x) {
    if (x == 0) {
        output_buffer_mt_10[buffer_index_mt_10 ++] = '0';
        return;
    }
    int orignial_index = buffer_index_mt_10;
    while (x) {
        output_buffer_mt_10[buffer_index_mt_10 ++] = (x % 10) + '0';
        x /= 10;
    }
    for (int i=orignial_index, j=buffer_index_mt_10-1; i<j; ++i, --j) {
        orignial_index = output_buffer_mt_10[i];
        output_buffer_mt_10[i] = output_buffer_mt_10[j];
        output_buffer_mt_10[j] = orignial_index;
    }
}
inline void deserialize_id_mt_10(int x) {
    int orginal_index = buffer_index_mt_10;
    memcpy(output_buffer_mt_10 + buffer_index_mt_10, integer_buffer[x], integer_buffer_size[x] * sizeof(char));
    buffer_index_mt_10 += integer_buffer_size[x];
}
void* deserialize_ans_mt_10(void* args) {
    int x, x_2, x_3;
    int begin, end;

    x = 4;
    x_2 = x+2, x_3 = x+3;
    begin = node_num*6/8; end = node_num*7/8;
    for (int y=begin; y<end; ++y) {
        switch (global_search_assignment[y])
        {
        case 0:
            if (unlikely(answer_header_mt_0[x][y] == 0)) continue;
            for (int i=answer_header_mt_0[x][y]; i!=0; i=all_answer_ptr_mt_0[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_10(all_answer_mt_0[i][j]);
                    output_buffer_mt_10[buffer_index_mt_10 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 1:
            if (unlikely(answer_header_mt_1[x][y] == 0)) continue;
            for (int i=answer_header_mt_1[x][y]; i!=0; i=all_answer_ptr_mt_1[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_10(all_answer_mt_1[i][j]);
                    output_buffer_mt_10[buffer_index_mt_10 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 2:
            if (unlikely(answer_header_mt_2[x][y] == 0)) continue;
            for (int i=answer_header_mt_2[x][y]; i!=0; i=all_answer_ptr_mt_2[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_10(all_answer_mt_2[i][j]);
                    output_buffer_mt_10[buffer_index_mt_10 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 3:
            if (unlikely(answer_header_mt_3[x][y] == 0)) continue;
            for (int i=answer_header_mt_3[x][y]; i!=0; i=all_answer_ptr_mt_3[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_10(all_answer_mt_3[i][j]);
                    output_buffer_mt_10[buffer_index_mt_10 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 4:
            if (unlikely(answer_header_mt_4[x][y] == 0)) continue;
            for (int i=answer_header_mt_4[x][y]; i!=0; i=all_answer_ptr_mt_4[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_10(all_answer_mt_4[i][j]);
                    output_buffer_mt_10[buffer_index_mt_10 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 5:
            if (unlikely(answer_header_mt_5[x][y] == 0)) continue;
            for (int i=answer_header_mt_5[x][y]; i!=0; i=all_answer_ptr_mt_5[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_10(all_answer_mt_5[i][j]);
                    output_buffer_mt_10[buffer_index_mt_10 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 6:
            if (unlikely(answer_header_mt_6[x][y] == 0)) continue;
            for (int i=answer_header_mt_6[x][y]; i!=0; i=all_answer_ptr_mt_6[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_10(all_answer_mt_6[i][j]);
                    output_buffer_mt_10[buffer_index_mt_10 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 7:
            if (unlikely(answer_header_mt_7[x][y] == 0)) continue;
            for (int i=answer_header_mt_7[x][y]; i!=0; i=all_answer_ptr_mt_7[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_10(all_answer_mt_7[i][j]);
                    output_buffer_mt_10[buffer_index_mt_10 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;        
        default:
            break;
        }
    }

    return NULL;
}
// end io thread 10

// io thread 11
char CACHE_LINE_ALIGN output_buffer_mt_11[ONE_INT_CHAR_SIZE * MAX_ANSW * 4 + 100];
int CACHE_LINE_ALIGN buffer_index_mt_11 = 0;
inline void deserialize_int_mt_11(int x) {
    if (x == 0) {
        output_buffer_mt_11[buffer_index_mt_11 ++] = '0';
        return;
    }
    int orignial_index = buffer_index_mt_11;
    while (x) {
        output_buffer_mt_11[buffer_index_mt_11 ++] = (x % 10) + '0';
        x /= 10;
    }
    for (int i=orignial_index, j=buffer_index_mt_11-1; i<j; ++i, --j) {
        orignial_index = output_buffer_mt_11[i];
        output_buffer_mt_11[i] = output_buffer_mt_11[j];
        output_buffer_mt_11[j] = orignial_index;
    }
}
inline void deserialize_id_mt_11(int x) {
    int orginal_index = buffer_index_mt_11;
    memcpy(output_buffer_mt_11 + buffer_index_mt_11, integer_buffer[x], integer_buffer_size[x] * sizeof(char));
    buffer_index_mt_11 += integer_buffer_size[x];
}
void* deserialize_ans_mt_11(void* args) {
    int x, x_2, x_3;
    int begin, end;

    x = 4;
    x_2 = x+2, x_3 = x+3;
    begin = node_num*7/8; end = node_num;
    for (int y=begin; y<end; ++y) {
        switch (global_search_assignment[y])
        {
        case 0:
            if (unlikely(answer_header_mt_0[x][y] == 0)) continue;
            for (int i=answer_header_mt_0[x][y]; i!=0; i=all_answer_ptr_mt_0[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_11(all_answer_mt_0[i][j]);
                    output_buffer_mt_11[buffer_index_mt_11 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 1:
            if (unlikely(answer_header_mt_1[x][y] == 0)) continue;
            for (int i=answer_header_mt_1[x][y]; i!=0; i=all_answer_ptr_mt_1[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_11(all_answer_mt_1[i][j]);
                    output_buffer_mt_11[buffer_index_mt_11 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 2:
            if (unlikely(answer_header_mt_2[x][y] == 0)) continue;
            for (int i=answer_header_mt_2[x][y]; i!=0; i=all_answer_ptr_mt_2[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_11(all_answer_mt_2[i][j]);
                    output_buffer_mt_11[buffer_index_mt_11 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 3:
            if (unlikely(answer_header_mt_3[x][y] == 0)) continue;
            for (int i=answer_header_mt_3[x][y]; i!=0; i=all_answer_ptr_mt_3[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_11(all_answer_mt_3[i][j]);
                    output_buffer_mt_11[buffer_index_mt_11 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 4:
            if (unlikely(answer_header_mt_4[x][y] == 0)) continue;
            for (int i=answer_header_mt_4[x][y]; i!=0; i=all_answer_ptr_mt_4[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_11(all_answer_mt_4[i][j]);
                    output_buffer_mt_11[buffer_index_mt_11 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 5:
            if (unlikely(answer_header_mt_5[x][y] == 0)) continue;
            for (int i=answer_header_mt_5[x][y]; i!=0; i=all_answer_ptr_mt_5[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_11(all_answer_mt_5[i][j]);
                    output_buffer_mt_11[buffer_index_mt_11 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 6:
            if (unlikely(answer_header_mt_6[x][y] == 0)) continue;
            for (int i=answer_header_mt_6[x][y]; i!=0; i=all_answer_ptr_mt_6[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_11(all_answer_mt_6[i][j]);
                    output_buffer_mt_11[buffer_index_mt_11 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 7:
            if (unlikely(answer_header_mt_7[x][y] == 0)) continue;
            for (int i=answer_header_mt_7[x][y]; i!=0; i=all_answer_ptr_mt_7[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_11(all_answer_mt_7[i][j]);
                    output_buffer_mt_11[buffer_index_mt_11 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;        
        default:
            break;
        }
    }

    return NULL;
}
// end io thread 11







// 根据时间决定线程的顺序
void write_to_disk() {
    // to first thr
    ssize_t temp;
    pthread_t io_thr[12];
    pthread_create(io_thr+0, NULL, deserialize_ans_mt_0, NULL);
    pthread_create(io_thr+1, NULL, deserialize_ans_mt_1, NULL);
    pthread_create(io_thr+2, NULL, deserialize_ans_mt_2, NULL);
    pthread_create(io_thr+3, NULL, deserialize_ans_mt_3, NULL);
    pthread_create(io_thr+4, NULL, deserialize_ans_mt_4, NULL);
    pthread_create(io_thr+5, NULL, deserialize_ans_mt_5, NULL);
    pthread_create(io_thr+6, NULL, deserialize_ans_mt_6, NULL);

    pthread_join(io_thr[0], NULL);
    pthread_create(io_thr+7, NULL, deserialize_ans_mt_7, NULL);
    temp = write(writer_fd, output_buffer_mt_0, buffer_index_mt_0);

    pthread_join(io_thr[1], NULL);
    pthread_create(io_thr+8, NULL, deserialize_ans_mt_8, NULL);
    temp = write(writer_fd, output_buffer_mt_1, buffer_index_mt_1);

    pthread_join(io_thr[2], NULL);
    pthread_create(io_thr+9, NULL, deserialize_ans_mt_9, NULL);
    temp = write(writer_fd, output_buffer_mt_2, buffer_index_mt_2);

    pthread_join(io_thr[3], NULL);
    pthread_create(io_thr+10, NULL, deserialize_ans_mt_10, NULL);
    temp = write(writer_fd, output_buffer_mt_3, buffer_index_mt_3);

    pthread_join(io_thr[4], NULL);
    pthread_create(io_thr+11, NULL, deserialize_ans_mt_11, NULL);
    temp = write(writer_fd, output_buffer_mt_4, buffer_index_mt_4);

    pthread_join(io_thr[5], NULL);
    temp = write(writer_fd, output_buffer_mt_5, buffer_index_mt_5);

    pthread_join(io_thr[6], NULL);
    temp = write(writer_fd, output_buffer_mt_6, buffer_index_mt_6);
    
    pthread_join(io_thr[7], NULL);
    temp = write(writer_fd, output_buffer_mt_7, buffer_index_mt_7);

    pthread_join(io_thr[8], NULL);
    temp = write(writer_fd, output_buffer_mt_8, buffer_index_mt_8);

    pthread_join(io_thr[9], NULL);
    temp = write(writer_fd, output_buffer_mt_9, buffer_index_mt_9);

    pthread_join(io_thr[10], NULL);
    temp = write(writer_fd, output_buffer_mt_10, buffer_index_mt_10);

    pthread_join(io_thr[11], NULL);
    temp = write(writer_fd, output_buffer_mt_11, buffer_index_mt_11);
    
}





//------------------ MBODY ---------------------
int main() {
    read_input();
    

    node_topo_filter(false);
    node_topo_filter(true);
    rehash_nodes();

    filter_searchable_nodes();
    // multi search
    pthread_t search_thread_ptr[8];
    
    pthread_create(search_thread_ptr, NULL, search_mt_0, NULL);
    pthread_create(search_thread_ptr + 1, NULL, search_mt_1, NULL);
    pthread_create(search_thread_ptr + 2, NULL, search_mt_2, NULL);
    pthread_create(search_thread_ptr + 3, NULL, search_mt_3, NULL);
    pthread_create(search_thread_ptr + 4, NULL, search_mt_4, NULL);
    pthread_create(search_thread_ptr + 5, NULL, search_mt_5, NULL);
    pthread_create(search_thread_ptr + 6, NULL, search_mt_6, NULL);
    pthread_create(search_thread_ptr + 7, NULL, search_mt_7, NULL);
    
    pthread_join(search_thread_ptr[0], NULL);
    pthread_join(search_thread_ptr[1], NULL);
    pthread_join(search_thread_ptr[2], NULL);
    pthread_join(search_thread_ptr[3], NULL);
    pthread_join(search_thread_ptr[4], NULL);
    pthread_join(search_thread_ptr[5], NULL);
    pthread_join(search_thread_ptr[6], NULL);
    pthread_join(search_thread_ptr[7], NULL);

    answer_num = answer_num_mt_0 + answer_num_mt_1 + answer_num_mt_2 + answer_num_mt_3 + answer_num_mt_4 + answer_num_mt_5 + answer_num_mt_6 + answer_num_mt_7 - 8;
 

    create_integer_buffer();

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