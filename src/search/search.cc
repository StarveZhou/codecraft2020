#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <unordered_map>
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
#define say_out(...)     printf(__VA_ARGS__);putchar('\n');fflush(__acrt_iob_func(1))


// 输入输出路径
// #define INPUT_PATH  "resources/data1.txt"
// #define INPUT_PATH  "gen_data.txt"
#define INPUT_PATH  "resources/test_data.txt"
#define OUTPUT_PATH "test_output.txt"

#else
// 输入输出路径
#define INPUT_PATH  "/data/test_data.txt"
#define OUTPUT_PATH "/projects/student/result.txt"

#endif

// 估计数据的大小，来确定缓存的大小
// ATTENTION 数组大小会不会太大了
#define MAX_NODE                 2800
#define MAX_EDGE                 28000 // 数据最多28W行
#define MAX_FOR_EACH_LINE        33     // 每个32位正整数10个字符 + 2个逗号 + 一个换行
#define CIRCLE_LENGTH            7
#define MAX_ANSWER               2800000 // 这个数值应该在思考后做调整

//------------------ DECLR ---------------------
// *************** data structure *****************

// buffer
char buffer[MAX_EDGE * MAX_FOR_EACH_LINE];

// store data
int  data_num = 0;
int  data[MAX_EDGE][3];

// for normalize and denormalize, disordered
int unique_node_num = 0;
std::map<int, int> normalize_v;
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
int queue[MAX_NODE];


bool search_visit[MAX_NODE];
int search_path[7];
int search_path_num = 0;
int answers[MAX_ANSWER][7];
int answers_length[MAX_ANSWER];
int answers_min_id[MAX_ANSWER];
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
    say_out("read: %d", data_num);
    normalize();
    say_out("unique: %d", unique_node_num);
    shortest_path();
    say_out("shortest path");
    search();
    say_out("search: %d", answer_num);
    sort_answer();
    say_out("sort");
    create_writer_fd();
    say_out("create writer fd");
    deserialize();
    say_out("deserialize: %d", buffer_index);
    

#ifdef TEST
    end_time = clock();
    say_out("running: %.6lf", ((double)(end_time - start_time) / PER_SEC));
#endif
    return 0;
}

//------------------ FBODY ---------------------

std::map<int, int>::iterator n_it;
inline void normalize_for_one_item(int& u) {
    say_out("item: %d", u);
    n_it = normalize_v.find(u);
    say_out("what?");
    if (n_it == normalize_v.end()) {
        say_out("A");
        normalize_v[u] = unique_node_num;
        denormalize_v[unique_node_num] = u;
        u = unique_node_num;
        unique_node_num ++;
    } else {
        say_out("B");
        u = n_it->second;
    }
    say_out("out: %d", u);
}

// todo can this function merges with reader?
void normalize() {
    int u, v, ptr;
    for (int i=0; i<data_num; ++i) {
        u = data[i][0]; v = data[i][1];
        say_out("-u,v: %d, %d, %d", u, v, i);

        normalize_for_one_item(u);
        normalize_for_one_item(v);

        say_out("*u,v: %d, %d, %d", u, v, i);

        edge_v[v_ptr_num] = v;
        edge_ptr[v_ptr_num] = edge_header[u];
        edge_header[u] = v_ptr_num ++;
    }
}

// bfs
void shortest_path_for_u(int start) {
    int mask = start + 1, u, v;
    int head = 0, tail = 1;
    queue[0] = start;
    while (head < tail) {
        u = queue[head ++];
        for (int i=edge_header[u]; i!=0; i=edge_ptr[i]) {
            v = edge_v[i];
            if (v == start) continue;
            if (sp_visit[v] == mask) continue;
            sp_visit[v] = mask;
            sp_matrix[start][v] = sp_matrix[start][u] + 1;
            if (sp_matrix[start][v] < 6) {
                queue[tail ++] = v;
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
    for (int i=0; i<search_path_num; ++i) {
        answers[answer_num][i] = search_path[i];
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
    // say_out("compare i, j: %d, %d", i, j);
    if (answers_length[i] != answers_length[j]) return answers_length[i] < answers_length[j];
    int x = answers_min_id[i], y = answers_min_id[j], len = answers_length[i];
    while (true) {
        if (answers[i][x] != answers[j][y]) {
            return denormalize_v[answers[i][x]] < denormalize_v[answers[j][y]];
        }
        x ++; if (x == len) x = 0;
        y ++; if (y == len) y = 0;
    }
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
        write(writer_fd, buffer, buffer_index);
        say_out("write buffer: %d", buffer_index);
        buffer_index = 0;
    }
}

void deserialize() {
    deserialize_int(answer_num);
    buffer[buffer_index ++] = '\n';
    int* path;
    int len, min_id, x;
    for (int i=0; i<answer_num; ++i) {
        path = answers[sorted_index[i]];
        len = answers_length[sorted_index[i]];
        x = answers_min_id[sorted_index[i]];
        for (int j=0; j<len; ++j) {
            deserialize_int(denormalize_v[path[x]]);
            x ++; if (x >= len) x = 0;
            if (j == len-1) {
                buffer[buffer_index ++] = '\n';
            } else {
                buffer[buffer_index ++] = ','; 
            }
        }
    }
    if (buffer_index > 0) {
        write(writer_fd, buffer, buffer_index);
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
    writer_fd = open(OUTPUT_PATH, O_WRONLY | O_CREAT);
}