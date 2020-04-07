#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <assert.h>
#include <math.h>

#include <map>

#include <algorithm>

//------------------ MACRO ---------------------

#ifdef TEST

// 计时，应该删除
#include <time.h>
clock_t start_time, end_time;
#define PER_SEC     1000

// 日志输出
#include <stdio.h>
#include <iostream>
using namespace std;

// 输入输出路径
#define INPUT_PATH  "resources/data1.txt"
#define OUTPUT_PATH  "search_output.txt"

#else

#ifdef TEST_PATH

#define INPUT_PATH  "resources/data1.txt"
#define OUTPUT_PATH  "search_output.txt"

#else

#define INPUT_PATH  "/data/test_data.txt"
#define OUTPUT_PATH "/projects/student/result.txt"

#endif

#endif

// 估计数据的大小，来确定缓存的大小
// ATTENTION 数组大小会不会太大了
#define MAX_NODE                 28000
#define MAX_EDGE                 280000 // 数据最多28W行
#define MAX_FOR_EACH_LINE        33     // 每个32位正整数10个字符 + 2个逗号 + 一个换行
#define CIRCLE_LENGTH            7

#ifdef TEST
#define MAX_ANSWER               2800000 // 这个数值应该在思考后做调整
#else
#define MAX_ANSWER               10000000
#endif

//------------------ DECLR ---------------------

// *************** hash map *********************

#define C_HASH_MAP_SIZE    (1 << 16)

const int hash_map_mask = (1 << 16) - 1;
struct c_hash_map_t {
    int counter = 1;
    int header[C_HASH_MAP_SIZE];
    int key[MAX_NODE + 10], value[MAX_NODE + 10], ptr[MAX_NODE + 10];
};
typedef c_hash_map_t* c_hash_map;
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
int c_hash_map_get(c_hash_map hash, int key) {
    int index = c_hash_map_hash(key);
    for (int i=hash->header[index]; i!=0; i=hash->ptr[i]) {
        if (key == hash->key[i]) {
            return hash->value[i];
        }
    }
    return -1;
}
/**
 * 如果没有key就插入，并且返回value，可以少一次计算hash
 * 如果有就直接返回value
 */
int c_hash_map_insert_if_absent(c_hash_map hash, int key, int value) {
    int index = c_hash_map_hash(key);
    for (int i=hash->header[index]; i!=0; i=hash->ptr[i]) {
        if (key == hash->key[i]) {
            return hash->value[i];
        }
    }
    hash->key[hash->counter] = key;
    hash->value[hash->counter] = value;
    hash->ptr[hash->counter] = hash->header[index];
    hash->header[index] = hash->counter ++;
    return value;
}

int c_hash_map_insert_if_absent_with_inc(c_hash_map hash, int key, int& value, int* dehash) {
    int index = c_hash_map_hash(key);
    for (int i=hash->header[index]; i!=0; i=hash->ptr[i]) {
        if (key == hash->key[i]) {
            return hash->value[i];
        }
    }
    // 定制
    dehash[value] = key;
    hash->key[hash->counter] = key;
    hash->value[hash->counter] = value;
    hash->ptr[hash->counter] = hash->header[index];
    hash->header[index] = hash->counter ++;
    return value ++;
}

// *************** hash map over *********************
// *************** data structure *****************



// buffer
char buffer[MAX_EDGE * MAX_FOR_EACH_LINE];

// store data
int  data_num = 0;
int  data[MAX_EDGE][3];

// for normalize and denormalize, disordered
int unique_node_num = 0;
c_hash_map_t t_normalize_v;
c_hash_map normalize_v = &t_normalize_v;
int denormalize_v[MAX_NODE];

// ptr 0 for end pointer, so edge ptr begins from 1
int v_ptr_num = 1;
int edge_header[MAX_NODE];
int edge_v[MAX_EDGE + 1];
int edge_ptr[MAX_EDGE + 1];

// sp stands for shortest path
// 0 for infinity or self
short sp_matrix[MAX_NODE][MAX_NODE];
// 0 for not visit, (u+1) for u visit
int sp_visit[MAX_NODE];
int sp_queue[MAX_NODE];

bool search_visit[MAX_NODE];
int search_path[7];
int search_path_num = 0;
// int answers[MAX_ANSWER][7];

int* answers = (int*)malloc(sizeof(int) * MAX_ANSWER * 7);
inline void answers_set(int i, int j, int value) {
    answers[i * 7 + j] = value;
}
inline int answers_get(int i, int j) {
    return answers[i * 7 + j];
}

int answers_length[MAX_ANSWER];
int answer_num = 0;

int sorted_index[MAX_ANSWER];

int buffer_index = 0;
int writer_fd;

// ************** function *******************

void read_input();
void create_writer_fd();

// store edges as linked list while normalizing
void normalize();

void shortest_path();

// use search method of Jiayu.Zhou
void search();

void sort_answer();

void deserialize();

//------------------ MBODY ---------------------
int main() {
#ifdef TEST
    start_time = clock();
#endif
    read_input();
    normalize();
    shortest_path();
    search();

#ifdef TEST
    end_time = clock();
    cout << "searching: " << (end_time - start_time) << "ms" << endl << flush;
#endif
    sort_answer();
    create_writer_fd();
    deserialize();
    close(writer_fd);
    
#ifdef TEST
    end_time = clock();
    cout << "running: " << (end_time - start_time) << "ms" << endl << flush;
#endif
    return 0;
}

//------------------ FBODY ---------------------

void do_normalize() {
    for (int i=0; i<data_num; ++i) {
        data[i][0] = c_hash_map_insert_if_absent_with_inc(normalize_v, data[i][0], unique_node_num, denormalize_v);
        data[i][1] = c_hash_map_insert_if_absent_with_inc(normalize_v, data[i][1], unique_node_num, denormalize_v);
    }
    return;
}

// todo can this function merges with reader?
void normalize() {
    do_normalize();
    int u, v, ptr;    
    for (int i=0; i<data_num; ++i) {
        u = data[i][0]; v = data[i][1];
        edge_v[v_ptr_num] = v;
        edge_ptr[v_ptr_num] = edge_header[u];
        edge_header[u] = v_ptr_num ++;
    }
}

// bfs
void shortest_path_for_u(int start) {
    int mask = start + 1, u, v;
    int head = 0, tail = 1;
    sp_queue[0] = start;
    while (head < tail) {
        u = sp_queue[head ++];
        for (int i=edge_header[u]; i!=0; i=edge_ptr[i]) {
            v = edge_v[i];
            if (v == start) continue;
            if (sp_visit[v] == mask) continue;
            sp_visit[v] = mask;
            sp_matrix[start][v] = sp_matrix[start][u] + 1;
            if (sp_matrix[start][v] < 6) {
                sp_queue[tail ++] = v;
            }
        }
    }
}

void shortest_path() {
    for (int i=0; i<unique_node_num; ++i) {
        shortest_path_for_u(i);
    }
}


inline void extract_answer(int min_id) {
    answers_length[answer_num] = search_path_num;
    for (int i=0, j=min_id; i<search_path_num; ++i) {
        answers_set(answer_num, i, denormalize_v[search_path[j]]);
        j ++; if (j == search_path_num) j = 0;
    }
    answer_num ++;
}

// min_id stands for the minimal denormalized index of the search path
void do_search(int min_id) {
    int u, v;
    u = search_path[search_path_num-1];
    for (int i=edge_header[u]; i!=0; i=edge_ptr[i]) {
        v = edge_v[i];
        if (v < search_path[0]) continue;
        if (search_visit[v] == false) {
            if (v == search_path[0]) {
                if (search_path_num >=3 && search_path_num <= 7) {
                    extract_answer(min_id);
                }
                continue;
            } else {
                if (search_path_num == 7) continue;
                if (sp_matrix[v][search_path[0]] == 0 || sp_matrix[v][search_path[0]] + search_path_num + 1 > 8) {
                    continue;
                }
                search_visit[v] = true;
                search_path[search_path_num ++] = v;
                if (denormalize_v[v] <= denormalize_v[search_path[min_id]]) {
                    do_search(search_path_num-1);
                } else {
                    do_search(min_id);
                }
                search_visit[v] = false;
                search_path_num --;
            }
        }
    }
}

void search() {
    search_path_num = 1;
    for (int i=0; i<unique_node_num; ++i) {
        search_path[0] = i;
        do_search(0);
    }
}

bool compare_answer(int i, int j) {
    if (answers_length[i] != answers_length[j]) return answers_length[i] < answers_length[j];
    for (int x=0; x<answers_length[i]; ++x) {
        if (answers_get(i, x) != answers_get(j, x)) return answers_get(i, x) < answers_get(j, x);
    }
    return false;
}

void sort_answer() {
    for (int i=0; i<answer_num; ++i) sorted_index[i] = i;
    std::sort(sorted_index, sorted_index + answer_num, compare_answer);
}

int max_available = MAX_EDGE * MAX_FOR_EACH_LINE - 100;

inline void deserialize_int(int x) {
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
        assert(ret_size == buffer_index);
        buffer_index = 0;
    }
}

void deserialize() {
    deserialize_int(answer_num);
    buffer[buffer_index ++] = '\n';
    int len, min_id, x, id;
    for (int i=0; i<answer_num; ++i) {
        id = sorted_index[i];
        len = answers_length[id];
        for (int j=0; j<len; ++j) {
            deserialize_int(answers_get(id, j));
            if (j == len-1) {
                buffer[buffer_index ++] = '\n';
            } else {
                buffer[buffer_index ++] = ','; 
            }
        }
    }
    if (buffer_index > 0) {
        int ret_size = write(writer_fd, buffer, buffer_index);
        assert(ret_size == buffer_index);
        buffer_index = 0;
    }
    // buffer[buffer_index-1] = EOF;
}

void read_input() {
    int fd = open(INPUT_PATH, O_RDONLY);
    size_t size = read(fd, &buffer, MAX_EDGE * MAX_FOR_EACH_LINE);
    // 这一步没啥必要？
    close(fd);

    // 解析
    int x = 0;
    int* data_ptr = &data[0][0];
    for (int i=0; i<size; ++i) {
        if (buffer[i] == ',' || buffer[i] == '\n') {
            *data_ptr = x; data_ptr ++;
            x = 0;
        } else {
            x = x * 10 + buffer[i] - '0';
        }
    }
    data_num = (data_ptr - &data[0][0]) / 3;
}

void create_writer_fd() {
#ifdef TEST
    writer_fd = open(OUTPUT_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0666);
#else
    writer_fd = open(OUTPUT_PATH, O_WRONLY | O_CREAT, 0666);
#endif
}