#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <assert.h>
#include <math.h>

#include <algorithm>

//------------------ MACRO ---------------------


#include <time.h>
clock_t start_time, end_time;

// 日志输出
#include <stdio.h>

// #define INPUT_PATH  "resources/2861665.txt"
// #define OUTPUT_PATH  "c_search_output.txt"


#define INPUT_PATH  "/data/test_data.txt"
#define OUTPUT_PATH "/projects/student/result.txt"

#define MAX_NODE               30000
#define MAX_EDGE               280000
#define MAX_ANSW               3000000
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

void read_input() {
    

    int fd = open(INPUT_PATH, O_RDONLY);
    size_t size = read(fd, &buffer, MAX_EDGE * MAX_FOR_EACH_LINE);
    // 这一步没啥必要？
    close(fd);

    printf("after read\n"); fflush(stdout);

    // 解析
    c_hash_map_t mapping_t; c_hash_map mapping = &mapping_t;
    int x = 0, idx = 0;
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
    printf("writer fd: %d\n", writer_fd);
    if (writer_fd == -1) {
        printf("failed open!");
        exit(0);
    }
}


int edge_num = 1;
int edge_v[MAX_EDGE], edge_ptr[MAX_EDGE], edge_header[MAX_NODE];
bool filter_useless_node[MAX_EDGE];
int filter_cnt[MAX_NODE];

int node_queue[MAX_NODE];

void build_filter_edge(bool revert) {
    int from = 0, to = 1;
    if (revert) {
        from = 1, to = 0;
    }
    for (int i=0; i<data_num; ++i) {
        if (filter_useless_node[data[i][0]] || filter_useless_node[data[i][1]]) continue;
        edge_v[edge_num] = data[i][to];
        edge_ptr[edge_num] = edge_header[data[i][from]];
        edge_header[data[i][from]] = edge_num ++;

        filter_cnt[data[i][to]] += 1;
    }
}

void edge_filter(bool revert) {
    if (!revert) {
        memset(edge_header, 0, sizeof(edge_header));
        memset(filter_cnt, 0, sizeof(filter_cnt));
        edge_num = 1;
        
    }

    build_filter_edge(revert);

    int head = 0, tail = 0;
    for (int i=0; i<node_num; ++i) {
        if (filter_cnt[i] == 0) {
            node_queue[tail ++] = i;
            filter_useless_node[i] = true;
        }
    }
    int u, v;
    while (head < tail) {
        u = node_queue[head ++];
        for (int i=edge_header[u]; i!=0; i=edge_ptr[i]) {
            v = edge_v[i];
            filter_cnt[v] --;
            if (filter_cnt[v] == 0) {
                node_queue[tail ++] = v;
                filter_useless_node[v] = true;
            }
        }
    }
}


int edges[MAX_EDGE];

void build_edge() {
    edge_num = 0;
    int v;
    for (int u=0; u<node_num; ++u) {
        if (filter_useless_node[u]) {
            edge_header[u] = edge_num;
            continue;
        }
        for (int i=edge_header[u]; i!=0; i=edge_ptr[i]) {
            v = edge_v[i];
            if (filter_useless_node[v]) continue;
            edges[edge_num ++] = v;
        }
        edge_header[u] = edge_num;
    }
    for (int i=node_num-1; i>=0; --i) edge_header[i+1] = edge_header[i];
    edge_header[0] = 0;
    for (int i=0; i<node_num; ++i) {
        if (edge_header[i] != edge_header[i+1]) {
            std::sort(edges + edge_header[i], edges + edge_header[i+1]);
        }
    }
}

int rev_edges_header[MAX_NODE], rev_edges[MAX_EDGE];
void build_rev_edge() {
    edge_num = 1;
    int v;
    for (int u=0; u<node_num; ++u) {
        for (int i=edge_header[u]; i<edge_header[u+1]; ++i) {
            v = edges[i];
            edge_v[edge_num] = u; 
            edge_ptr[edge_num] = rev_edges_header[v];
            rev_edges_header[v] = edge_num ++;
        }
    }

    edge_num = 0;
    for (int u=0; u<node_num; ++u) {
        for (int i=rev_edges_header[u]; i!=0; i=edge_ptr[i]) {
            v = edge_v[i];
            rev_edges[edge_num ++] = v;
        }
        rev_edges_header[u] = edge_num;
    }
    for (int i=node_num-1; i>=0; --i) rev_edges_header[i+1] = rev_edges_header[i];
    rev_edges_header[0] = 0;
}



int temp_num = 0;
int sp_dist[MAX_NODE], visit[MAX_NODE], mask = 0;
void shortest_path(int u) {
    temp_num = 0;

    ++ mask;
    sp_dist[u] = 0; visit[u] = mask;
    node_queue[0] = u;

    int head = 0, tail = 1, v;
    while (head < tail) {
        u = node_queue[head ++];
        temp_num ++;
        for (int i=rev_edges_header[u]; i<rev_edges_header[u+1]; ++i) {
            v = rev_edges[i];
            if (visit[v] == mask) continue;
            sp_dist[v] = sp_dist[u] + 1;
            visit[v] = mask;
            if (sp_dist[v] == 7) continue;
            node_queue[tail ++] = v;
        }
    }
}

int depth, dfs_path[8];
int all_answer[MAX_ANSW + 1][7], answer_num = 1;
int all_answer_ptr[MAX_ANSW];
int answer_header[5][MAX_NODE], answer_tail[5][MAX_NODE];

/**
 * 结果有大问题，但是应该是映射，不是搜索，因为答案的数量是一样的，出现了没有的点
 */
void extract_answer() {
    for (int i=0; i<depth; ++i) {
        assert(dfs_path[i] < 28500);
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



void do_search() {
    int u, v; 
    u = dfs_path[depth-1];
    for (int i=edge_header[u]; i<edge_header[u+1]; ++i) {
        v = edges[i];
        if (v == dfs_path[0]) {
            if (depth >= 3 && depth <= 7) extract_answer();
            continue;
        }
        if (v <= dfs_path[0] || visit[v] == mask) continue;
        if (sp_dist[v] == -1 || sp_dist[v] + depth > 7) continue;
        dfs_path[depth ++] = v; visit[v] = mask;
        do_search();
        depth --; visit[v] = 0;
    }
}

void search() {
    int total = 0;
    for (int i=0; i<node_num; ++i) {
        if (edge_header[i] == edge_header[i+1]) continue;
        shortest_path(i);
        total += temp_num;
        mask ++; visit[i] = mask;
        depth = 1; dfs_path[0] = i;
        do_search();
    }
    printf("all sp dist: %d\n", total);
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
        printf("%d, %d\n", ret_size, buffer_index);
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
        printf("%d, %d\n", ret_size, buffer_index);
        // assert(ret_size == buffer_index);
    }
}

//------------------ MBODY ---------------------
int main() {
    start_time = clock();

    read_input();
    printf("node num: %d\n", node_num); fflush(stdout);

    end_time = clock();
    printf("read: %d ms\n", end_time - start_time); fflush(stdout);

    edge_filter(true);
    edge_filter(false);

    build_edge();
    build_rev_edge();

    printf("edge num: %d\n", edge_header[node_num]); fflush(stdout);

    end_time = clock();
    printf("build edge: %d ms\n", end_time - start_time); fflush(stdout);

    search();
    printf("answer: %d\n", answer_num - 1); fflush(stdout);

    end_time = clock();
    printf("search: %d ms\n", end_time - start_time); fflush(stdout);

    create_writer_fd();
    write_to_disk();
    close(writer_fd);
    end_time = clock();
    printf("running: %d ms\n", end_time - start_time);
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