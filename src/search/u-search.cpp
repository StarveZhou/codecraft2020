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
    }
}

#define IS_VISIT           (1)
#define DOUBLE_VISIT       (3)
#define REV_VISIT          (2)

int topo_height[MAX_NODE << 1];
int visit[MAX_NODE], mask = 1111;
int node_topo[MAX_NODE]; // as min_dist
bool filter_useless_edge[MAX_EDGE];

void edge_topo_double_filter() {
    int queue_counter = 0;
    // printf("into edge filter\n");

    int head, tail, u, v, topo_now;
    for (int x=0; x<node_num; ++x) {
        if (visit[x] != 0) continue;
        visit[x] = 1;
        head = tail = 0; node_queue[tail ++] = x;
        node_topo[x] = 1; topo_height[0] = 1;
        while (head < tail) {
            queue_counter ++;
            topo_now = topo_height[head];
            u = node_queue[head ++]; 
            // printf("head: %d, %d ****\n", u, topo_now);
            for (int i=edge_topo_header[u]; i<edge_topo_header[u+1]; ++i) {
                v = edge_topo_edges[i];
                if (visit[v] & IS_VISIT) continue;
                if (node_topo[v] == 0) {
                    visit[v] = IS_VISIT;
                    node_topo[v] = topo_height[tail] = topo_now + 1;
                    node_queue[tail ++] = v;
                } else {
                    // printf("in: %d %d %d %d %d %d\n", u, v, topo_now, node_topo[u], node_topo[v], topo_now - node_topo[v]);
                    if (topo_now - node_topo[v] >= 7) filter_useless_edge[i] = true;
                    if (visit[v] & REV_VISIT) continue; // has visit
                    visit[v] = DOUBLE_VISIT;
                    topo_height[tail] = topo_now + 1;
                    node_queue[tail ++] = v;
                }
            }
            visit[u] --;
        }
    }
    printf("queue counter: %d\n", queue_counter);

    int total_edge_ori = 0, total_edge_after = 0;

    for (int i=0; i<node_num; ++i) {
        for (int j=edge_topo_header[i]; j<edge_topo_header[i+1]; j++) {
            total_edge_after += filter_useless_edge[j] == false;
            total_edge_ori ++;
            // printf("%d %d %d\n", i, edge_topo_edges[j], filter_useless_edge[j]);
        }
    }
    printf("total: %d, after: %d\n", total_edge_ori, total_edge_after);
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

    edge_topo_double_filter();
    printf("node num: %d\n", node_num); fflush(stdout);

    end_time = clock();
    printf("build edge: %d ms\n", (int)(end_time - start_time)); fflush(stdout);


    end_time = clock();
    printf("search: %d ms\n", (int)(end_time - start_time)); fflush(stdout);

    create_writer_fd();

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