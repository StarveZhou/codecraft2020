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
#define OUTPUT_PATH  "glue.txt"


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

int CACHE_LINE_ALIGN visit[MAX_NODE], mask = 1111;
int CACHE_LINE_ALIGN edge_topo_starter[MAX_NODE];
int dfs_path[5], dfs_path_num, milestone;

int all_edges[MAX_ANSW << 4][2], all_edges_header[MAX_NODE], all_edges_tailer[MAX_NODE];
int all_edges_ptr[MAX_ANSW << 4], all_edges_prv[MAX_ANSW << 4], all_edges_num;

int rev_edges[MAX_ANSW], rev_edges_header[MAX_NODE], rev_edges_tailer[MAX_NODE];
int rev_edges_ptr[MAX_ANSW], rev_edges_num;
int rev_edges_contain[MAX_NODE], rev_edges_mask = 0;

void pre_dfs(int begin_with) {
    all_edges_header[begin_with] = all_edges_tailer[begin_with] = -1;
    int u, v;
    for (int i=edge_topo_header[begin_with]; i<edge_topo_header[begin_with+1]; ++i) {
        u = edge_topo_edges[i];
        if (u == milestone) continue;
        for (int j=edge_topo_header[u]; j<edge_topo_header[u+1]; ++j) {
            v = edge_topo_edges[j];
            if (v == begin_with) continue;
            all_edges[all_edges_num][0] = u;
            all_edges[all_edges_num][1] = v;
            all_edges_ptr[all_edges_num] = -1;
            if (all_edges_header[begin_with] == -1) {
                all_edges_prv[all_edges_num] = all_edges_ptr[all_edges_num] = -1;
                all_edges_header[begin_with] = all_edges_tailer[begin_with] = all_edges_num;
            } else {
                all_edges_prv[all_edges_num] = all_edges_tailer[begin_with];
                all_edges_ptr[all_edges_tailer[begin_with]] = all_edges_num;
                all_edges_tailer[begin_with] = all_edges_num;
            }
            all_edges_num ++;
        }
    }
    printf("%d %d\n", begin_with, all_edges_num); fflush(stdout);
}

void pre_rev_dfs() {
    rev_edges_num = 0;
    rev_edges_mask ++;
    int u, v;
    for (int i=rev_edge_topo_header[milestone]; i<rev_edge_topo_header[milestone+1]; ++i) {
        u = rev_edge_topo_edges[i];
        if (u < milestone) continue;
        for (int j=rev_edge_topo_header[u]; j<rev_edge_topo_header[u+1]; ++j) {
            v = rev_edge_topo_edges[j];
            if (v < milestone) continue;
            if (rev_edges_contain[v] != rev_edges_mask) {
                rev_edges_contain[v] = rev_edges_mask;
                rev_edges_header[v] = -1;
            }
            
            rev_edges[rev_edges_num] = u;
            rev_edges_ptr[rev_edges_num] = rev_edges_header[v];
            rev_edges_header[v] = rev_edges_num;
            rev_edges_num ++;
        }
    }
}

inline void rm_all_edges(int u, int i) {
    int nxt, prv;
    nxt = all_edges_ptr[i];
    prv = all_edges_prv[i];
    if (nxt == -1 && prv == -1) {
        all_edges_header[u] = all_edges_tailer[u] = -1;
    } else if (nxt == -1) {
        all_edges_ptr[prv] = -1;
    } else if (prv == -1) {
        all_edges_prv[nxt] = -1;
        all_edges_header[u] = nxt;
    } else {
        all_edges_prv[nxt] = prv;
        all_edges_ptr[prv] = nxt;
    }
}

int answer[5][MAX_ANSW][7];
int answer_num[5];

int extract_path[7];
inline void extract_answer_2(int x1) {
    int x0 = milestone, x2, x3, x4, x5, x6;
    int prv, nxt;
    extract_path[1] = x1;
    int i, j, k, li, lj, lk;

    // 3
    i = rev_edges_header[x1];
    if (rev_edges_contain[x1] == rev_edges_mask)
    while (i != -1) {
        x2 = rev_edges[i];
        i = rev_edges_ptr[i];
        extract_path[2] = x2;
        memcpy(answer[0][answer_num[0]], extract_path, sizeof(int) * 3);
        answer_num[0] ++;
    }
    
    // 5 & 7
    i = all_edges_header[x1];
    while (i != -1) {
        x2 = all_edges[i][0]; x3 = all_edges[i][1];
        li = i;
        i = all_edges_ptr[i];
        if (x2 <= milestone || x3 <= milestone) {
            rm_all_edges(x1, li);
            continue;
        }
        extract_path[2] = x2; extract_path[3] = x3;
        // 5
        j = rev_edges_header[x3];
        if (rev_edges_contain[x3] == rev_edges_mask)
        while (j != -1) {
            x4 = rev_edges[j];
            j = rev_edges_ptr[j];
            if (x4 == x1 || x4 == x2) continue;
            extract_path[4] = x4;
            memcpy(answer[2][answer_num[2]], extract_path, sizeof(int) * 5);
            answer_num[2] ++;
        }
        // 7
        j = all_edges_header[x3];
        while (j != -1) {
            x4 = all_edges[j][0]; x5 = all_edges[j][1];
            lj = j;
            j = all_edges_ptr[j];
            if (x4 <= milestone || x5 <= milestone) {
                rm_all_edges(x3, lj);
                continue;
            }
            if (x4 == x1 || x4 == x2 || x5 == x1 || x5 == x2) continue;
            extract_path[4] = x4; extract_path[5] = x5;
            k = rev_edges_header[x5];
            if (rev_edges_contain[x5] == rev_edges_mask)
            while (k != -1) {
                x6 = rev_edges[k];
                k = rev_edges_ptr[k];
                if (x6 == x1 || x6 == x2 || x6 == x3 || x6 == x4) continue;
                extract_path[6] = x6;
                memcpy(answer[4][answer_num[4]], extract_path, sizeof(int) * 7);
                answer_num[4] ++;
            }
        }
    }
}

inline void extract_answer_3(int x1, int x2) {
    int x0 = milestone, x3, x4, x5;
    extract_path[1] = x1; extract_path[2] = x2;

    int i, j, li, lj;

    //4
    i = rev_edges_header[x2];
    if (rev_edges_contain[x2] == rev_edges_mask)
    while (i != -1) {
        x3 = rev_edges[i];
        i = rev_edges_ptr[i];
        if (x3 == x1) continue;
        extract_path[3] = x3;
        memcpy(answer[1][answer_num[1]], extract_path, sizeof(int) * 4);
        answer_num[1] ++;
    }

    // 6
    i = all_edges_header[x2];
    while (i != -1) {
        x3 = all_edges[i][0]; x4 = all_edges[i][1];
        li = i;
        i = all_edges_ptr[i];
        if (x3 <= milestone || x4 <= milestone) {
            rm_all_edges(x2, li);
            continue;
        }
        if (x3 == x1 || x4 == x1) continue;
        extract_path[3] = x3; extract_path[4] = x4;
        j = rev_edges_header[x4];
        if (rev_edges_contain[x4] == rev_edges_mask)
        while (j != -1) {
            x5 = rev_edges[j];
            j = rev_edges_ptr[j];
            if (x5 == x1 || x5 == x2 || x5 == x3) continue;
            extract_path[5] = x5;
            memcpy(answer[3][answer_num[3]], extract_path, sizeof(int) * 6);
            answer_num[3] ++;
        }
    }
}

void do_search(int begin_with) {
    
    milestone = begin_with;
    pre_rev_dfs();
    printf("search: %d %d\n", begin_with, rev_edges_num); fflush(stdout);
    if (rev_edges_num == 0) return;
    extract_path[0] = milestone;

    int u, v;
    while (edge_topo_starter[begin_with] < edge_topo_header[begin_with+1] && edge_topo_edges[edge_topo_starter[begin_with]] < milestone) edge_topo_starter[begin_with] ++;
    for (int i=edge_topo_starter[begin_with]; i<edge_topo_header[begin_with+1]; ++i) {
        u = edge_topo_edges[i];
        extract_answer_2(u);
        while (edge_topo_starter[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter[u]] < milestone) edge_topo_starter[u] ++;
        for (int j=edge_topo_starter[u]; j<edge_topo_header[u]; ++j) {
            v = edge_topo_edges[j];
            if (v == begin_with) continue;
            extract_answer_3(u, v);
        }
    }
    
}

void search() {
    memcpy(edge_topo_starter, edge_topo_header, sizeof_int_mul_node_num);
    rev_edges_num = all_edges_num = 0;
    for (int i=1; i<node_num; ++i) pre_dfs(i);
    printf("begin with: %d\n", all_edges_num);
    // return;
    for (int u=0; u<node_num; ++u) {
        if (unlikely(edge_header[u] == edge_header[u+1])) continue;
        if (unlikely(!searchable_nodes[0][u])) continue;
        do_search(u);
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

int answer_num_total = 0;
char output_buffer[MAX_ANSW * ONE_INT_CHAR_SIZE * 7];
int output_buffer_num = 0;
void write_to_disk() {
    size_t rub;
    deserialize_int(output_buffer, output_buffer_num, answer_num_total);
    output_buffer[output_buffer_num ++] = '\n';
    for (int i=0; i<5; ++i) {
        for (int j=0; j<answer_num[i]; ++j) {
            for (int k=0; k<i+3; ++k) {
                deserialize_id(output_buffer, output_buffer_num, answer[i][j][k]);
                output_buffer[output_buffer_num ++] = k == (i + 2) ? '\n' : ',';
            }
        }
    }
    rub = write(writer_fd, output_buffer, output_buffer_num);
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
    for (int i=0; i<PATH_PAR_3; ++i) std_id_arr[i] = i;

    end_time = clock();
    printf("build edge: %d ms\n", (int)(end_time - start_time)); fflush(stdout);

    mid_time = clock();
    search();
    answer_num_total = 0;
    for (int i=0; i<5; ++i) answer_num_total += answer_num[i];
    printf("answer: %d\n", answer_num_total);
    

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