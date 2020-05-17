/**
 * 算法有问题，不会更快而且正确性有待确定
 */

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <string.h>
#include <assert.h>
#include <math.h>

#include <algorithm>

//------------------ MACRO ---------------------


#include <time.h>
clock_t start_time, end_time;

// 日志输出
#include <stdio.h>

// #define INPUT_PATH   "resources/topo_edge_test.txt"
// #define INPUT_PATH   "resources/topo_test.txt"
// #define INPUT_PATH  "resources/data1.txt"
#define INPUT_PATH  "resources/2861665.txt"
// #define INPUT_PATH   "resources/test_data.txt"
// #define INPUT_PATH   "resources/pre_test.txt"
// #define INPUT_PATH   "resources/reuse_test_data.txt"
#define OUTPUT_PATH  "r_output.txt"


// #define INPUT_PATH  "/data/test_data.txt"
// #define OUTPUT_PATH "/projects/student/result.txt"

#define MAX_NODE               570000
#define MAX_EDGE               285000
#define MAX_ANSW               3000100
#define MAX_FOR_EACH_LINE      34



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







char buffer[MAX_EDGE * MAX_FOR_EACH_LINE];
int data[MAX_EDGE][3], data_num;

int data_rev_mapping[MAX_NODE];
int node_num = 0;
c_hash_map_t mapping_t; c_hash_map mapping = &mapping_t;

void read_input() {
    printf("before read\n"); fflush(stdout);
    int fd = open(INPUT_PATH, O_RDONLY);
    printf("reader fd: %d\n", fd); fflush(stdout);
    if (fd == -1) {
        printf("open read only file failed\n");
        exit(0);
    }
    size_t size = read(fd, &buffer, MAX_EDGE * MAX_FOR_EACH_LINE);
    // 这一步没啥必要？
    close(fd);
    printf("after read: %d\n", size); fflush(stdout);

    // 解析
    
    if (buffer[size-1] >= '0' && buffer[size-1] <= '9') {
        buffer[size ++] = '\n';
    }

    int x = 0;
    int* local_data = &data[0][0];
    for (int i=0; i<size; ++i) {
        if (buffer[i] == ',' || buffer[i] == '\n') {
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
        if (c_hash_map_get(mapping, data[i][0]) == -1) {
            c_hash_map_insert(mapping, data[i][0], 0);
            data_rev_mapping[node_num ++] = data[i][0];
        }
        if (c_hash_map_get(mapping, data[i][1]) == -1) {
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
    // printf("writer fd: %d\n", writer_fd);
    if (writer_fd == -1) {
        printf("failed open!");
        exit(0);
    }
}


int edge_num = 1;
int edge_v[MAX_EDGE], edge_ptr[MAX_EDGE], edge_header[MAX_NODE];
bool filter_useless_node[MAX_EDGE];
int filter_cnt[MAX_NODE];

int node_queue[MAX_NODE << 1];


void node_topo_filter(bool revert) {
    int u, v, head = (revert == false), tail = (0 ^ head);
    if (edge_num != 1) {
        memset(edge_header, 0, sizeof(int) * node_num);
        memset(filter_cnt, 0, sizeof(int) * node_num);
        edge_num = 0;
    }
    for (int i=0; i<data_num; ++i) {
        u = data[i][head]; v = data[i][tail];
        if (filter_useless_node[u] || filter_useless_node[v]) continue;

        edge_v[edge_num] = v;
        edge_ptr[edge_num] = edge_header[u];
        edge_header[u] = edge_num ++;

        filter_cnt[v] ++;
    }

    head = tail = 0;
    for (int i=0; i<node_num; ++i) if (filter_cnt[i] == 0) {
        node_queue[tail ++] = i;
    }
    while (head < tail) {
        u = node_queue[head ++];
        filter_useless_node[u] = true;
        for (int i=edge_header[u]; i!=0; i=edge_ptr[i]) {
            v = edge_v[i];
            filter_cnt[v] --;
            if (filter_cnt[v] == 0) node_queue[tail ++] = i;
        }
    }
}

int rehash_mapping[MAX_NODE];
int edge_topo_header[MAX_NODE];
int edge_topo_edges[MAX_EDGE];
int rev_edge_topo_header[MAX_NODE];
int rev_edge_topo_edges[MAX_NODE];
void rehash_nodes() {
    int rehash_node_num = 0;
    for (int i=0; i<node_num; ++i) {
        if (filter_useless_node[i]) continue;
        data_rev_mapping[rehash_node_num] = data_rev_mapping[i];
        rehash_mapping[i] = rehash_node_num ++;
    }


    int u, v;
    edge_num = 1;
    memset(edge_header, 0, sizeof(int) * rehash_node_num);
    for (int i=0; i<data_num; ++i) {
        u = data[i][0]; v = data[i][1];
        if (filter_useless_node[u] || filter_useless_node[v]) continue;
        u = rehash_mapping[u]; v = rehash_mapping[v];
        
        edge_v[edge_num] = v;
        edge_ptr[edge_num] = edge_header[u];
        edge_header[u] = edge_num ++; 
    }
    node_num = rehash_node_num;

    // printf("\n**after rehash:\n");
    // for (int i=0; i<node_num; ++i) {
    //     printf("%d %d\n", data_rev_mapping[i], i);
    // }
    // printf("over\n");

    edge_num = 0;
    for (u=0; u<node_num; ++u) {
        for (int i=edge_header[u]; i!=0; i=edge_ptr[i]) {
            v = edge_v[i];
            edge_topo_edges[edge_num ++] = v;
        }
        edge_topo_header[u+1] = edge_num;
        std::sort(edge_topo_edges + edge_topo_header[u], edge_topo_edges + edge_topo_header[u+1]);
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
    }
}




bool searchable_nodes[2][MAX_NODE];
void filter_searchable_nodes() {
    int v;
    for (int u=0; u<node_num; ++u) {
        for (int j=edge_topo_header[u]; j<edge_topo_header[u+1]; ++j) {
            v = edge_topo_edges[j];
            if (v < u) searchable_nodes[0][v] = true;
            if (v > u) searchable_nodes[1][u] = true;
        }
    }
    for (int i=0; i<node_num; ++i) {
        searchable_nodes[0][i] &= searchable_nodes[1][i];
    }

    int total_nodes_c = 0, searchable_nodes_c = 0;
    for (int i=0; i<node_num; ++i) {
        total_nodes_c ++;
        searchable_nodes_c += (searchable_nodes[0][i] == true && searchable_nodes[1][i] == true);
    }
    printf("total node: %d, searchable: %d\n", total_nodes_c, searchable_nodes_c);
}

int visit[MAX_NODE], mask = 1111;
int search_order[MAX_NODE], search_order_num = 0;
int search_next[MAX_NODE];
void calc_search_order() {
    int usage = 0;

    mask ++;
    int u, v, tmp;
    for (u=0; u<node_num; ++u) {
        if (visit[u] == mask || rev_edge_topo_header[u] == rev_edge_topo_header[u+1]) continue;
        if (!searchable_nodes[0][u]) continue;
        visit[u] = mask;
        search_order[search_order_num] = u;
        search_next[search_order_num] = -1;
        for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
            v = rev_edge_topo_edges[i];
            if (visit[v] == mask) continue;
            if (rev_edge_topo_header[v] == rev_edge_topo_header[v+1] || !searchable_nodes[0][v]) continue;
            visit[v] = mask;
            search_next[search_order_num] = v;
            // swap to ensure the first one is always search_next
            rev_edge_topo_edges[i] = rev_edge_topo_edges[rev_edge_topo_header[u]];
            rev_edge_topo_edges[rev_edge_topo_header[u]] = v;

            usage ++;
            break;
        }
        search_order_num ++;
    }

    printf("1878: %d\n", rev_edge_topo_edges[rev_edge_topo_header[1878]]);

    // printf("reuse bfs tree: %d\n", usage);
    // printf("\n** search order:\n");
    // for (int i=0; i<search_order_num; ++i) {
    //     printf("%d %d\n", search_order[i], search_next[i]);
    // }
    // printf("over\n");
    
}

#define MAX_SP_DIST       (4)


int sp_dist[MAX_NODE], sp_dist_ext[MAX_NODE];
int is_first_parent[MAX_NODE];
void shortest_path_ext(int u) {
    memset(sp_dist, -1, sizeof(int) * node_num);
    memset(sp_dist_ext, -1, sizeof(int) * node_num);

    mask ++;
    sp_dist[u] = 0;

    int v, head = 0, tail = 0, height, height_ext, is_first;
    bool reach_max;
    node_queue[tail ++] = u;
    while (head < tail) {
        u = node_queue[head ++];
        if (head == 2) {
            is_first_parent[u] = mask;
        }
        height = sp_dist[u];
        height_ext = sp_dist_ext[u];
        is_first = is_first_parent[u];
        for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
            v = rev_edge_topo_edges[i];
            if (sp_dist[v] == -1) {
                sp_dist[v] = height + 1;
                is_first_parent[v] = is_first;
                if (is_first == mask) {
                    if (height == MAX_SP_DIST) continue;
                } else {
                    if (height == MAX_SP_DIST-1) continue;
                }
                node_queue[tail ++] = v;
            } else {
                if (sp_dist_ext[v] != -1) continue;
                if (is_first_parent[v] == mask) continue;
                if (is_first_parent[u] == mask) {
                    sp_dist_ext[v] = height + 1;
                    if (height == MAX_SP_DIST) continue;
                } else {
                    sp_dist_ext[v] = height_ext + 1;
                    if (height_ext == MAX_SP_DIST) continue;
                }
                
                node_queue[tail ++] = v;
            }
        }
    }
    // for (int i=0; i<node_num; ++i) {
    //     printf("** %d[%d]: %d, %d\n", i, is_first_parent[i] == mask, sp_dist[i], sp_dist_ext[i]);
    // }
    for (int i=0; i<node_num; ++i) {
        if (is_first_parent[i] == mask) sp_dist_ext[i] = sp_dist[i];
    }
}


void shortest_path(int u) {
    memset(sp_dist, -1, sizeof(int) * node_num);
    mask ++;
    sp_dist[u] = 0;

    int v, head = 0, tail = 0, height;
    bool reach_max;
    node_queue[tail ++] = u;
    while (head < tail) {
        u = node_queue[head ++];
        height = sp_dist[u];
        reach_max = height == MAX_SP_DIST-1;
        for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
            v = rev_edge_topo_edges[i];
            if (visit[v] == mask) continue;
            visit[v] = mask;
            sp_dist[v] = height + 1;
            if (reach_max) continue;
            node_queue[tail ++] = v;
        }
    }
}

int depth, dfs_path[8];
int all_answer[MAX_ANSW + 1][7], answer_num = 1;
int all_answer_ptr[MAX_ANSW];
int answer_header[5][MAX_NODE], answer_tail[5][MAX_NODE];


void extract_answer() {
    for (int i=0; i<depth; ++i) {
        all_answer[answer_num][i] = data_rev_mapping[dfs_path[i]];
    }
    int x = depth - 3, y = dfs_path[0];
    if (answer_header[x][y] == 0) {
        answer_header[x][y] = answer_tail[x][y] = answer_num;
    } else {
        all_answer_ptr[answer_tail[x][y]] = answer_num;
        answer_tail[x][y] = answer_num;
    }
    answer_num ++;
}

int cut_count = 0;
bool use_extent;

void do_search() {
    int u, v, mid; 
    u = dfs_path[depth-1];
    for (int i=edge_topo_header[u]; i<edge_topo_header[u+1]; ++i) {
        v = edge_topo_edges[i];
        if (v == dfs_path[0]) {
            if (depth >= 3 && depth <= 7) extract_answer();
            continue;
        }
        if (v <= dfs_path[0] || visit[v] == mask) continue;
        if (depth == 7) continue;
        if (depth + MAX_SP_DIST >= 7) {
            if (use_extent) {
                if (sp_dist_ext[v] == -1 || sp_dist_ext[v] + depth > 8) continue;
            } else {
                if (sp_dist[v] == -1 || sp_dist[v] + depth > 7) continue;
            }
        }
        dfs_path[depth ++] = v; visit[v] = mask;
        do_search();
        depth --; visit[v] = 0;
    }
}

void search() {
    int last_answer_num = 0, x, y;
    for (int i=0; i<search_order_num; ++i) {
        x = search_order[i]; y = search_next[i];
        if (y != -1) {
            shortest_path_ext(x);
        } else {
            shortest_path(x);
        }

        mask ++; visit[i] = mask;
        depth = 1; dfs_path[0] = x;
        use_extent = false;
        do_search();
        if (y != -1) {
            mask ++;
            depth = 1; dfs_path[0] = y;
            use_extent = true;
            do_search();
        }
        if (answer_num - last_answer_num > 100000) {
            printf("%d %d answer num: %d %d\n", x, y, answer_num, cut_count); fflush(stdout);
            last_answer_num = answer_num;
        }
    }
    // printf("all sp dist: %d\n", total);
}



int buffer_index = 0;
const int max_available = MAX_EDGE * MAX_FOR_EACH_LINE - 100;
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
    if (buffer_index >= max_available) {
        int ret_size = write(writer_fd, buffer, buffer_index);
        // printf("%d, %d\n", ret_size, buffer_index);
        // assert(ret_size == buffer_index);
        buffer_index = 0;
    }
}

void write_to_disk() {
    deserialize_int(answer_num - 1);
    buffer[buffer_index ++] = '\n';
    for (int x=0; x<5; ++x) {
        for (int y=0; y<node_num; ++y) {
            if (answer_header[x][y] == 0) continue;
            for (int i=answer_header[x][y]; i!=0; i=all_answer_ptr[i]) {
                for (int j=0; j<x+3; ++j) {
                    deserialize_int(all_answer[i][j]);
                    buffer[buffer_index ++] = j == (x + 2) ? '\n' : ',';
                }
            }
        }
    }
    if (buffer_index > 0) {
        int ret_size = write(writer_fd, buffer, buffer_index);
        // printf("%d, %d\n", ret_size, buffer_index);
        // assert(ret_size == buffer_index);
    }
}

//------------------ MBODY ---------------------
int main() {
    start_time = clock();

    printf("why\n"); fflush(stdout);

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

    end_time = clock();
    printf("build edge: %d ms\n", (int)(end_time - start_time)); fflush(stdout);

    calc_search_order();
    
    search();
    printf("answer num: %d\n", answer_num - 1);

    end_time = clock();
    printf("search: %d ms\n", (int)(end_time - start_time)); fflush(stdout);

    create_writer_fd();
    write_to_disk();
    close(writer_fd);
    end_time = clock();
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