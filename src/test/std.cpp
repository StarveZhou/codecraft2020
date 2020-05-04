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
// #define INPUT_PATH  "resources/2861665.txt"
#define INPUT_PATH   "resources/test_data.txt"
// #define INPUT_PATH   "resources/pre_test.txt"
#define OUTPUT_PATH  "output/old-std.txt"


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
int dfs_path[5], dfs_path_num;

char CACHE_LINE_ALIGN answer_3[MAX_3_ANSW * ONE_INT_CHAR_SIZE * 3];
char CACHE_LINE_ALIGN answer_4[MAX_4_ANSW * ONE_INT_CHAR_SIZE * 4];
int answer_3_buffer_num, answer_4_buffer_num;
int answer_3_num = 0, answer_4_num = 0;

inline void extract_3_answer() {
    answer_3_num ++;
    deserialize_id(answer_3, answer_3_buffer_num, dfs_path[0]);
    answer_3[answer_3_buffer_num ++] = ',';
    deserialize_id(answer_3, answer_3_buffer_num, dfs_path[1]);
    answer_3[answer_3_buffer_num ++] = ',';
    deserialize_id(answer_3, answer_3_buffer_num, dfs_path[2]);
    answer_3[answer_3_buffer_num ++] = '\n';
}

inline void extract_4_answer() {
    answer_4_num ++;
    deserialize_id(answer_4, answer_4_buffer_num, dfs_path[0]);
    answer_4[answer_4_buffer_num ++] = ',';
    deserialize_id(answer_4, answer_4_buffer_num, dfs_path[1]);
    answer_4[answer_4_buffer_num ++] = ',';
    deserialize_id(answer_4, answer_4_buffer_num, dfs_path[2]);
    answer_4[answer_4_buffer_num ++] = ',';
    deserialize_id(answer_4, answer_4_buffer_num, dfs_path[3]);
    answer_4[answer_4_buffer_num ++] = '\n';
}


int backward_3_path_header[MAX_NODE];
int backward_3_path_ptr[MAX_NODE];
int backward_3_path_value[PATH_PAR_3][2];
int backward_3_num = 0;
int backward_contains[MAX_NODE], backward_contains_num = 0;

inline void extract_backward_3_path() {
    if (backward_3_path_header[dfs_path[3]] == -1) {
        backward_contains[backward_contains_num ++] = dfs_path[3];
    }
    backward_3_path_value[backward_3_num][0] = dfs_path[1];
    backward_3_path_value[backward_3_num][1] = dfs_path[2];
    backward_3_path_ptr[backward_3_num] = backward_3_path_header[dfs_path[3]];
    backward_3_path_header[dfs_path[3]] = backward_3_num ++;
}



void backward_dfs() {
    int u, v, original_path_num = dfs_path_num;
    u = dfs_path[dfs_path_num - 1]; dfs_path_num ++;
    for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
        v = rev_edge_topo_edges[i];
        if (v <= dfs_path[0]) break;
        if (visit[v] == mask) continue;
        dfs_path[original_path_num] = v; 
        if (dfs_path_num == 4) {
            extract_backward_3_path();
            continue;
        }
        visit[v] = mask;
        backward_dfs();
        visit[v] = 0;
    }
    dfs_path_num --;
}

int sorted_backward_3_path[PATH_PAR_3];
int sorted_backward_3_path_fin[PATH_PAR_3][2];
int sorted_backward_3_path_from[MAX_NODE];
int sorted_backward_3_path_to[MAX_NODE];
int sorted_backward_id[PATH_PAR_3];

void sort_out(int begin_with) {
    int i, u, v, num = 0, from, to;
    for (i=0; i<backward_contains_num; ++i) {
        u = backward_contains[i];
        from = sorted_backward_3_path_from[u] = num;
        for (int j=backward_3_path_header[u]; j!=-1; j=backward_3_path_ptr[j]) {
            sorted_backward_3_path[num ++] = j;
        }
        to = sorted_backward_3_path_to[u] = num;
        std::sort(sorted_backward_3_path + from, sorted_backward_3_path + to, [](int x, int y) -> bool {
            if (backward_3_path_value[x][1] != backward_3_path_value[y][1]) return backward_3_path_value[x][1] < backward_3_path_value[y][1];
            else return backward_3_path_value[x][0] < backward_3_path_value[y][0];
        });
        for (int j=from; j<to; ++j) {
            sorted_backward_3_path_fin[j][0] = backward_3_path_value[sorted_backward_3_path[j]][0];
            sorted_backward_3_path_fin[j][1] = backward_3_path_value[sorted_backward_3_path[j]][1];
        }
    }
} 


int answer_5[MAX_ANSW][5];
int answer_6[MAX_ANSW][6];
int answer_7[MAX_ANSW][7];
int answer_5_num = 0, answer_6_num = 0, answer_7_num = 0;

inline void extract_forward_2_path() {
    int v = dfs_path[2], x, y;
    for (int i=sorted_backward_3_path_from[v]; i<sorted_backward_3_path_to[v]; ++i) {
        x = sorted_backward_3_path_fin[i][0];
        y = sorted_backward_3_path_fin[i][1];
        if (visit[x] == mask || visit[y] == mask) continue;
        memcpy(answer_5[answer_5_num], dfs_path, sizeof(int) * 3);
        answer_5[answer_5_num][3] = y;
        answer_5[answer_5_num][4] = x;
        answer_5_num ++;
    }
}


inline void extract_forward_3_path() {
    int v = dfs_path[3], x, y;
    for (int i=sorted_backward_3_path_from[v]; i<sorted_backward_3_path_to[v]; ++i) {
        x = sorted_backward_3_path_fin[i][0];
        y = sorted_backward_3_path_fin[i][1];
        if (visit[x] == mask || visit[y] == mask) continue;
        memcpy(answer_6[answer_6_num], dfs_path, sizeof(int) * 4);
        answer_6[answer_6_num][4] = y;
        answer_6[answer_6_num][5] = x;
        answer_6_num ++;
    }
}

inline void extract_forward_4_path() {
    int v = dfs_path[4], x, y;
    for (int i=sorted_backward_3_path_from[v]; i<sorted_backward_3_path_to[v]; ++i) {
        x = sorted_backward_3_path_fin[i][0];
        y = sorted_backward_3_path_fin[i][1];
        if (visit[x] == mask || visit[y] == mask) continue;
        memcpy(answer_7[answer_7_num], dfs_path, sizeof(int) * 5);
        answer_7[answer_7_num][5] = y;
        answer_7[answer_7_num][6] = x;
        answer_7_num ++;
    }
}

int total_dfs = 0;
void forward_dfs() {
    total_dfs ++;
    int u, v, original_path_num = dfs_path_num;
    u = dfs_path[dfs_path_num - 1];  dfs_path_num ++;
    while (edge_topo_starter[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter[u]] < dfs_path[0]) edge_topo_starter[u] ++;
    for (int i=edge_topo_starter[u]; i<edge_topo_header[u+1]; ++i) {
        v = edge_topo_edges[i];
        if (v == dfs_path[0]) {
            if (original_path_num == 3) extract_3_answer();
            if (original_path_num == 4) extract_4_answer();
            continue;
        }
        if (visit[v] == mask) continue;
        dfs_path[original_path_num] = v; 
        if (backward_3_path_header[v] != -1) {
            switch (original_path_num)
            {
            case 2:
                extract_forward_2_path();
                break;
            case 3:
                extract_forward_3_path();
                break;
            case 4:
                extract_forward_4_path();
                break;
            default:
                break;
            }
        }
        if (original_path_num == 4) continue;
        visit[v] = mask;
        forward_dfs();
        visit[v] = 0;
    }
    dfs_path_num --;
}







void do_search_mim(int begin_with) {
    backward_3_num = 0;
    backward_contains_num = 0;
    // 可以不用memset，用一个visit就好了
    memset(backward_3_path_header + begin_with, -1, sizeof(int) * (node_num - begin_with));
    // printf("started with: %d %d\n", begin_with, data_rev_mapping[begin_with]); fflush(stdout);
    dfs_path[0] = begin_with; dfs_path_num = 1;
    mask ++; visit[begin_with] = mask;
    backward_dfs();
    if (backward_3_num == 0) return;
    sort_out(begin_with);
    dfs_path[0] = begin_with; dfs_path_num = 1;
    mask ++; visit[begin_with] = mask;
    forward_dfs();
}

void search() {
    memcpy(edge_topo_starter, edge_topo_header, sizeof_int_mul_node_num);
    for (int u=0; u<node_num; ++u) {
        if (unlikely(edge_header[u] == edge_header[u+1])) continue;
        if (unlikely(!searchable_nodes[0][u])) continue;
        do_search_mim(u);
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
char output_buffer[MAX_ANSW * ONE_INT_CHAR_SIZE * 7];
int output_buffer_num = 0;
void write_to_disk() {
    size_t rub;
    deserialize_int(output_buffer, output_buffer_num, answer_num);
    output_buffer[output_buffer_num ++] = '\n';

    rub = write(writer_fd, output_buffer, output_buffer_num);

    rub = write(writer_fd, answer_3, answer_3_buffer_num);
    rub = write(writer_fd, answer_4, answer_4_buffer_num);

    output_buffer_num = 0;
    for (int i=0; i<answer_5_num; ++i) {
        deserialize_id(output_buffer, output_buffer_num, answer_5[i][0]);
        output_buffer[output_buffer_num ++] = ',';
        deserialize_id(output_buffer, output_buffer_num, answer_5[i][1]);
        output_buffer[output_buffer_num ++] = ',';
        deserialize_id(output_buffer, output_buffer_num, answer_5[i][2]);
        output_buffer[output_buffer_num ++] = ',';
        deserialize_id(output_buffer, output_buffer_num, answer_5[i][3]);
        output_buffer[output_buffer_num ++] = ',';
        deserialize_id(output_buffer, output_buffer_num, answer_5[i][4]);
        output_buffer[output_buffer_num ++] = '\n';
    }
    rub = write(writer_fd, output_buffer, output_buffer_num);

    output_buffer_num = 0;
    for (int i=0; i<answer_6_num; ++i) {
        deserialize_id(output_buffer, output_buffer_num, answer_6[i][0]);
        output_buffer[output_buffer_num ++] = ',';
        deserialize_id(output_buffer, output_buffer_num, answer_6[i][1]);
        output_buffer[output_buffer_num ++] = ',';
        deserialize_id(output_buffer, output_buffer_num, answer_6[i][2]);
        output_buffer[output_buffer_num ++] = ',';
        deserialize_id(output_buffer, output_buffer_num, answer_6[i][3]);
        output_buffer[output_buffer_num ++] = ',';
        deserialize_id(output_buffer, output_buffer_num, answer_6[i][4]);
        output_buffer[output_buffer_num ++] = ',';
        deserialize_id(output_buffer, output_buffer_num, answer_6[i][5]);
        output_buffer[output_buffer_num ++] = '\n';
    }
    rub = write(writer_fd, output_buffer, output_buffer_num);

    output_buffer_num = 0;
    for (int i=0; i<answer_7_num; ++i) {
        deserialize_id(output_buffer, output_buffer_num, answer_7[i][0]);
        output_buffer[output_buffer_num ++] = ',';
        deserialize_id(output_buffer, output_buffer_num, answer_7[i][1]);
        output_buffer[output_buffer_num ++] = ',';
        deserialize_id(output_buffer, output_buffer_num, answer_7[i][2]);
        output_buffer[output_buffer_num ++] = ',';
        deserialize_id(output_buffer, output_buffer_num, answer_7[i][3]);
        output_buffer[output_buffer_num ++] = ',';
        deserialize_id(output_buffer, output_buffer_num, answer_7[i][4]);
        output_buffer[output_buffer_num ++] = ',';
        deserialize_id(output_buffer, output_buffer_num, answer_7[i][5]);
        output_buffer[output_buffer_num ++] = ',';
        deserialize_id(output_buffer, output_buffer_num, answer_7[i][6]);
        output_buffer[output_buffer_num ++] = '\n';
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
    

    // node_topo_filter(false);
    // node_topo_filter(true);
    // rehash_nodes();
    

    printf("node num: %d\n", node_num); fflush(stdout);

    return 0;

    filter_searchable_nodes();
    create_integer_buffer();

    sizeof_int_mul_node_num = sizeof(int) * node_num;
    for (int i=0; i<PATH_PAR_3; ++i) std_id_arr[i] = i;

    end_time = clock();
    printf("build edge: %d ms\n", (int)(end_time - start_time)); fflush(stdout);

    mid_time = clock();
    search();
    answer_num = answer_3_num + answer_4_num + answer_5_num + answer_6_num + answer_7_num;
    printf("answer: %d\n", answer_num);
    
    printf("total %d\n", total_dfs);

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