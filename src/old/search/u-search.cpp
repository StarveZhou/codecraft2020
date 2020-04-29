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
clock_t start_time, end_time, mid_time;

// 日志输出
#include <stdio.h>

// #define INPUT_PATH   "resources/topo_edge_test.txt"
// #define INPUT_PATH   "resources/topo_test.txt"
// #define INPUT_PATH  "resources/data1.txt"
#define INPUT_PATH  "resources/2861665.txt"
// #define INPUT_PATH   "resources/test_data.txt"
// #define INPUT_PATH   "resources/pre_test.txt"
#define OUTPUT_PATH  "u_output.txt"


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







char buffer[(MAX_EDGE * MAX_FOR_EACH_LINE)];

int data[MAX_EDGE][3], data_num;

int data_rev_mapping[MAX_NODE];
int node_num = 0;
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
        std::sort(rev_edge_topo_edges + rev_edge_topo_header[u], rev_edge_topo_edges + rev_edge_topo_header[u+1], [](int i, int j) -> bool {
            return i > j;
        });
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

    // int total_nodes_c = 0, searchable_nodes_c = 0;
    // for (int i=0; i<node_num; ++i) {
    //     total_nodes_c ++;
    //     searchable_nodes_c += (searchable_nodes[0][i] == true && searchable_nodes[1][i] == true);
    // }
    // printf("total node: %d, searchable: %d\n", total_nodes_c, searchable_nodes_c);
}

#define MAX_SP_DIST       (4)

int visit[MAX_NODE], mask = 1111;
int sp_dist[MAX_NODE];
void shortest_path(int u) {
    memset(sp_dist, -1, sizeof(int) * node_num);
    mask ++;
    sp_dist[u] = 0;

    int v, head = 0, tail = 0, height, orig_u = u;
    bool reach_max;
    node_queue[tail ++] = u;
    while (head < tail) {
        u = node_queue[head ++];
        height = sp_dist[u];
        reach_max = height == MAX_SP_DIST-1;
        for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
            v = rev_edge_topo_edges[i];
            if (visit[v] == mask) continue;
            if (v <= orig_u) break;
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
        all_answer[answer_num][i] = dfs_path[i];
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
// #define MID_DIVIDER            6

// 二分法
inline int decide_search_start(int u) {
    int start = edge_topo_header[u], end = edge_topo_header[u+1];
    if (start == end) return start;
    else if (edge_topo_edges[start] >= dfs_path[0]) return start;
    else if (edge_topo_edges[end-1] < dfs_path[0]) return end;
    else {
        // 该步是否需要调参，在小于阈值的时候遍历，大于阈值才二分？
        int mid; end --;
        while (start+1 < end) {
            mid = (start + end) >> 1;
            if (edge_topo_edges[mid] < dfs_path[0]) start = mid;
            else end = mid;
        }
        return end;
    }
}

void do_search() {
    int u, v, mid; 
    u = dfs_path[depth-1];
    for (int i=decide_search_start(u); i<edge_topo_header[u+1]; ++i) {
        v = edge_topo_edges[i];
        if (v == dfs_path[0]) {
            if (depth >= 3 && depth <= 7) extract_answer();
            continue;
        }
        // if (v < dfs_path[0]) continue;
        if (visit[v] == mask) continue;
        if (depth == 7) continue;
        if (depth + MAX_SP_DIST >= 7) {
            if (sp_dist[v] == -1 || sp_dist[v] + depth > 7) continue;
        }
        dfs_path[depth ++] = v; visit[v] = mask;
        do_search();
        depth --; visit[v] = 0;
    }
}

void search() {
    // int last_answer_num = 0;
    for (int i=0; i<node_num; ++i) {
        if (edge_header[i] == edge_header[i+1]) continue;
        if (!searchable_nodes[0][i] || !searchable_nodes[1][i]) continue;
        shortest_path(i);
        mask ++; visit[i] = mask;
        depth = 1; dfs_path[0] = i;
        do_search();
        // if (answer_num - last_answer_num > 100000) {
        //     printf("%d answer num: %d %d\n", i, answer_num, cut_count); fflush(stdout);
        //     last_answer_num = answer_num;
        // }
    }
    // printf("all sp dist: %d\n", total);
}


char integer_buffer[MAX_NODE][10];
int integer_buffer_size[MAX_NODE];
void create_integer_buffer() {
    int x, i=0, l, r;
    if (data_rev_mapping[0] == 0) {
        integer_buffer_size[0] = 1;
        integer_buffer[0][0] = '0';
        i ++;
    }
    for (; i<node_num; ++i) {
        x = data_rev_mapping[i];
        while (x) {
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

// 可以调试一下io的参数，比如使用mmap，或者更改buffer的size
#define OUTPUT_BUFFER        1280000


int buffer_index = 0;
const int max_available = OUTPUT_BUFFER - 100;

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
    if (buffer_index >= max_available) {
        int ret_size = write(writer_fd, buffer, buffer_index);
        printf("%d, %d\n", ret_size, buffer_index);
        assert(ret_size == buffer_index);
        buffer_index = 0;
    }
}


void write_to_disk() {
    deserialize_int(answer_num - 1);
    buffer[buffer_index ++] = '\n';
    int x_3, x_2;
    for (int x=0; x<5; ++x) {
        x_3 = x + 3; x_2 = x + 2;
        for (int y=0; y<node_num; ++y) {
            if (answer_header[x][y] == 0) continue;
            for (int i=answer_header[x][y]; i!=0; i=all_answer_ptr[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id(all_answer[i][j]);
                    buffer[buffer_index ++] = j == x_2 ? '\n' : ',';
                }
            }
        }
    }
    if (buffer_index > 0) {
        int ret_size = write(writer_fd, buffer, buffer_index);
        printf("%d, %d\n", ret_size, buffer_index);
        assert(ret_size == buffer_index);
    }
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

    end_time = clock();
    printf("build edge: %d ms\n", (int)(end_time - start_time)); fflush(stdout);

    mid_time = clock();
    search();
    printf("answer num: %d\n", answer_num - 1);

    end_time = clock();
    printf("searching: %d ms\n", (int)(end_time - mid_time)); fflush(stdout);


    end_time = clock();
    printf("create integer buffer: %d ms\n", (int)(end_time - start_time)); fflush(stdout);


    mid_time = clock();

    create_integer_buffer();
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