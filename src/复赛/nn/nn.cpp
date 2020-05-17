#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <pthread.h>

#include <string.h>


#include <algorithm>
#include <unordered_map>

//------------------ MACRO ---------------------

#include <stdio.h>

// #define INPUT_PATH     "resources/test_data.txt"
// #define OUTPUT_PATH    "output/result.txt"

#define INPUT_PATH  "/data/test_data.txt"
#define OUTPUT_PATH "/projects/student/result.txt"

#define MAX_NODE               4000000
#define MAX_EDGE               2000000
#define MAX_ANSW               20000000
#define MAX_FOR_EACH_LINE      34
#define ONE_INT_CHAR_SIZE      11
#define MAX_3_ANSW             2000000
#define MAX_4_ANSW             5000000
#define PATH_PAR_2             5000000
#define PATH_PAR_3             10000000
#define PATH_PAR_4             20000000


#define CACHE_LINE_ALIGN       __attribute__((aligned(64)))
#define likely(x)             __builtin_expect((x),1)
#define unlikely(x)           __builtin_expect((x),0)


// ***************************************************************







char* buffer;

int CACHE_LINE_ALIGN data[MAX_EDGE][3];
int CACHE_LINE_ALIGN data_num;

int CACHE_LINE_ALIGN data_rev_mapping[MAX_NODE];
int CACHE_LINE_ALIGN node_num = 0;



void read_input() {
    buffer = (char*) malloc(MAX_EDGE * MAX_FOR_EACH_LINE * sizeof(char));
    // printf("before %lld\n", buffer); fflush(stdout);
    int fd = open(INPUT_PATH, O_RDONLY);
    if (fd == -1) {
        printf("fail to open\n"); fflush(stdout);
        exit(0);
    }
    // printf("before read\n"); fflush(stdout);
    size_t size = read(fd, buffer, MAX_EDGE * MAX_FOR_EACH_LINE);
    // 这一步没啥必要？
    close(fd);
    // printf("closed %d %d #%c#\n", MAX_EDGE * MAX_FOR_EACH_LINE, size, buffer[size-1]); fflush(stdout);
    // 解析
    // printf("what ? %d\n", size < MAX_EDGE * MAX_FOR_EACH_LINE); fflush(stdout);
    if (buffer[size-1] >= '0' && buffer[size-1] <= '9') {
        buffer[size ++] = '\n';
    }
    int x = 0;
    int* local_data = &data[0][0];
    for (int i=0; i<size; ++i) {
        // printf("A "); fflush(stdout);
        if (unlikely(buffer[i] == ',' || buffer[i] == '\n')) {
            *local_data = x;
            local_data ++;
            x = 0;
        } else if (buffer[i] >= '0' && buffer[i] <= '9') {
            x = x * 10 + buffer[i] - '0';
        }
        // printf("%d\n", i); fflush(stdout);
    }
    data_num = (local_data - &data[0][0]) / 3;
    
    // printf("after data num %d\n", data_num); fflush(stdout);
    std::unordered_map<int, int> hashmap;
    for (int i=0; i<data_num; ++i) {
        x = data[i][0];
        if (hashmap.find(x) == hashmap.end()) {
            hashmap[x] = node_num;
            data_rev_mapping[node_num ++] = x;
        }
        x = data[i][1];
        if (hashmap.find(x) == hashmap.end()) {
            hashmap[x] = node_num;
            data_rev_mapping[node_num ++] = x;
        }
    }

    std::sort(data_rev_mapping, data_rev_mapping + node_num);


    for (int i=0; i<node_num; ++i) {
        hashmap[data_rev_mapping[i]] = i;
    }

    for (int i=0; i<data_num; ++i) {
        data[i][0] = hashmap[data[i][0]];
        data[i][1] = hashmap[data[i][1]];
    }

    free(buffer);
    printf("node num: %d\n", node_num);
}

int writer_fd;
void create_writer_fd() {
    writer_fd = open(OUTPUT_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (writer_fd == -1) {
        exit(0);
    }
}


int* edge_sorted_id = (int*) malloc(sizeof(int) * MAX_EDGE);
int* rev_edge_sorted_id = (int*) malloc(sizeof(int) * MAX_EDGE);

int* edge_counter = (int*) malloc(sizeof(int) * MAX_NODE);
int* rev_edge_counter = (int*) malloc(sizeof(int) * MAX_NODE);

int CACHE_LINE_ALIGN edge_topo_header[MAX_NODE];
int CACHE_LINE_ALIGN edge_topo_edges[MAX_EDGE];
int CACHE_LINE_ALIGN edge_topo_values[MAX_EDGE];

int CACHE_LINE_ALIGN rev_edge_topo_header[MAX_NODE];
int CACHE_LINE_ALIGN rev_edge_topo_edges[MAX_NODE];
int CACHE_LINE_ALIGN rev_edge_topo_values[MAX_NODE];

void rehash_nodes() {
    int u, v;
    memset(edge_counter, 0, sizeof(edge_counter));
    memset(rev_edge_counter, 0, sizeof(rev_edge_counter));
    for (int i=0; i<data_num; ++i) {
        u = data[i][0]; v = data[i][1];
        edge_counter[u] ++;
        rev_edge_counter[v] ++;
    }

    for (int i=1; i<node_num; ++i) {
        edge_counter[i] += edge_counter[i-1];
        rev_edge_counter[i] += rev_edge_counter[i-1];
        
        edge_topo_header[i] = edge_counter[i-1];
        rev_edge_topo_header[i] = rev_edge_counter[i-1];
    }

    for (int i=0; i<data_num; ++i) {
        u = data[i][0]; v = data[i][1];
        edge_sorted_id[edge_topo_header[u] ++] = i;
        rev_edge_sorted_id[rev_edge_topo_header[v] ++] = i;
    }

    for (int i=node_num; i>0; --i) {
        edge_topo_header[i] = edge_topo_header[i-1];
        rev_edge_topo_header[i] = rev_edge_topo_header[i-1];
    }
    edge_topo_header[0] = rev_edge_topo_header[0] = 0;

    for (int i=0; i<node_num; ++i) {
        std::sort(edge_sorted_id + edge_topo_header[i], edge_sorted_id + edge_topo_header[i+1], [](int i, int j) {
            return data[i][1] < data[j][1];
        });
        std::sort(rev_edge_sorted_id + rev_edge_topo_header[i], rev_edge_sorted_id + rev_edge_topo_header[i+1], [](int i, int j) {
            return data[i][0] > data[j][0];
        });
    }

    int index;
    for (int i=0; i<data_num; ++i) {
        index = edge_sorted_id[i];
        edge_topo_edges[i] = data[index][1];
        edge_topo_values[i] = data[index][2];

        index = rev_edge_sorted_id[i];
        rev_edge_topo_edges[i] = data[index][0];
        rev_edge_topo_values[i] = data[index][2];
    }

    free(edge_sorted_id);
    free(rev_edge_sorted_id);
    free(edge_counter);
    free(rev_edge_counter);
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

    // for (int i=0; i<node_num; ++i) {
    //     printf("%d %d: ", i, data_rev_mapping[i]);
    //     for (int j=0; j<integer_buffer_size[i]; ++j) {
    //         putchar(integer_buffer[i][j]);
    //     }
    //     putchar('\n');
    // }
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

inline bool check_x_y(long long x, long long y) {
    return x <= 5*y && y <= 3*x; 
}

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
int answer_header[MAX_NODE][5], answer_tail[MAX_NODE][5];

// search mt_0

int* answer_mt_0[5];
int* answer_fake_mt_0[2];
int answer_num_mt_0[5], total_answer_num_mt_0 = 0;

int* malloc_all_mt_0;

int* edge_topo_starter_mt_0;

int* back_1_mt_0, *back_2_mt_0, *back_value_back_mt_0, *back_value_front_mt_0;
int* back_header_mt_0, *back_nxt_mt_0;
int back_num_mt_0;

int mask_mt_0 = 123;
int* back_visit_mt_0;
int* back_sorted_visit_mt_0;
int* back_sorted_index_mt_0, *back_sorted_from_mt_0, *back_sorted_to_mt_0;
int back_sorted_num_mt_0;

int* front_visit_mt_0, *visit_mt_0;
int* front_value_mt_0;

void malloc_mt_0() {
    answer_fake_mt_0[0] = (int*) malloc(sizeof(int) * MAX_BACK * 2);
    answer_fake_mt_0[1] = (int*) malloc(sizeof(int) * MAX_BACK * 2);
    
    answer_mt_0[0] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_0[1] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_0[2] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_0[3] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_0[4] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);

    int total_size = node_num + MAX_BACK + MAX_BACK + MAX_BACK + MAX_BACK + node_num + 
        MAX_BACK + node_num + node_num + node_num + node_num + MAX_BACK
        + node_num + node_num + node_num;
    int size_now = 0;
    malloc_all_mt_0 = (int*) malloc(sizeof(int) * total_size);

    edge_topo_starter_mt_0 = malloc_all_mt_0 + size_now; size_now += node_num;

    back_1_mt_0 = malloc_all_mt_0 + size_now; size_now += MAX_BACK;
    back_2_mt_0 = malloc_all_mt_0 + size_now; size_now += MAX_BACK;
    back_value_back_mt_0 = malloc_all_mt_0 + size_now; size_now += MAX_BACK;
    back_value_front_mt_0 = malloc_all_mt_0 + size_now; size_now += MAX_BACK;
    back_header_mt_0 = malloc_all_mt_0 + size_now; size_now += node_num;
    back_nxt_mt_0 = malloc_all_mt_0 + size_now; size_now += MAX_BACK;

    back_visit_mt_0 = malloc_all_mt_0 + size_now; size_now += node_num;
    back_sorted_visit_mt_0 = malloc_all_mt_0 + size_now; size_now += node_num;
    back_sorted_index_mt_0 = malloc_all_mt_0 + size_now; size_now += MAX_BACK;
    back_sorted_from_mt_0 = malloc_all_mt_0 + size_now; size_now += node_num;
    back_sorted_to_mt_0 = malloc_all_mt_0 + size_now; size_now += node_num;

    front_visit_mt_0 = malloc_all_mt_0 + size_now; size_now += node_num;
    front_value_mt_0 = malloc_all_mt_0 + size_now; size_now += node_num;

    visit_mt_0 = malloc_all_mt_0 + size_now; size_now += node_num;

    memset(back_visit_mt_0, 0, sizeof(int) * node_num);
    memset(back_sorted_visit_mt_0, 0, sizeof(int) * node_num);
    memset(front_visit_mt_0, 0, sizeof(int) * node_num);
    memset(visit_mt_0, 0, sizeof(int) * node_num);
}

void free_mt_0() {
    free(malloc_all_mt_0);
    free(answer_fake_mt_0[0]);
    free(answer_fake_mt_0[1]);
}

inline void refresh_starter_mt_0(int u, int starter) {
    while (edge_topo_starter_mt_0[u] < edge_topo_header[u+1] 
        && edge_topo_edges[edge_topo_starter_mt_0[u]] <= starter)
        edge_topo_starter_mt_0[u] ++;
}

void gain_sorted_back_mt_0(int u) {
    if (back_sorted_visit_mt_0[u] == mask_mt_0) return;
    if (back_visit_mt_0[u] != mask_mt_0) {
        back_sorted_from_mt_0[u] = back_sorted_to_mt_0[u] = 0;
        return;
    }
    back_sorted_visit_mt_0[u] = mask_mt_0;
    back_sorted_from_mt_0[u] = back_sorted_num_mt_0;
    for (int i=back_header_mt_0[u]; i!=-1; i=back_nxt_mt_0[i]) {
        back_sorted_index_mt_0[back_sorted_num_mt_0 ++] = i;
    }
    back_sorted_to_mt_0[u] = back_sorted_num_mt_0;
    std::sort(back_sorted_index_mt_0 + back_sorted_from_mt_0[u], back_sorted_index_mt_0 + back_sorted_to_mt_0[u],
        [](int i, int j) {
            if (back_2_mt_0[i] != back_2_mt_0[j]) return back_2_mt_0[i] < back_2_mt_0[j];
            return back_1_mt_0[i] < back_1_mt_0[j];
        });
}

void do_back_search_mt_0(int starter) {
    back_num_mt_0 = 0;
    int fake_num[2]; fake_num[0] = fake_num[1] = 0;

    refresh_starter_mt_0(starter, starter);
    int u, v, w, x, y, z, o, index;
    for (int i=edge_topo_starter_mt_0[starter]; i<edge_topo_header[starter+1]; ++i) {
        u = edge_topo_edges[i]; x = edge_topo_values[i];
        front_visit_mt_0[u] = mask_mt_0;
        front_value_mt_0[u] = x;
    }

    for (int i=rev_edge_topo_header[starter]; i<rev_edge_topo_header[starter+1]; ++i) {
        u = rev_edge_topo_edges[i]; x = rev_edge_topo_values[i];
        if (u <= starter) break;
        // printf("u: %d\n", u);
        for (int j=rev_edge_topo_header[u]; j<rev_edge_topo_header[u+1]; ++j) {
            v = rev_edge_topo_edges[j]; y = rev_edge_topo_values[j];
            // printf("v0: %d\n", v);
            if (v <= starter) break;
            // printf("v1: %d\n", v);
            if (!check_x_y(y, x)) continue;
            // printf("v: %d\n", v);
            // circle 3
            if (front_visit_mt_0[v] == mask_mt_0) {
                z = front_value_mt_0[v];
                if (check_x_y(z, y) && check_x_y(x, z)) {
                    index = fake_num[0];
                    answer_fake_mt_0[0][index ++] = v;
                    answer_fake_mt_0[0][index ++] = u;
                    fake_num[0] += 2;
                    total_answer_num_mt_0 ++;
                    // printf("%d %d %d\n", data_rev_mapping[starter], data_rev_mapping[v], data_rev_mapping[u]);
                }
            }

            for (int k=rev_edge_topo_header[v]; k<rev_edge_topo_header[v+1]; ++k) {
                w = rev_edge_topo_edges[k]; z = rev_edge_topo_values[k];
                if (w <= starter) break;
                if (w == u) continue;
                if (!check_x_y(z, y)) continue;
                // printf("w: %d\n", w);
                // circle 4
                if (front_visit_mt_0[w] == mask_mt_0) {
                    o = front_value_mt_0[w];
                    if (check_x_y(x, o) && check_x_y(o, z)) {
                        index = fake_num[1];
                        answer_fake_mt_0[1][index ++] = w;
                        answer_fake_mt_0[1][index ++] = v;
                        answer_fake_mt_0[1][index ++] = u;
                        fake_num[1] += 3;
                        total_answer_num_mt_0 ++;
                        // printf("%d %d %d %d\n", data_rev_mapping[starter], data_rev_mapping[w], data_rev_mapping[v], data_rev_mapping[u]);
                    }
                }

                if (back_visit_mt_0[w] != mask_mt_0) {
                    back_visit_mt_0[w] = mask_mt_0;
                    back_header_mt_0[w] = -1;
                }
                index = back_num_mt_0 ++;
                back_value_front_mt_0[index] = x;
                back_value_back_mt_0[index] = z;
                back_1_mt_0[index] = u;
                back_2_mt_0[index] = v;
                back_nxt_mt_0[index] = back_header_mt_0[w];
                back_header_mt_0[w] = index;
            }
        }
    }

    int index_num;
    if (fake_num[0] > 0) {
        index_num = 0;
        for (int i=0; i<fake_num[0]; i+=2) back_sorted_index_mt_0[index_num ++] = i;
        std::sort(back_sorted_index_mt_0, back_sorted_index_mt_0 + index_num, [](int i, int j) {
            if (answer_fake_mt_0[0][i] != answer_fake_mt_0[0][j]) return answer_fake_mt_0[0][i] < answer_fake_mt_0[0][j];
            return answer_fake_mt_0[0][i+1] < answer_fake_mt_0[0][j+1];
        });
        for (int i=0; i<index_num; ++i) {
            index = back_sorted_index_mt_0[i];
            answer_mt_0[0][answer_num_mt_0[0] ++] = answer_fake_mt_0[0][index];
            answer_mt_0[0][answer_num_mt_0[0] ++] = answer_fake_mt_0[0][index+1];
        }
    }
    if (fake_num[1] > 0) {
        index_num = 0;
        for (int i=0; i<fake_num[1]; i+=3) back_sorted_index_mt_0[index_num ++] = i;
        std::sort(back_sorted_index_mt_0, back_sorted_index_mt_0 + index_num, [](int i, int j) {
            if (answer_fake_mt_0[1][i] != answer_fake_mt_0[1][j]) return answer_fake_mt_0[1][i] < answer_fake_mt_0[1][j];
            if (answer_fake_mt_0[1][i+1] != answer_fake_mt_0[1][j+1]) return answer_fake_mt_0[1][i+1] < answer_fake_mt_0[1][j+1];
            return answer_fake_mt_0[1][i+2] < answer_fake_mt_0[1][j+2];
        });
        for (int i=0; i<index_num; ++i) {
            index = back_sorted_index_mt_0[i];
            answer_mt_0[1][answer_num_mt_0[1] ++] = answer_fake_mt_0[1][index];
            answer_mt_0[1][answer_num_mt_0[1] ++] = answer_fake_mt_0[1][index+1];
            answer_mt_0[1][answer_num_mt_0[1] ++] = answer_fake_mt_0[1][index+2];
        }
    }
}

void do_front_search_mt_0(int starter) {
    int x, y, z, t, o, f;
    int u, v, w, e, index, ai;
    int back1, back2;

    visit_mt_0[starter] = mask_mt_0;
    refresh_starter_mt_0(starter, starter);
    for (int i=edge_topo_starter_mt_0[starter]; i<edge_topo_header[starter+1]; ++i) {
        u = edge_topo_edges[i]; x = edge_topo_values[i];
        visit_mt_0[u] = mask_mt_0;
        // printf("u: %d\n", u);
        refresh_starter_mt_0(u, starter);
        for (int j=edge_topo_starter_mt_0[u]; j<edge_topo_header[u+1]; ++j) {
            v = edge_topo_edges[j]; y = edge_topo_values[j];
            if (!check_x_y(x, y)) continue;
            visit_mt_0[v] = mask_mt_0;
            // printf("v: %d\n", v);
            // circle 5
            gain_sorted_back_mt_0(v);
            for (int r=back_sorted_from_mt_0[v]; r<back_sorted_to_mt_0[v]; ++r) {
                index = back_sorted_index_mt_0[r];
                o = back_value_back_mt_0[index];
                f = back_value_front_mt_0[index];
                back1 = back_1_mt_0[index];
                back2 = back_2_mt_0[index];
                if (check_x_y(f, x) && check_x_y(y, o) && visit_mt_0[back1] != mask_mt_0 && visit_mt_0[back2] != mask_mt_0) {
                    ai = answer_num_mt_0[2];
                    answer_mt_0[2][ai ++] = u;
                    answer_mt_0[2][ai ++] = v;
                    answer_mt_0[2][ai ++] = back2;
                    answer_mt_0[2][ai ++] = back1;
                    answer_num_mt_0[2] += 4;
                    total_answer_num_mt_0 ++;
                }
            }

            refresh_starter_mt_0(v, starter);
            for (int k=edge_topo_starter_mt_0[v]; k<edge_topo_header[v+1]; ++k) {
                w = edge_topo_edges[k]; z = edge_topo_values[k];
                if (!check_x_y(y, z)) continue;
                if (u == w) continue;
                visit_mt_0[w] = mask_mt_0;
                // printf("w: %d\n", w);
                // circle 6
                gain_sorted_back_mt_0(w);
                for (int r=back_sorted_from_mt_0[w]; r<back_sorted_to_mt_0[w]; ++r) {
                    index = back_sorted_index_mt_0[r];
                    o = back_value_back_mt_0[index];
                    f = back_value_front_mt_0[index];
                    back1 = back_1_mt_0[index];
                    back2 = back_2_mt_0[index];
                    if (check_x_y(f, x) && check_x_y(z, o) && visit_mt_0[back1] != mask_mt_0 && visit_mt_0[back2] != mask_mt_0) {
                        ai = answer_num_mt_0[3];
                        answer_mt_0[3][ai ++] = u;
                        answer_mt_0[3][ai ++] = v;
                        answer_mt_0[3][ai ++] = w;
                        answer_mt_0[3][ai ++] = back2;
                        answer_mt_0[3][ai ++] = back1;
                        answer_num_mt_0[3] += 5;
                        total_answer_num_mt_0 ++;
                    }
                }

                refresh_starter_mt_0(w, starter);
                for (int l=edge_topo_starter_mt_0[w]; l<edge_topo_header[w+1]; ++l) {
                    e = edge_topo_edges[l]; t = edge_topo_values[l];
                    if (!check_x_y(z, t)) continue;
                    if (e == v || e == u) continue;
                    visit_mt_0[e] = mask_mt_0;
                    // printf("e: %d\n", e);
                    // circle 7
                    gain_sorted_back_mt_0(e);
                    for (int r=back_sorted_from_mt_0[e]; r<back_sorted_to_mt_0[e]; ++r) {
                        index = back_sorted_index_mt_0[r];
                        o = back_value_back_mt_0[index];
                        f = back_value_front_mt_0[index];
                        back1 = back_1_mt_0[index];
                        back2 = back_2_mt_0[index];
                        if (check_x_y(f, x) && check_x_y(t, o) && visit_mt_0[back1] != mask_mt_0 && visit_mt_0[back2] != mask_mt_0) {
                            ai = answer_num_mt_0[4];
                            answer_mt_0[4][ai ++] = u;
                            answer_mt_0[4][ai ++] = v;
                            answer_mt_0[4][ai ++] = w;
                            answer_mt_0[4][ai ++] = e;
                            answer_mt_0[4][ai ++] = back2;
                            answer_mt_0[4][ai ++] = back1;
                            answer_num_mt_0[4] += 6;
                            total_answer_num_mt_0 ++;
                        }
                    }
                    visit_mt_0[e] = -1;
                }
                visit_mt_0[w] = -1;
            }
            visit_mt_0[v] = -1;
        }
        visit_mt_0[u] = -1;
    }
}

void do_search_mt_0(int starter) {
    mask_mt_0 ++;
    back_sorted_num_mt_0 = 0;
    do_back_search_mt_0(starter);
    if (back_num_mt_0 == 0) return;
    do_front_search_mt_0(starter);
}

void* search_mt_0(void* args) {
    malloc_mt_0();
    memcpy(edge_topo_starter_mt_0, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_counter, 1);
        if (u >= node_num) break;
        global_assign[u] = 0;
        for (int i=0; i<5; ++i) {
            answer_header[u][i] = answer_num_mt_0[i];
        }
        // printf("starter: %d\n", u);
        // printf("starter: %d %d %d %d\n", u, data_rev_mapping[u], answer_header[u][0], answer_header[u][1]);
        do_search_mt_0(u);
        // printf("\n");
        for (int i=0; i<5; ++i) {
            answer_tail[u][i] = answer_num_mt_0[i];
        }
        // printf("ender: %d %d %d\n", u, answer_tail[u][0], answer_tail[u][1]);
    }
    free_mt_0();
    return NULL;
}

// search mt_0
















































// search mt_1

int* answer_mt_1[5];
int* answer_fake_mt_1[2];
int answer_num_mt_1[5], total_answer_num_mt_1 = 0;

int* malloc_all_mt_1;

int* edge_topo_starter_mt_1;

int* back_1_mt_1, *back_2_mt_1, *back_value_back_mt_1, *back_value_front_mt_1;
int* back_header_mt_1, *back_nxt_mt_1;
int back_num_mt_1;

int mask_mt_1 = 123;
int* back_visit_mt_1;
int* back_sorted_visit_mt_1;
int* back_sorted_index_mt_1, *back_sorted_from_mt_1, *back_sorted_to_mt_1;
int back_sorted_num_mt_1;

int* front_visit_mt_1, *visit_mt_1;
int* front_value_mt_1;

void malloc_mt_1() {
    answer_fake_mt_1[0] = (int*) malloc(sizeof(int) * MAX_BACK * 2);
    answer_fake_mt_1[1] = (int*) malloc(sizeof(int) * MAX_BACK * 2);
    
    answer_mt_1[0] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_1[1] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_1[2] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_1[3] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_1[4] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);

    int total_size = node_num + MAX_BACK + MAX_BACK + MAX_BACK + MAX_BACK + node_num + 
        MAX_BACK + node_num + node_num + node_num + node_num + MAX_BACK
        + node_num + node_num + node_num;
    int size_now = 0;
    malloc_all_mt_1 = (int*) malloc(sizeof(int) * total_size);

    edge_topo_starter_mt_1 = malloc_all_mt_1 + size_now; size_now += node_num;

    back_1_mt_1 = malloc_all_mt_1 + size_now; size_now += MAX_BACK;
    back_2_mt_1 = malloc_all_mt_1 + size_now; size_now += MAX_BACK;
    back_value_back_mt_1 = malloc_all_mt_1 + size_now; size_now += MAX_BACK;
    back_value_front_mt_1 = malloc_all_mt_1 + size_now; size_now += MAX_BACK;
    back_header_mt_1 = malloc_all_mt_1 + size_now; size_now += node_num;
    back_nxt_mt_1 = malloc_all_mt_1 + size_now; size_now += MAX_BACK;

    back_visit_mt_1 = malloc_all_mt_1 + size_now; size_now += node_num;
    back_sorted_visit_mt_1 = malloc_all_mt_1 + size_now; size_now += node_num;
    back_sorted_index_mt_1 = malloc_all_mt_1 + size_now; size_now += MAX_BACK;
    back_sorted_from_mt_1 = malloc_all_mt_1 + size_now; size_now += node_num;
    back_sorted_to_mt_1 = malloc_all_mt_1 + size_now; size_now += node_num;

    front_visit_mt_1 = malloc_all_mt_1 + size_now; size_now += node_num;
    front_value_mt_1 = malloc_all_mt_1 + size_now; size_now += node_num;

    visit_mt_1 = malloc_all_mt_1 + size_now; size_now += node_num;

    memset(back_visit_mt_1, 0, sizeof(int) * node_num);
    memset(back_sorted_visit_mt_1, 0, sizeof(int) * node_num);
    memset(front_visit_mt_1, 0, sizeof(int) * node_num);
    memset(visit_mt_1, 0, sizeof(int) * node_num);
}

void free_mt_1() {
    free(malloc_all_mt_1);
    free(answer_fake_mt_1[0]);
    free(answer_fake_mt_1[1]);
}

inline void refresh_starter_mt_1(int u, int starter) {
    while (edge_topo_starter_mt_1[u] < edge_topo_header[u+1] 
        && edge_topo_edges[edge_topo_starter_mt_1[u]] <= starter)
        edge_topo_starter_mt_1[u] ++;
}

void gain_sorted_back_mt_1(int u) {
    if (back_sorted_visit_mt_1[u] == mask_mt_1) return;
    if (back_visit_mt_1[u] != mask_mt_1) {
        back_sorted_from_mt_1[u] = back_sorted_to_mt_1[u] = 0;
        return;
    }
    back_sorted_visit_mt_1[u] = mask_mt_1;
    back_sorted_from_mt_1[u] = back_sorted_num_mt_1;
    for (int i=back_header_mt_1[u]; i!=-1; i=back_nxt_mt_1[i]) {
        back_sorted_index_mt_1[back_sorted_num_mt_1 ++] = i;
    }
    back_sorted_to_mt_1[u] = back_sorted_num_mt_1;
    std::sort(back_sorted_index_mt_1 + back_sorted_from_mt_1[u], back_sorted_index_mt_1 + back_sorted_to_mt_1[u],
        [](int i, int j) {
            if (back_2_mt_1[i] != back_2_mt_1[j]) return back_2_mt_1[i] < back_2_mt_1[j];
            return back_1_mt_1[i] < back_1_mt_1[j];
        });
}

void do_back_search_mt_1(int starter) {
    back_num_mt_1 = 0;
    int fake_num[2]; fake_num[0] = fake_num[1] = 0;

    refresh_starter_mt_1(starter, starter);
    int u, v, w, x, y, z, o, index;
    for (int i=edge_topo_starter_mt_1[starter]; i<edge_topo_header[starter+1]; ++i) {
        u = edge_topo_edges[i]; x = edge_topo_values[i];
        front_visit_mt_1[u] = mask_mt_1;
        front_value_mt_1[u] = x;
    }

    for (int i=rev_edge_topo_header[starter]; i<rev_edge_topo_header[starter+1]; ++i) {
        u = rev_edge_topo_edges[i]; x = rev_edge_topo_values[i];
        if (u <= starter) break;
        // printf("u: %d\n", u);
        for (int j=rev_edge_topo_header[u]; j<rev_edge_topo_header[u+1]; ++j) {
            v = rev_edge_topo_edges[j]; y = rev_edge_topo_values[j];
            // printf("v0: %d\n", v);
            if (v <= starter) break;
            // printf("v1: %d\n", v);
            if (!check_x_y(y, x)) continue;
            // printf("v: %d\n", v);
            // circle 3
            if (front_visit_mt_1[v] == mask_mt_1) {
                z = front_value_mt_1[v];
                if (check_x_y(z, y) && check_x_y(x, z)) {
                    index = fake_num[0];
                    answer_fake_mt_1[0][index ++] = v;
                    answer_fake_mt_1[0][index ++] = u;
                    fake_num[0] += 2;
                    total_answer_num_mt_1 ++;
                    // printf("%d %d %d\n", data_rev_mapping[starter], data_rev_mapping[v], data_rev_mapping[u]);
                }
            }

            for (int k=rev_edge_topo_header[v]; k<rev_edge_topo_header[v+1]; ++k) {
                w = rev_edge_topo_edges[k]; z = rev_edge_topo_values[k];
                if (w <= starter) break;
                if (w == u) continue;
                if (!check_x_y(z, y)) continue;
                // printf("w: %d\n", w);
                // circle 4
                if (front_visit_mt_1[w] == mask_mt_1) {
                    o = front_value_mt_1[w];
                    if (check_x_y(x, o) && check_x_y(o, z)) {
                        index = fake_num[1];
                        answer_fake_mt_1[1][index ++] = w;
                        answer_fake_mt_1[1][index ++] = v;
                        answer_fake_mt_1[1][index ++] = u;
                        fake_num[1] += 3;
                        total_answer_num_mt_1 ++;
                        // printf("%d %d %d %d\n", data_rev_mapping[starter], data_rev_mapping[w], data_rev_mapping[v], data_rev_mapping[u]);
                    }
                }

                if (back_visit_mt_1[w] != mask_mt_1) {
                    back_visit_mt_1[w] = mask_mt_1;
                    back_header_mt_1[w] = -1;
                }
                index = back_num_mt_1 ++;
                back_value_front_mt_1[index] = x;
                back_value_back_mt_1[index] = z;
                back_1_mt_1[index] = u;
                back_2_mt_1[index] = v;
                back_nxt_mt_1[index] = back_header_mt_1[w];
                back_header_mt_1[w] = index;
            }
        }
    }

    int index_num;
    if (fake_num[0] > 0) {
        index_num = 0;
        for (int i=0; i<fake_num[0]; i+=2) back_sorted_index_mt_1[index_num ++] = i;
        std::sort(back_sorted_index_mt_1, back_sorted_index_mt_1 + index_num, [](int i, int j) {
            if (answer_fake_mt_1[0][i] != answer_fake_mt_1[0][j]) return answer_fake_mt_1[0][i] < answer_fake_mt_1[0][j];
            return answer_fake_mt_1[0][i+1] < answer_fake_mt_1[0][j+1];
        });
        for (int i=0; i<index_num; ++i) {
            index = back_sorted_index_mt_1[i];
            answer_mt_1[0][answer_num_mt_1[0] ++] = answer_fake_mt_1[0][index];
            answer_mt_1[0][answer_num_mt_1[0] ++] = answer_fake_mt_1[0][index+1];
        }
    }
    if (fake_num[1] > 0) {
        index_num = 0;
        for (int i=0; i<fake_num[1]; i+=3) back_sorted_index_mt_1[index_num ++] = i;
        std::sort(back_sorted_index_mt_1, back_sorted_index_mt_1 + index_num, [](int i, int j) {
            if (answer_fake_mt_1[1][i] != answer_fake_mt_1[1][j]) return answer_fake_mt_1[1][i] < answer_fake_mt_1[1][j];
            if (answer_fake_mt_1[1][i+1] != answer_fake_mt_1[1][j+1]) return answer_fake_mt_1[1][i+1] < answer_fake_mt_1[1][j+1];
            return answer_fake_mt_1[1][i+2] < answer_fake_mt_1[1][j+2];
        });
        for (int i=0; i<index_num; ++i) {
            index = back_sorted_index_mt_1[i];
            answer_mt_1[1][answer_num_mt_1[1] ++] = answer_fake_mt_1[1][index];
            answer_mt_1[1][answer_num_mt_1[1] ++] = answer_fake_mt_1[1][index+1];
            answer_mt_1[1][answer_num_mt_1[1] ++] = answer_fake_mt_1[1][index+2];
        }
    }
}

void do_front_search_mt_1(int starter) {
    int x, y, z, t, o, f;
    int u, v, w, e, index, ai;
    int back1, back2;

    visit_mt_1[starter] = mask_mt_1;
    refresh_starter_mt_1(starter, starter);
    for (int i=edge_topo_starter_mt_1[starter]; i<edge_topo_header[starter+1]; ++i) {
        u = edge_topo_edges[i]; x = edge_topo_values[i];
        visit_mt_1[u] = mask_mt_1;
        // printf("u: %d\n", u);
        refresh_starter_mt_1(u, starter);
        for (int j=edge_topo_starter_mt_1[u]; j<edge_topo_header[u+1]; ++j) {
            v = edge_topo_edges[j]; y = edge_topo_values[j];
            if (!check_x_y(x, y)) continue;
            visit_mt_1[v] = mask_mt_1;
            // printf("v: %d\n", v);
            // circle 5
            gain_sorted_back_mt_1(v);
            for (int r=back_sorted_from_mt_1[v]; r<back_sorted_to_mt_1[v]; ++r) {
                index = back_sorted_index_mt_1[r];
                o = back_value_back_mt_1[index];
                f = back_value_front_mt_1[index];
                back1 = back_1_mt_1[index];
                back2 = back_2_mt_1[index];
                if (check_x_y(f, x) && check_x_y(y, o) && visit_mt_1[back1] != mask_mt_1 && visit_mt_1[back2] != mask_mt_1) {
                    ai = answer_num_mt_1[2];
                    answer_mt_1[2][ai ++] = u;
                    answer_mt_1[2][ai ++] = v;
                    answer_mt_1[2][ai ++] = back2;
                    answer_mt_1[2][ai ++] = back1;
                    answer_num_mt_1[2] += 4;
                    total_answer_num_mt_1 ++;
                }
            }

            refresh_starter_mt_1(v, starter);
            for (int k=edge_topo_starter_mt_1[v]; k<edge_topo_header[v+1]; ++k) {
                w = edge_topo_edges[k]; z = edge_topo_values[k];
                if (!check_x_y(y, z)) continue;
                if (u == w) continue;
                visit_mt_1[w] = mask_mt_1;
                // printf("w: %d\n", w);
                // circle 6
                gain_sorted_back_mt_1(w);
                for (int r=back_sorted_from_mt_1[w]; r<back_sorted_to_mt_1[w]; ++r) {
                    index = back_sorted_index_mt_1[r];
                    o = back_value_back_mt_1[index];
                    f = back_value_front_mt_1[index];
                    back1 = back_1_mt_1[index];
                    back2 = back_2_mt_1[index];
                    if (check_x_y(f, x) && check_x_y(z, o) && visit_mt_1[back1] != mask_mt_1 && visit_mt_1[back2] != mask_mt_1) {
                        ai = answer_num_mt_1[3];
                        answer_mt_1[3][ai ++] = u;
                        answer_mt_1[3][ai ++] = v;
                        answer_mt_1[3][ai ++] = w;
                        answer_mt_1[3][ai ++] = back2;
                        answer_mt_1[3][ai ++] = back1;
                        answer_num_mt_1[3] += 5;
                        total_answer_num_mt_1 ++;
                    }
                }

                refresh_starter_mt_1(w, starter);
                for (int l=edge_topo_starter_mt_1[w]; l<edge_topo_header[w+1]; ++l) {
                    e = edge_topo_edges[l]; t = edge_topo_values[l];
                    if (!check_x_y(z, t)) continue;
                    if (e == v || e == u) continue;
                    visit_mt_1[e] = mask_mt_1;
                    // printf("e: %d\n", e);
                    // circle 7
                    gain_sorted_back_mt_1(e);
                    for (int r=back_sorted_from_mt_1[e]; r<back_sorted_to_mt_1[e]; ++r) {
                        index = back_sorted_index_mt_1[r];
                        o = back_value_back_mt_1[index];
                        f = back_value_front_mt_1[index];
                        back1 = back_1_mt_1[index];
                        back2 = back_2_mt_1[index];
                        if (check_x_y(f, x) && check_x_y(t, o) && visit_mt_1[back1] != mask_mt_1 && visit_mt_1[back2] != mask_mt_1) {
                            ai = answer_num_mt_1[4];
                            answer_mt_1[4][ai ++] = u;
                            answer_mt_1[4][ai ++] = v;
                            answer_mt_1[4][ai ++] = w;
                            answer_mt_1[4][ai ++] = e;
                            answer_mt_1[4][ai ++] = back2;
                            answer_mt_1[4][ai ++] = back1;
                            answer_num_mt_1[4] += 6;
                            total_answer_num_mt_1 ++;
                        }
                    }
                    visit_mt_1[e] = -1;
                }
                visit_mt_1[w] = -1;
            }
            visit_mt_1[v] = -1;
        }
        visit_mt_1[u] = -1;
    }
}

void do_search_mt_1(int starter) {
    mask_mt_1 ++;
    back_sorted_num_mt_1 = 0;
    do_back_search_mt_1(starter);
    if (back_num_mt_1 == 0) return;
    do_front_search_mt_1(starter);
}

void* search_mt_1(void* args) {
    malloc_mt_1();
    memcpy(edge_topo_starter_mt_1, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_counter, 1);
        if (u >= node_num) break;
        global_assign[u] = 1;
        for (int i=0; i<5; ++i) {
            answer_header[u][i] = answer_num_mt_1[i];
        }
        // printf("starter: %d\n", u);
        // printf("starter: %d %d %d %d\n", u, data_rev_mapping[u], answer_header[u][0], answer_header[u][1]);
        do_search_mt_1(u);
        // printf("\n");
        for (int i=0; i<5; ++i) {
            answer_tail[u][i] = answer_num_mt_1[i];
        }
        // printf("ender: %d %d %d\n", u, answer_tail[u][0], answer_tail[u][1]);
    }
    free_mt_1();
    return NULL;
}

// search mt_1

// search mt_2

int* answer_mt_2[5];
int* answer_fake_mt_2[2];
int answer_num_mt_2[5], total_answer_num_mt_2 = 0;

int* malloc_all_mt_2;

int* edge_topo_starter_mt_2;

int* back_1_mt_2, *back_2_mt_2, *back_value_back_mt_2, *back_value_front_mt_2;
int* back_header_mt_2, *back_nxt_mt_2;
int back_num_mt_2;

int mask_mt_2 = 123;
int* back_visit_mt_2;
int* back_sorted_visit_mt_2;
int* back_sorted_index_mt_2, *back_sorted_from_mt_2, *back_sorted_to_mt_2;
int back_sorted_num_mt_2;

int* front_visit_mt_2, *visit_mt_2;
int* front_value_mt_2;

void malloc_mt_2() {
    answer_fake_mt_2[0] = (int*) malloc(sizeof(int) * MAX_BACK * 2);
    answer_fake_mt_2[1] = (int*) malloc(sizeof(int) * MAX_BACK * 2);
    
    answer_mt_2[0] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_2[1] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_2[2] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_2[3] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_2[4] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);

    int total_size = node_num + MAX_BACK + MAX_BACK + MAX_BACK + MAX_BACK + node_num + 
        MAX_BACK + node_num + node_num + node_num + node_num + MAX_BACK
        + node_num + node_num + node_num;
    int size_now = 0;
    malloc_all_mt_2 = (int*) malloc(sizeof(int) * total_size);

    edge_topo_starter_mt_2 = malloc_all_mt_2 + size_now; size_now += node_num;

    back_1_mt_2 = malloc_all_mt_2 + size_now; size_now += MAX_BACK;
    back_2_mt_2 = malloc_all_mt_2 + size_now; size_now += MAX_BACK;
    back_value_back_mt_2 = malloc_all_mt_2 + size_now; size_now += MAX_BACK;
    back_value_front_mt_2 = malloc_all_mt_2 + size_now; size_now += MAX_BACK;
    back_header_mt_2 = malloc_all_mt_2 + size_now; size_now += node_num;
    back_nxt_mt_2 = malloc_all_mt_2 + size_now; size_now += MAX_BACK;

    back_visit_mt_2 = malloc_all_mt_2 + size_now; size_now += node_num;
    back_sorted_visit_mt_2 = malloc_all_mt_2 + size_now; size_now += node_num;
    back_sorted_index_mt_2 = malloc_all_mt_2 + size_now; size_now += MAX_BACK;
    back_sorted_from_mt_2 = malloc_all_mt_2 + size_now; size_now += node_num;
    back_sorted_to_mt_2 = malloc_all_mt_2 + size_now; size_now += node_num;

    front_visit_mt_2 = malloc_all_mt_2 + size_now; size_now += node_num;
    front_value_mt_2 = malloc_all_mt_2 + size_now; size_now += node_num;

    visit_mt_2 = malloc_all_mt_2 + size_now; size_now += node_num;

    memset(back_visit_mt_2, 0, sizeof(int) * node_num);
    memset(back_sorted_visit_mt_2, 0, sizeof(int) * node_num);
    memset(front_visit_mt_2, 0, sizeof(int) * node_num);
    memset(visit_mt_2, 0, sizeof(int) * node_num);
}

void free_mt_2() {
    free(malloc_all_mt_2);
    free(answer_fake_mt_2[0]);
    free(answer_fake_mt_2[1]);
}

inline void refresh_starter_mt_2(int u, int starter) {
    while (edge_topo_starter_mt_2[u] < edge_topo_header[u+1] 
        && edge_topo_edges[edge_topo_starter_mt_2[u]] <= starter)
        edge_topo_starter_mt_2[u] ++;
}

void gain_sorted_back_mt_2(int u) {
    if (back_sorted_visit_mt_2[u] == mask_mt_2) return;
    if (back_visit_mt_2[u] != mask_mt_2) {
        back_sorted_from_mt_2[u] = back_sorted_to_mt_2[u] = 0;
        return;
    }
    back_sorted_visit_mt_2[u] = mask_mt_2;
    back_sorted_from_mt_2[u] = back_sorted_num_mt_2;
    for (int i=back_header_mt_2[u]; i!=-1; i=back_nxt_mt_2[i]) {
        back_sorted_index_mt_2[back_sorted_num_mt_2 ++] = i;
    }
    back_sorted_to_mt_2[u] = back_sorted_num_mt_2;
    std::sort(back_sorted_index_mt_2 + back_sorted_from_mt_2[u], back_sorted_index_mt_2 + back_sorted_to_mt_2[u],
        [](int i, int j) {
            if (back_2_mt_2[i] != back_2_mt_2[j]) return back_2_mt_2[i] < back_2_mt_2[j];
            return back_1_mt_2[i] < back_1_mt_2[j];
        });
}

void do_back_search_mt_2(int starter) {
    back_num_mt_2 = 0;
    int fake_num[2]; fake_num[0] = fake_num[1] = 0;

    refresh_starter_mt_2(starter, starter);
    int u, v, w, x, y, z, o, index;
    for (int i=edge_topo_starter_mt_2[starter]; i<edge_topo_header[starter+1]; ++i) {
        u = edge_topo_edges[i]; x = edge_topo_values[i];
        front_visit_mt_2[u] = mask_mt_2;
        front_value_mt_2[u] = x;
    }

    for (int i=rev_edge_topo_header[starter]; i<rev_edge_topo_header[starter+1]; ++i) {
        u = rev_edge_topo_edges[i]; x = rev_edge_topo_values[i];
        if (u <= starter) break;
        // printf("u: %d\n", u);
        for (int j=rev_edge_topo_header[u]; j<rev_edge_topo_header[u+1]; ++j) {
            v = rev_edge_topo_edges[j]; y = rev_edge_topo_values[j];
            // printf("v0: %d\n", v);
            if (v <= starter) break;
            // printf("v1: %d\n", v);
            if (!check_x_y(y, x)) continue;
            // printf("v: %d\n", v);
            // circle 3
            if (front_visit_mt_2[v] == mask_mt_2) {
                z = front_value_mt_2[v];
                if (check_x_y(z, y) && check_x_y(x, z)) {
                    index = fake_num[0];
                    answer_fake_mt_2[0][index ++] = v;
                    answer_fake_mt_2[0][index ++] = u;
                    fake_num[0] += 2;
                    total_answer_num_mt_2 ++;
                    // printf("%d %d %d\n", data_rev_mapping[starter], data_rev_mapping[v], data_rev_mapping[u]);
                }
            }

            for (int k=rev_edge_topo_header[v]; k<rev_edge_topo_header[v+1]; ++k) {
                w = rev_edge_topo_edges[k]; z = rev_edge_topo_values[k];
                if (w <= starter) break;
                if (w == u) continue;
                if (!check_x_y(z, y)) continue;
                // printf("w: %d\n", w);
                // circle 4
                if (front_visit_mt_2[w] == mask_mt_2) {
                    o = front_value_mt_2[w];
                    if (check_x_y(x, o) && check_x_y(o, z)) {
                        index = fake_num[1];
                        answer_fake_mt_2[1][index ++] = w;
                        answer_fake_mt_2[1][index ++] = v;
                        answer_fake_mt_2[1][index ++] = u;
                        fake_num[1] += 3;
                        total_answer_num_mt_2 ++;
                        // printf("%d %d %d %d\n", data_rev_mapping[starter], data_rev_mapping[w], data_rev_mapping[v], data_rev_mapping[u]);
                    }
                }

                if (back_visit_mt_2[w] != mask_mt_2) {
                    back_visit_mt_2[w] = mask_mt_2;
                    back_header_mt_2[w] = -1;
                }
                index = back_num_mt_2 ++;
                back_value_front_mt_2[index] = x;
                back_value_back_mt_2[index] = z;
                back_1_mt_2[index] = u;
                back_2_mt_2[index] = v;
                back_nxt_mt_2[index] = back_header_mt_2[w];
                back_header_mt_2[w] = index;
            }
        }
    }

    int index_num;
    if (fake_num[0] > 0) {
        index_num = 0;
        for (int i=0; i<fake_num[0]; i+=2) back_sorted_index_mt_2[index_num ++] = i;
        std::sort(back_sorted_index_mt_2, back_sorted_index_mt_2 + index_num, [](int i, int j) {
            if (answer_fake_mt_2[0][i] != answer_fake_mt_2[0][j]) return answer_fake_mt_2[0][i] < answer_fake_mt_2[0][j];
            return answer_fake_mt_2[0][i+1] < answer_fake_mt_2[0][j+1];
        });
        for (int i=0; i<index_num; ++i) {
            index = back_sorted_index_mt_2[i];
            answer_mt_2[0][answer_num_mt_2[0] ++] = answer_fake_mt_2[0][index];
            answer_mt_2[0][answer_num_mt_2[0] ++] = answer_fake_mt_2[0][index+1];
        }
    }
    if (fake_num[1] > 0) {
        index_num = 0;
        for (int i=0; i<fake_num[1]; i+=3) back_sorted_index_mt_2[index_num ++] = i;
        std::sort(back_sorted_index_mt_2, back_sorted_index_mt_2 + index_num, [](int i, int j) {
            if (answer_fake_mt_2[1][i] != answer_fake_mt_2[1][j]) return answer_fake_mt_2[1][i] < answer_fake_mt_2[1][j];
            if (answer_fake_mt_2[1][i+1] != answer_fake_mt_2[1][j+1]) return answer_fake_mt_2[1][i+1] < answer_fake_mt_2[1][j+1];
            return answer_fake_mt_2[1][i+2] < answer_fake_mt_2[1][j+2];
        });
        for (int i=0; i<index_num; ++i) {
            index = back_sorted_index_mt_2[i];
            answer_mt_2[1][answer_num_mt_2[1] ++] = answer_fake_mt_2[1][index];
            answer_mt_2[1][answer_num_mt_2[1] ++] = answer_fake_mt_2[1][index+1];
            answer_mt_2[1][answer_num_mt_2[1] ++] = answer_fake_mt_2[1][index+2];
        }
    }
}

void do_front_search_mt_2(int starter) {
    int x, y, z, t, o, f;
    int u, v, w, e, index, ai;
    int back1, back2;

    visit_mt_2[starter] = mask_mt_2;
    refresh_starter_mt_2(starter, starter);
    for (int i=edge_topo_starter_mt_2[starter]; i<edge_topo_header[starter+1]; ++i) {
        u = edge_topo_edges[i]; x = edge_topo_values[i];
        visit_mt_2[u] = mask_mt_2;
        // printf("u: %d\n", u);
        refresh_starter_mt_2(u, starter);
        for (int j=edge_topo_starter_mt_2[u]; j<edge_topo_header[u+1]; ++j) {
            v = edge_topo_edges[j]; y = edge_topo_values[j];
            if (!check_x_y(x, y)) continue;
            visit_mt_2[v] = mask_mt_2;
            // printf("v: %d\n", v);
            // circle 5
            gain_sorted_back_mt_2(v);
            for (int r=back_sorted_from_mt_2[v]; r<back_sorted_to_mt_2[v]; ++r) {
                index = back_sorted_index_mt_2[r];
                o = back_value_back_mt_2[index];
                f = back_value_front_mt_2[index];
                back1 = back_1_mt_2[index];
                back2 = back_2_mt_2[index];
                if (check_x_y(f, x) && check_x_y(y, o) && visit_mt_2[back1] != mask_mt_2 && visit_mt_2[back2] != mask_mt_2) {
                    ai = answer_num_mt_2[2];
                    answer_mt_2[2][ai ++] = u;
                    answer_mt_2[2][ai ++] = v;
                    answer_mt_2[2][ai ++] = back2;
                    answer_mt_2[2][ai ++] = back1;
                    answer_num_mt_2[2] += 4;
                    total_answer_num_mt_2 ++;
                }
            }

            refresh_starter_mt_2(v, starter);
            for (int k=edge_topo_starter_mt_2[v]; k<edge_topo_header[v+1]; ++k) {
                w = edge_topo_edges[k]; z = edge_topo_values[k];
                if (!check_x_y(y, z)) continue;
                if (u == w) continue;
                visit_mt_2[w] = mask_mt_2;
                // printf("w: %d\n", w);
                // circle 6
                gain_sorted_back_mt_2(w);
                for (int r=back_sorted_from_mt_2[w]; r<back_sorted_to_mt_2[w]; ++r) {
                    index = back_sorted_index_mt_2[r];
                    o = back_value_back_mt_2[index];
                    f = back_value_front_mt_2[index];
                    back1 = back_1_mt_2[index];
                    back2 = back_2_mt_2[index];
                    if (check_x_y(f, x) && check_x_y(z, o) && visit_mt_2[back1] != mask_mt_2 && visit_mt_2[back2] != mask_mt_2) {
                        ai = answer_num_mt_2[3];
                        answer_mt_2[3][ai ++] = u;
                        answer_mt_2[3][ai ++] = v;
                        answer_mt_2[3][ai ++] = w;
                        answer_mt_2[3][ai ++] = back2;
                        answer_mt_2[3][ai ++] = back1;
                        answer_num_mt_2[3] += 5;
                        total_answer_num_mt_2 ++;
                    }
                }

                refresh_starter_mt_2(w, starter);
                for (int l=edge_topo_starter_mt_2[w]; l<edge_topo_header[w+1]; ++l) {
                    e = edge_topo_edges[l]; t = edge_topo_values[l];
                    if (!check_x_y(z, t)) continue;
                    if (e == v || e == u) continue;
                    visit_mt_2[e] = mask_mt_2;
                    // printf("e: %d\n", e);
                    // circle 7
                    gain_sorted_back_mt_2(e);
                    for (int r=back_sorted_from_mt_2[e]; r<back_sorted_to_mt_2[e]; ++r) {
                        index = back_sorted_index_mt_2[r];
                        o = back_value_back_mt_2[index];
                        f = back_value_front_mt_2[index];
                        back1 = back_1_mt_2[index];
                        back2 = back_2_mt_2[index];
                        if (check_x_y(f, x) && check_x_y(t, o) && visit_mt_2[back1] != mask_mt_2 && visit_mt_2[back2] != mask_mt_2) {
                            ai = answer_num_mt_2[4];
                            answer_mt_2[4][ai ++] = u;
                            answer_mt_2[4][ai ++] = v;
                            answer_mt_2[4][ai ++] = w;
                            answer_mt_2[4][ai ++] = e;
                            answer_mt_2[4][ai ++] = back2;
                            answer_mt_2[4][ai ++] = back1;
                            answer_num_mt_2[4] += 6;
                            total_answer_num_mt_2 ++;
                        }
                    }
                    visit_mt_2[e] = -1;
                }
                visit_mt_2[w] = -1;
            }
            visit_mt_2[v] = -1;
        }
        visit_mt_2[u] = -1;
    }
}

void do_search_mt_2(int starter) {
    mask_mt_2 ++;
    back_sorted_num_mt_2 = 0;
    do_back_search_mt_2(starter);
    if (back_num_mt_2 == 0) return;
    do_front_search_mt_2(starter);
}

void* search_mt_2(void* args) {
    malloc_mt_2();
    memcpy(edge_topo_starter_mt_2, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_counter, 1);
        if (u >= node_num) break;
        global_assign[u] = 2;
        for (int i=0; i<5; ++i) {
            answer_header[u][i] = answer_num_mt_2[i];
        }
        // printf("starter: %d\n", u);
        // printf("starter: %d %d %d %d\n", u, data_rev_mapping[u], answer_header[u][0], answer_header[u][1]);
        do_search_mt_2(u);
        // printf("\n");
        for (int i=0; i<5; ++i) {
            answer_tail[u][i] = answer_num_mt_2[i];
        }
        // printf("ender: %d %d %d\n", u, answer_tail[u][0], answer_tail[u][1]);
    }
    free_mt_2();
    return NULL;
}

// search mt_2

// search mt_3

int* answer_mt_3[5];
int* answer_fake_mt_3[2];
int answer_num_mt_3[5], total_answer_num_mt_3 = 0;

int* malloc_all_mt_3;

int* edge_topo_starter_mt_3;

int* back_1_mt_3, *back_2_mt_3, *back_value_back_mt_3, *back_value_front_mt_3;
int* back_header_mt_3, *back_nxt_mt_3;
int back_num_mt_3;

int mask_mt_3 = 123;
int* back_visit_mt_3;
int* back_sorted_visit_mt_3;
int* back_sorted_index_mt_3, *back_sorted_from_mt_3, *back_sorted_to_mt_3;
int back_sorted_num_mt_3;

int* front_visit_mt_3, *visit_mt_3;
int* front_value_mt_3;

void malloc_mt_3() {
    answer_fake_mt_3[0] = (int*) malloc(sizeof(int) * MAX_BACK * 2);
    answer_fake_mt_3[1] = (int*) malloc(sizeof(int) * MAX_BACK * 2);
    
    answer_mt_3[0] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_3[1] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_3[2] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_3[3] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_3[4] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);

    int total_size = node_num + MAX_BACK + MAX_BACK + MAX_BACK + MAX_BACK + node_num + 
        MAX_BACK + node_num + node_num + node_num + node_num + MAX_BACK
        + node_num + node_num + node_num;
    int size_now = 0;
    malloc_all_mt_3 = (int*) malloc(sizeof(int) * total_size);

    edge_topo_starter_mt_3 = malloc_all_mt_3 + size_now; size_now += node_num;

    back_1_mt_3 = malloc_all_mt_3 + size_now; size_now += MAX_BACK;
    back_2_mt_3 = malloc_all_mt_3 + size_now; size_now += MAX_BACK;
    back_value_back_mt_3 = malloc_all_mt_3 + size_now; size_now += MAX_BACK;
    back_value_front_mt_3 = malloc_all_mt_3 + size_now; size_now += MAX_BACK;
    back_header_mt_3 = malloc_all_mt_3 + size_now; size_now += node_num;
    back_nxt_mt_3 = malloc_all_mt_3 + size_now; size_now += MAX_BACK;

    back_visit_mt_3 = malloc_all_mt_3 + size_now; size_now += node_num;
    back_sorted_visit_mt_3 = malloc_all_mt_3 + size_now; size_now += node_num;
    back_sorted_index_mt_3 = malloc_all_mt_3 + size_now; size_now += MAX_BACK;
    back_sorted_from_mt_3 = malloc_all_mt_3 + size_now; size_now += node_num;
    back_sorted_to_mt_3 = malloc_all_mt_3 + size_now; size_now += node_num;

    front_visit_mt_3 = malloc_all_mt_3 + size_now; size_now += node_num;
    front_value_mt_3 = malloc_all_mt_3 + size_now; size_now += node_num;

    visit_mt_3 = malloc_all_mt_3 + size_now; size_now += node_num;

    memset(back_visit_mt_3, 0, sizeof(int) * node_num);
    memset(back_sorted_visit_mt_3, 0, sizeof(int) * node_num);
    memset(front_visit_mt_3, 0, sizeof(int) * node_num);
    memset(visit_mt_3, 0, sizeof(int) * node_num);
}

void free_mt_3() {
    free(malloc_all_mt_3);
    free(answer_fake_mt_3[0]);
    free(answer_fake_mt_3[1]);
}

inline void refresh_starter_mt_3(int u, int starter) {
    while (edge_topo_starter_mt_3[u] < edge_topo_header[u+1] 
        && edge_topo_edges[edge_topo_starter_mt_3[u]] <= starter)
        edge_topo_starter_mt_3[u] ++;
}

void gain_sorted_back_mt_3(int u) {
    if (back_sorted_visit_mt_3[u] == mask_mt_3) return;
    if (back_visit_mt_3[u] != mask_mt_3) {
        back_sorted_from_mt_3[u] = back_sorted_to_mt_3[u] = 0;
        return;
    }
    back_sorted_visit_mt_3[u] = mask_mt_3;
    back_sorted_from_mt_3[u] = back_sorted_num_mt_3;
    for (int i=back_header_mt_3[u]; i!=-1; i=back_nxt_mt_3[i]) {
        back_sorted_index_mt_3[back_sorted_num_mt_3 ++] = i;
    }
    back_sorted_to_mt_3[u] = back_sorted_num_mt_3;
    std::sort(back_sorted_index_mt_3 + back_sorted_from_mt_3[u], back_sorted_index_mt_3 + back_sorted_to_mt_3[u],
        [](int i, int j) {
            if (back_2_mt_3[i] != back_2_mt_3[j]) return back_2_mt_3[i] < back_2_mt_3[j];
            return back_1_mt_3[i] < back_1_mt_3[j];
        });
}

void do_back_search_mt_3(int starter) {
    back_num_mt_3 = 0;
    int fake_num[2]; fake_num[0] = fake_num[1] = 0;

    refresh_starter_mt_3(starter, starter);
    int u, v, w, x, y, z, o, index;
    for (int i=edge_topo_starter_mt_3[starter]; i<edge_topo_header[starter+1]; ++i) {
        u = edge_topo_edges[i]; x = edge_topo_values[i];
        front_visit_mt_3[u] = mask_mt_3;
        front_value_mt_3[u] = x;
    }

    for (int i=rev_edge_topo_header[starter]; i<rev_edge_topo_header[starter+1]; ++i) {
        u = rev_edge_topo_edges[i]; x = rev_edge_topo_values[i];
        if (u <= starter) break;
        // printf("u: %d\n", u);
        for (int j=rev_edge_topo_header[u]; j<rev_edge_topo_header[u+1]; ++j) {
            v = rev_edge_topo_edges[j]; y = rev_edge_topo_values[j];
            // printf("v0: %d\n", v);
            if (v <= starter) break;
            // printf("v1: %d\n", v);
            if (!check_x_y(y, x)) continue;
            // printf("v: %d\n", v);
            // circle 3
            if (front_visit_mt_3[v] == mask_mt_3) {
                z = front_value_mt_3[v];
                if (check_x_y(z, y) && check_x_y(x, z)) {
                    index = fake_num[0];
                    answer_fake_mt_3[0][index ++] = v;
                    answer_fake_mt_3[0][index ++] = u;
                    fake_num[0] += 2;
                    total_answer_num_mt_3 ++;
                    // printf("%d %d %d\n", data_rev_mapping[starter], data_rev_mapping[v], data_rev_mapping[u]);
                }
            }

            for (int k=rev_edge_topo_header[v]; k<rev_edge_topo_header[v+1]; ++k) {
                w = rev_edge_topo_edges[k]; z = rev_edge_topo_values[k];
                if (w <= starter) break;
                if (w == u) continue;
                if (!check_x_y(z, y)) continue;
                // printf("w: %d\n", w);
                // circle 4
                if (front_visit_mt_3[w] == mask_mt_3) {
                    o = front_value_mt_3[w];
                    if (check_x_y(x, o) && check_x_y(o, z)) {
                        index = fake_num[1];
                        answer_fake_mt_3[1][index ++] = w;
                        answer_fake_mt_3[1][index ++] = v;
                        answer_fake_mt_3[1][index ++] = u;
                        fake_num[1] += 3;
                        total_answer_num_mt_3 ++;
                        // printf("%d %d %d %d\n", data_rev_mapping[starter], data_rev_mapping[w], data_rev_mapping[v], data_rev_mapping[u]);
                    }
                }

                if (back_visit_mt_3[w] != mask_mt_3) {
                    back_visit_mt_3[w] = mask_mt_3;
                    back_header_mt_3[w] = -1;
                }
                index = back_num_mt_3 ++;
                back_value_front_mt_3[index] = x;
                back_value_back_mt_3[index] = z;
                back_1_mt_3[index] = u;
                back_2_mt_3[index] = v;
                back_nxt_mt_3[index] = back_header_mt_3[w];
                back_header_mt_3[w] = index;
            }
        }
    }

    int index_num;
    if (fake_num[0] > 0) {
        index_num = 0;
        for (int i=0; i<fake_num[0]; i+=2) back_sorted_index_mt_3[index_num ++] = i;
        std::sort(back_sorted_index_mt_3, back_sorted_index_mt_3 + index_num, [](int i, int j) {
            if (answer_fake_mt_3[0][i] != answer_fake_mt_3[0][j]) return answer_fake_mt_3[0][i] < answer_fake_mt_3[0][j];
            return answer_fake_mt_3[0][i+1] < answer_fake_mt_3[0][j+1];
        });
        for (int i=0; i<index_num; ++i) {
            index = back_sorted_index_mt_3[i];
            answer_mt_3[0][answer_num_mt_3[0] ++] = answer_fake_mt_3[0][index];
            answer_mt_3[0][answer_num_mt_3[0] ++] = answer_fake_mt_3[0][index+1];
        }
    }
    if (fake_num[1] > 0) {
        index_num = 0;
        for (int i=0; i<fake_num[1]; i+=3) back_sorted_index_mt_3[index_num ++] = i;
        std::sort(back_sorted_index_mt_3, back_sorted_index_mt_3 + index_num, [](int i, int j) {
            if (answer_fake_mt_3[1][i] != answer_fake_mt_3[1][j]) return answer_fake_mt_3[1][i] < answer_fake_mt_3[1][j];
            if (answer_fake_mt_3[1][i+1] != answer_fake_mt_3[1][j+1]) return answer_fake_mt_3[1][i+1] < answer_fake_mt_3[1][j+1];
            return answer_fake_mt_3[1][i+2] < answer_fake_mt_3[1][j+2];
        });
        for (int i=0; i<index_num; ++i) {
            index = back_sorted_index_mt_3[i];
            answer_mt_3[1][answer_num_mt_3[1] ++] = answer_fake_mt_3[1][index];
            answer_mt_3[1][answer_num_mt_3[1] ++] = answer_fake_mt_3[1][index+1];
            answer_mt_3[1][answer_num_mt_3[1] ++] = answer_fake_mt_3[1][index+2];
        }
    }
}

void do_front_search_mt_3(int starter) {
    int x, y, z, t, o, f;
    int u, v, w, e, index, ai;
    int back1, back2;

    visit_mt_3[starter] = mask_mt_3;
    refresh_starter_mt_3(starter, starter);
    for (int i=edge_topo_starter_mt_3[starter]; i<edge_topo_header[starter+1]; ++i) {
        u = edge_topo_edges[i]; x = edge_topo_values[i];
        visit_mt_3[u] = mask_mt_3;
        // printf("u: %d\n", u);
        refresh_starter_mt_3(u, starter);
        for (int j=edge_topo_starter_mt_3[u]; j<edge_topo_header[u+1]; ++j) {
            v = edge_topo_edges[j]; y = edge_topo_values[j];
            if (!check_x_y(x, y)) continue;
            visit_mt_3[v] = mask_mt_3;
            // printf("v: %d\n", v);
            // circle 5
            gain_sorted_back_mt_3(v);
            for (int r=back_sorted_from_mt_3[v]; r<back_sorted_to_mt_3[v]; ++r) {
                index = back_sorted_index_mt_3[r];
                o = back_value_back_mt_3[index];
                f = back_value_front_mt_3[index];
                back1 = back_1_mt_3[index];
                back2 = back_2_mt_3[index];
                if (check_x_y(f, x) && check_x_y(y, o) && visit_mt_3[back1] != mask_mt_3 && visit_mt_3[back2] != mask_mt_3) {
                    ai = answer_num_mt_3[2];
                    answer_mt_3[2][ai ++] = u;
                    answer_mt_3[2][ai ++] = v;
                    answer_mt_3[2][ai ++] = back2;
                    answer_mt_3[2][ai ++] = back1;
                    answer_num_mt_3[2] += 4;
                    total_answer_num_mt_3 ++;
                }
            }

            refresh_starter_mt_3(v, starter);
            for (int k=edge_topo_starter_mt_3[v]; k<edge_topo_header[v+1]; ++k) {
                w = edge_topo_edges[k]; z = edge_topo_values[k];
                if (!check_x_y(y, z)) continue;
                if (u == w) continue;
                visit_mt_3[w] = mask_mt_3;
                // printf("w: %d\n", w);
                // circle 6
                gain_sorted_back_mt_3(w);
                for (int r=back_sorted_from_mt_3[w]; r<back_sorted_to_mt_3[w]; ++r) {
                    index = back_sorted_index_mt_3[r];
                    o = back_value_back_mt_3[index];
                    f = back_value_front_mt_3[index];
                    back1 = back_1_mt_3[index];
                    back2 = back_2_mt_3[index];
                    if (check_x_y(f, x) && check_x_y(z, o) && visit_mt_3[back1] != mask_mt_3 && visit_mt_3[back2] != mask_mt_3) {
                        ai = answer_num_mt_3[3];
                        answer_mt_3[3][ai ++] = u;
                        answer_mt_3[3][ai ++] = v;
                        answer_mt_3[3][ai ++] = w;
                        answer_mt_3[3][ai ++] = back2;
                        answer_mt_3[3][ai ++] = back1;
                        answer_num_mt_3[3] += 5;
                        total_answer_num_mt_3 ++;
                    }
                }

                refresh_starter_mt_3(w, starter);
                for (int l=edge_topo_starter_mt_3[w]; l<edge_topo_header[w+1]; ++l) {
                    e = edge_topo_edges[l]; t = edge_topo_values[l];
                    if (!check_x_y(z, t)) continue;
                    if (e == v || e == u) continue;
                    visit_mt_3[e] = mask_mt_3;
                    // printf("e: %d\n", e);
                    // circle 7
                    gain_sorted_back_mt_3(e);
                    for (int r=back_sorted_from_mt_3[e]; r<back_sorted_to_mt_3[e]; ++r) {
                        index = back_sorted_index_mt_3[r];
                        o = back_value_back_mt_3[index];
                        f = back_value_front_mt_3[index];
                        back1 = back_1_mt_3[index];
                        back2 = back_2_mt_3[index];
                        if (check_x_y(f, x) && check_x_y(t, o) && visit_mt_3[back1] != mask_mt_3 && visit_mt_3[back2] != mask_mt_3) {
                            ai = answer_num_mt_3[4];
                            answer_mt_3[4][ai ++] = u;
                            answer_mt_3[4][ai ++] = v;
                            answer_mt_3[4][ai ++] = w;
                            answer_mt_3[4][ai ++] = e;
                            answer_mt_3[4][ai ++] = back2;
                            answer_mt_3[4][ai ++] = back1;
                            answer_num_mt_3[4] += 6;
                            total_answer_num_mt_3 ++;
                        }
                    }
                    visit_mt_3[e] = -1;
                }
                visit_mt_3[w] = -1;
            }
            visit_mt_3[v] = -1;
        }
        visit_mt_3[u] = -1;
    }
}

void do_search_mt_3(int starter) {
    mask_mt_3 ++;
    back_sorted_num_mt_3 = 0;
    do_back_search_mt_3(starter);
    if (back_num_mt_3 == 0) return;
    do_front_search_mt_3(starter);
}

void* search_mt_3(void* args) {
    malloc_mt_3();
    memcpy(edge_topo_starter_mt_3, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_counter, 1);
        if (u >= node_num) break;
        global_assign[u] = 3;
        for (int i=0; i<5; ++i) {
            answer_header[u][i] = answer_num_mt_3[i];
        }
        // printf("starter: %d\n", u);
        // printf("starter: %d %d %d %d\n", u, data_rev_mapping[u], answer_header[u][0], answer_header[u][1]);
        do_search_mt_3(u);
        // printf("\n");
        for (int i=0; i<5; ++i) {
            answer_tail[u][i] = answer_num_mt_3[i];
        }
        // printf("ender: %d %d %d\n", u, answer_tail[u][0], answer_tail[u][1]);
    }
    free_mt_3();
    return NULL;
}

// search mt_3

// search mt_4

int* answer_mt_4[5];
int* answer_fake_mt_4[2];
int answer_num_mt_4[5], total_answer_num_mt_4 = 0;

int* malloc_all_mt_4;

int* edge_topo_starter_mt_4;

int* back_1_mt_4, *back_2_mt_4, *back_value_back_mt_4, *back_value_front_mt_4;
int* back_header_mt_4, *back_nxt_mt_4;
int back_num_mt_4;

int mask_mt_4 = 123;
int* back_visit_mt_4;
int* back_sorted_visit_mt_4;
int* back_sorted_index_mt_4, *back_sorted_from_mt_4, *back_sorted_to_mt_4;
int back_sorted_num_mt_4;

int* front_visit_mt_4, *visit_mt_4;
int* front_value_mt_4;

void malloc_mt_4() {
    answer_fake_mt_4[0] = (int*) malloc(sizeof(int) * MAX_BACK * 2);
    answer_fake_mt_4[1] = (int*) malloc(sizeof(int) * MAX_BACK * 2);
    
    answer_mt_4[0] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_4[1] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_4[2] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_4[3] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_4[4] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);

    int total_size = node_num + MAX_BACK + MAX_BACK + MAX_BACK + MAX_BACK + node_num + 
        MAX_BACK + node_num + node_num + node_num + node_num + MAX_BACK
        + node_num + node_num + node_num;
    int size_now = 0;
    malloc_all_mt_4 = (int*) malloc(sizeof(int) * total_size);

    edge_topo_starter_mt_4 = malloc_all_mt_4 + size_now; size_now += node_num;

    back_1_mt_4 = malloc_all_mt_4 + size_now; size_now += MAX_BACK;
    back_2_mt_4 = malloc_all_mt_4 + size_now; size_now += MAX_BACK;
    back_value_back_mt_4 = malloc_all_mt_4 + size_now; size_now += MAX_BACK;
    back_value_front_mt_4 = malloc_all_mt_4 + size_now; size_now += MAX_BACK;
    back_header_mt_4 = malloc_all_mt_4 + size_now; size_now += node_num;
    back_nxt_mt_4 = malloc_all_mt_4 + size_now; size_now += MAX_BACK;

    back_visit_mt_4 = malloc_all_mt_4 + size_now; size_now += node_num;
    back_sorted_visit_mt_4 = malloc_all_mt_4 + size_now; size_now += node_num;
    back_sorted_index_mt_4 = malloc_all_mt_4 + size_now; size_now += MAX_BACK;
    back_sorted_from_mt_4 = malloc_all_mt_4 + size_now; size_now += node_num;
    back_sorted_to_mt_4 = malloc_all_mt_4 + size_now; size_now += node_num;

    front_visit_mt_4 = malloc_all_mt_4 + size_now; size_now += node_num;
    front_value_mt_4 = malloc_all_mt_4 + size_now; size_now += node_num;

    visit_mt_4 = malloc_all_mt_4 + size_now; size_now += node_num;

    memset(back_visit_mt_4, 0, sizeof(int) * node_num);
    memset(back_sorted_visit_mt_4, 0, sizeof(int) * node_num);
    memset(front_visit_mt_4, 0, sizeof(int) * node_num);
    memset(visit_mt_4, 0, sizeof(int) * node_num);
}

void free_mt_4() {
    free(malloc_all_mt_4);
    free(answer_fake_mt_4[0]);
    free(answer_fake_mt_4[1]);
}

inline void refresh_starter_mt_4(int u, int starter) {
    while (edge_topo_starter_mt_4[u] < edge_topo_header[u+1] 
        && edge_topo_edges[edge_topo_starter_mt_4[u]] <= starter)
        edge_topo_starter_mt_4[u] ++;
}

void gain_sorted_back_mt_4(int u) {
    if (back_sorted_visit_mt_4[u] == mask_mt_4) return;
    if (back_visit_mt_4[u] != mask_mt_4) {
        back_sorted_from_mt_4[u] = back_sorted_to_mt_4[u] = 0;
        return;
    }
    back_sorted_visit_mt_4[u] = mask_mt_4;
    back_sorted_from_mt_4[u] = back_sorted_num_mt_4;
    for (int i=back_header_mt_4[u]; i!=-1; i=back_nxt_mt_4[i]) {
        back_sorted_index_mt_4[back_sorted_num_mt_4 ++] = i;
    }
    back_sorted_to_mt_4[u] = back_sorted_num_mt_4;
    std::sort(back_sorted_index_mt_4 + back_sorted_from_mt_4[u], back_sorted_index_mt_4 + back_sorted_to_mt_4[u],
        [](int i, int j) {
            if (back_2_mt_4[i] != back_2_mt_4[j]) return back_2_mt_4[i] < back_2_mt_4[j];
            return back_1_mt_4[i] < back_1_mt_4[j];
        });
}

void do_back_search_mt_4(int starter) {
    back_num_mt_4 = 0;
    int fake_num[2]; fake_num[0] = fake_num[1] = 0;

    refresh_starter_mt_4(starter, starter);
    int u, v, w, x, y, z, o, index;
    for (int i=edge_topo_starter_mt_4[starter]; i<edge_topo_header[starter+1]; ++i) {
        u = edge_topo_edges[i]; x = edge_topo_values[i];
        front_visit_mt_4[u] = mask_mt_4;
        front_value_mt_4[u] = x;
    }

    for (int i=rev_edge_topo_header[starter]; i<rev_edge_topo_header[starter+1]; ++i) {
        u = rev_edge_topo_edges[i]; x = rev_edge_topo_values[i];
        if (u <= starter) break;
        // printf("u: %d\n", u);
        for (int j=rev_edge_topo_header[u]; j<rev_edge_topo_header[u+1]; ++j) {
            v = rev_edge_topo_edges[j]; y = rev_edge_topo_values[j];
            // printf("v0: %d\n", v);
            if (v <= starter) break;
            // printf("v1: %d\n", v);
            if (!check_x_y(y, x)) continue;
            // printf("v: %d\n", v);
            // circle 3
            if (front_visit_mt_4[v] == mask_mt_4) {
                z = front_value_mt_4[v];
                if (check_x_y(z, y) && check_x_y(x, z)) {
                    index = fake_num[0];
                    answer_fake_mt_4[0][index ++] = v;
                    answer_fake_mt_4[0][index ++] = u;
                    fake_num[0] += 2;
                    total_answer_num_mt_4 ++;
                    // printf("%d %d %d\n", data_rev_mapping[starter], data_rev_mapping[v], data_rev_mapping[u]);
                }
            }

            for (int k=rev_edge_topo_header[v]; k<rev_edge_topo_header[v+1]; ++k) {
                w = rev_edge_topo_edges[k]; z = rev_edge_topo_values[k];
                if (w <= starter) break;
                if (w == u) continue;
                if (!check_x_y(z, y)) continue;
                // printf("w: %d\n", w);
                // circle 4
                if (front_visit_mt_4[w] == mask_mt_4) {
                    o = front_value_mt_4[w];
                    if (check_x_y(x, o) && check_x_y(o, z)) {
                        index = fake_num[1];
                        answer_fake_mt_4[1][index ++] = w;
                        answer_fake_mt_4[1][index ++] = v;
                        answer_fake_mt_4[1][index ++] = u;
                        fake_num[1] += 3;
                        total_answer_num_mt_4 ++;
                        // printf("%d %d %d %d\n", data_rev_mapping[starter], data_rev_mapping[w], data_rev_mapping[v], data_rev_mapping[u]);
                    }
                }

                if (back_visit_mt_4[w] != mask_mt_4) {
                    back_visit_mt_4[w] = mask_mt_4;
                    back_header_mt_4[w] = -1;
                }
                index = back_num_mt_4 ++;
                back_value_front_mt_4[index] = x;
                back_value_back_mt_4[index] = z;
                back_1_mt_4[index] = u;
                back_2_mt_4[index] = v;
                back_nxt_mt_4[index] = back_header_mt_4[w];
                back_header_mt_4[w] = index;
            }
        }
    }

    int index_num;
    if (fake_num[0] > 0) {
        index_num = 0;
        for (int i=0; i<fake_num[0]; i+=2) back_sorted_index_mt_4[index_num ++] = i;
        std::sort(back_sorted_index_mt_4, back_sorted_index_mt_4 + index_num, [](int i, int j) {
            if (answer_fake_mt_4[0][i] != answer_fake_mt_4[0][j]) return answer_fake_mt_4[0][i] < answer_fake_mt_4[0][j];
            return answer_fake_mt_4[0][i+1] < answer_fake_mt_4[0][j+1];
        });
        for (int i=0; i<index_num; ++i) {
            index = back_sorted_index_mt_4[i];
            answer_mt_4[0][answer_num_mt_4[0] ++] = answer_fake_mt_4[0][index];
            answer_mt_4[0][answer_num_mt_4[0] ++] = answer_fake_mt_4[0][index+1];
        }
    }
    if (fake_num[1] > 0) {
        index_num = 0;
        for (int i=0; i<fake_num[1]; i+=3) back_sorted_index_mt_4[index_num ++] = i;
        std::sort(back_sorted_index_mt_4, back_sorted_index_mt_4 + index_num, [](int i, int j) {
            if (answer_fake_mt_4[1][i] != answer_fake_mt_4[1][j]) return answer_fake_mt_4[1][i] < answer_fake_mt_4[1][j];
            if (answer_fake_mt_4[1][i+1] != answer_fake_mt_4[1][j+1]) return answer_fake_mt_4[1][i+1] < answer_fake_mt_4[1][j+1];
            return answer_fake_mt_4[1][i+2] < answer_fake_mt_4[1][j+2];
        });
        for (int i=0; i<index_num; ++i) {
            index = back_sorted_index_mt_4[i];
            answer_mt_4[1][answer_num_mt_4[1] ++] = answer_fake_mt_4[1][index];
            answer_mt_4[1][answer_num_mt_4[1] ++] = answer_fake_mt_4[1][index+1];
            answer_mt_4[1][answer_num_mt_4[1] ++] = answer_fake_mt_4[1][index+2];
        }
    }
}

void do_front_search_mt_4(int starter) {
    int x, y, z, t, o, f;
    int u, v, w, e, index, ai;
    int back1, back2;

    visit_mt_4[starter] = mask_mt_4;
    refresh_starter_mt_4(starter, starter);
    for (int i=edge_topo_starter_mt_4[starter]; i<edge_topo_header[starter+1]; ++i) {
        u = edge_topo_edges[i]; x = edge_topo_values[i];
        visit_mt_4[u] = mask_mt_4;
        // printf("u: %d\n", u);
        refresh_starter_mt_4(u, starter);
        for (int j=edge_topo_starter_mt_4[u]; j<edge_topo_header[u+1]; ++j) {
            v = edge_topo_edges[j]; y = edge_topo_values[j];
            if (!check_x_y(x, y)) continue;
            visit_mt_4[v] = mask_mt_4;
            // printf("v: %d\n", v);
            // circle 5
            gain_sorted_back_mt_4(v);
            for (int r=back_sorted_from_mt_4[v]; r<back_sorted_to_mt_4[v]; ++r) {
                index = back_sorted_index_mt_4[r];
                o = back_value_back_mt_4[index];
                f = back_value_front_mt_4[index];
                back1 = back_1_mt_4[index];
                back2 = back_2_mt_4[index];
                if (check_x_y(f, x) && check_x_y(y, o) && visit_mt_4[back1] != mask_mt_4 && visit_mt_4[back2] != mask_mt_4) {
                    ai = answer_num_mt_4[2];
                    answer_mt_4[2][ai ++] = u;
                    answer_mt_4[2][ai ++] = v;
                    answer_mt_4[2][ai ++] = back2;
                    answer_mt_4[2][ai ++] = back1;
                    answer_num_mt_4[2] += 4;
                    total_answer_num_mt_4 ++;
                }
            }

            refresh_starter_mt_4(v, starter);
            for (int k=edge_topo_starter_mt_4[v]; k<edge_topo_header[v+1]; ++k) {
                w = edge_topo_edges[k]; z = edge_topo_values[k];
                if (!check_x_y(y, z)) continue;
                if (u == w) continue;
                visit_mt_4[w] = mask_mt_4;
                // printf("w: %d\n", w);
                // circle 6
                gain_sorted_back_mt_4(w);
                for (int r=back_sorted_from_mt_4[w]; r<back_sorted_to_mt_4[w]; ++r) {
                    index = back_sorted_index_mt_4[r];
                    o = back_value_back_mt_4[index];
                    f = back_value_front_mt_4[index];
                    back1 = back_1_mt_4[index];
                    back2 = back_2_mt_4[index];
                    if (check_x_y(f, x) && check_x_y(z, o) && visit_mt_4[back1] != mask_mt_4 && visit_mt_4[back2] != mask_mt_4) {
                        ai = answer_num_mt_4[3];
                        answer_mt_4[3][ai ++] = u;
                        answer_mt_4[3][ai ++] = v;
                        answer_mt_4[3][ai ++] = w;
                        answer_mt_4[3][ai ++] = back2;
                        answer_mt_4[3][ai ++] = back1;
                        answer_num_mt_4[3] += 5;
                        total_answer_num_mt_4 ++;
                    }
                }

                refresh_starter_mt_4(w, starter);
                for (int l=edge_topo_starter_mt_4[w]; l<edge_topo_header[w+1]; ++l) {
                    e = edge_topo_edges[l]; t = edge_topo_values[l];
                    if (!check_x_y(z, t)) continue;
                    if (e == v || e == u) continue;
                    visit_mt_4[e] = mask_mt_4;
                    // printf("e: %d\n", e);
                    // circle 7
                    gain_sorted_back_mt_4(e);
                    for (int r=back_sorted_from_mt_4[e]; r<back_sorted_to_mt_4[e]; ++r) {
                        index = back_sorted_index_mt_4[r];
                        o = back_value_back_mt_4[index];
                        f = back_value_front_mt_4[index];
                        back1 = back_1_mt_4[index];
                        back2 = back_2_mt_4[index];
                        if (check_x_y(f, x) && check_x_y(t, o) && visit_mt_4[back1] != mask_mt_4 && visit_mt_4[back2] != mask_mt_4) {
                            ai = answer_num_mt_4[4];
                            answer_mt_4[4][ai ++] = u;
                            answer_mt_4[4][ai ++] = v;
                            answer_mt_4[4][ai ++] = w;
                            answer_mt_4[4][ai ++] = e;
                            answer_mt_4[4][ai ++] = back2;
                            answer_mt_4[4][ai ++] = back1;
                            answer_num_mt_4[4] += 6;
                            total_answer_num_mt_4 ++;
                        }
                    }
                    visit_mt_4[e] = -1;
                }
                visit_mt_4[w] = -1;
            }
            visit_mt_4[v] = -1;
        }
        visit_mt_4[u] = -1;
    }
}

void do_search_mt_4(int starter) {
    mask_mt_4 ++;
    back_sorted_num_mt_4 = 0;
    do_back_search_mt_4(starter);
    if (back_num_mt_4 == 0) return;
    do_front_search_mt_4(starter);
}

void* search_mt_4(void* args) {
    malloc_mt_4();
    memcpy(edge_topo_starter_mt_4, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_counter, 1);
        if (u >= node_num) break;
        global_assign[u] = 4;
        for (int i=0; i<5; ++i) {
            answer_header[u][i] = answer_num_mt_4[i];
        }
        // printf("starter: %d\n", u);
        // printf("starter: %d %d %d %d\n", u, data_rev_mapping[u], answer_header[u][0], answer_header[u][1]);
        do_search_mt_4(u);
        // printf("\n");
        for (int i=0; i<5; ++i) {
            answer_tail[u][i] = answer_num_mt_4[i];
        }
        // printf("ender: %d %d %d\n", u, answer_tail[u][0], answer_tail[u][1]);
    }
    free_mt_4();
    return NULL;
}

// search mt_4

// search mt_5

int* answer_mt_5[5];
int* answer_fake_mt_5[2];
int answer_num_mt_5[5], total_answer_num_mt_5 = 0;

int* malloc_all_mt_5;

int* edge_topo_starter_mt_5;

int* back_1_mt_5, *back_2_mt_5, *back_value_back_mt_5, *back_value_front_mt_5;
int* back_header_mt_5, *back_nxt_mt_5;
int back_num_mt_5;

int mask_mt_5 = 123;
int* back_visit_mt_5;
int* back_sorted_visit_mt_5;
int* back_sorted_index_mt_5, *back_sorted_from_mt_5, *back_sorted_to_mt_5;
int back_sorted_num_mt_5;

int* front_visit_mt_5, *visit_mt_5;
int* front_value_mt_5;

void malloc_mt_5() {
    answer_fake_mt_5[0] = (int*) malloc(sizeof(int) * MAX_BACK * 2);
    answer_fake_mt_5[1] = (int*) malloc(sizeof(int) * MAX_BACK * 2);
    
    answer_mt_5[0] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_5[1] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_5[2] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_5[3] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);
    answer_mt_5[4] = (int*) malloc(sizeof(int) * MAX_ANSW * 2);

    int total_size = node_num + MAX_BACK + MAX_BACK + MAX_BACK + MAX_BACK + node_num + 
        MAX_BACK + node_num + node_num + node_num + node_num + MAX_BACK
        + node_num + node_num + node_num;
    int size_now = 0;
    malloc_all_mt_5 = (int*) malloc(sizeof(int) * total_size);

    edge_topo_starter_mt_5 = malloc_all_mt_5 + size_now; size_now += node_num;

    back_1_mt_5 = malloc_all_mt_5 + size_now; size_now += MAX_BACK;
    back_2_mt_5 = malloc_all_mt_5 + size_now; size_now += MAX_BACK;
    back_value_back_mt_5 = malloc_all_mt_5 + size_now; size_now += MAX_BACK;
    back_value_front_mt_5 = malloc_all_mt_5 + size_now; size_now += MAX_BACK;
    back_header_mt_5 = malloc_all_mt_5 + size_now; size_now += node_num;
    back_nxt_mt_5 = malloc_all_mt_5 + size_now; size_now += MAX_BACK;

    back_visit_mt_5 = malloc_all_mt_5 + size_now; size_now += node_num;
    back_sorted_visit_mt_5 = malloc_all_mt_5 + size_now; size_now += node_num;
    back_sorted_index_mt_5 = malloc_all_mt_5 + size_now; size_now += MAX_BACK;
    back_sorted_from_mt_5 = malloc_all_mt_5 + size_now; size_now += node_num;
    back_sorted_to_mt_5 = malloc_all_mt_5 + size_now; size_now += node_num;

    front_visit_mt_5 = malloc_all_mt_5 + size_now; size_now += node_num;
    front_value_mt_5 = malloc_all_mt_5 + size_now; size_now += node_num;

    visit_mt_5 = malloc_all_mt_5 + size_now; size_now += node_num;

    memset(back_visit_mt_5, 0, sizeof(int) * node_num);
    memset(back_sorted_visit_mt_5, 0, sizeof(int) * node_num);
    memset(front_visit_mt_5, 0, sizeof(int) * node_num);
    memset(visit_mt_5, 0, sizeof(int) * node_num);
}

void free_mt_5() {
    free(malloc_all_mt_5);
    free(answer_fake_mt_5[0]);
    free(answer_fake_mt_5[1]);
}

inline void refresh_starter_mt_5(int u, int starter) {
    while (edge_topo_starter_mt_5[u] < edge_topo_header[u+1] 
        && edge_topo_edges[edge_topo_starter_mt_5[u]] <= starter)
        edge_topo_starter_mt_5[u] ++;
}

void gain_sorted_back_mt_5(int u) {
    if (back_sorted_visit_mt_5[u] == mask_mt_5) return;
    if (back_visit_mt_5[u] != mask_mt_5) {
        back_sorted_from_mt_5[u] = back_sorted_to_mt_5[u] = 0;
        return;
    }
    back_sorted_visit_mt_5[u] = mask_mt_5;
    back_sorted_from_mt_5[u] = back_sorted_num_mt_5;
    for (int i=back_header_mt_5[u]; i!=-1; i=back_nxt_mt_5[i]) {
        back_sorted_index_mt_5[back_sorted_num_mt_5 ++] = i;
    }
    back_sorted_to_mt_5[u] = back_sorted_num_mt_5;
    std::sort(back_sorted_index_mt_5 + back_sorted_from_mt_5[u], back_sorted_index_mt_5 + back_sorted_to_mt_5[u],
        [](int i, int j) {
            if (back_2_mt_5[i] != back_2_mt_5[j]) return back_2_mt_5[i] < back_2_mt_5[j];
            return back_1_mt_5[i] < back_1_mt_5[j];
        });
}

void do_back_search_mt_5(int starter) {
    back_num_mt_5 = 0;
    int fake_num[2]; fake_num[0] = fake_num[1] = 0;

    refresh_starter_mt_5(starter, starter);
    int u, v, w, x, y, z, o, index;
    for (int i=edge_topo_starter_mt_5[starter]; i<edge_topo_header[starter+1]; ++i) {
        u = edge_topo_edges[i]; x = edge_topo_values[i];
        front_visit_mt_5[u] = mask_mt_5;
        front_value_mt_5[u] = x;
    }

    for (int i=rev_edge_topo_header[starter]; i<rev_edge_topo_header[starter+1]; ++i) {
        u = rev_edge_topo_edges[i]; x = rev_edge_topo_values[i];
        if (u <= starter) break;
        // printf("u: %d\n", u);
        for (int j=rev_edge_topo_header[u]; j<rev_edge_topo_header[u+1]; ++j) {
            v = rev_edge_topo_edges[j]; y = rev_edge_topo_values[j];
            // printf("v0: %d\n", v);
            if (v <= starter) break;
            // printf("v1: %d\n", v);
            if (!check_x_y(y, x)) continue;
            // printf("v: %d\n", v);
            // circle 3
            if (front_visit_mt_5[v] == mask_mt_5) {
                z = front_value_mt_5[v];
                if (check_x_y(z, y) && check_x_y(x, z)) {
                    index = fake_num[0];
                    answer_fake_mt_5[0][index ++] = v;
                    answer_fake_mt_5[0][index ++] = u;
                    fake_num[0] += 2;
                    total_answer_num_mt_5 ++;
                    // printf("%d %d %d\n", data_rev_mapping[starter], data_rev_mapping[v], data_rev_mapping[u]);
                }
            }

            for (int k=rev_edge_topo_header[v]; k<rev_edge_topo_header[v+1]; ++k) {
                w = rev_edge_topo_edges[k]; z = rev_edge_topo_values[k];
                if (w <= starter) break;
                if (w == u) continue;
                if (!check_x_y(z, y)) continue;
                // printf("w: %d\n", w);
                // circle 4
                if (front_visit_mt_5[w] == mask_mt_5) {
                    o = front_value_mt_5[w];
                    if (check_x_y(x, o) && check_x_y(o, z)) {
                        index = fake_num[1];
                        answer_fake_mt_5[1][index ++] = w;
                        answer_fake_mt_5[1][index ++] = v;
                        answer_fake_mt_5[1][index ++] = u;
                        fake_num[1] += 3;
                        total_answer_num_mt_5 ++;
                        // printf("%d %d %d %d\n", data_rev_mapping[starter], data_rev_mapping[w], data_rev_mapping[v], data_rev_mapping[u]);
                    }
                }

                if (back_visit_mt_5[w] != mask_mt_5) {
                    back_visit_mt_5[w] = mask_mt_5;
                    back_header_mt_5[w] = -1;
                }
                index = back_num_mt_5 ++;
                back_value_front_mt_5[index] = x;
                back_value_back_mt_5[index] = z;
                back_1_mt_5[index] = u;
                back_2_mt_5[index] = v;
                back_nxt_mt_5[index] = back_header_mt_5[w];
                back_header_mt_5[w] = index;
            }
        }
    }

    int index_num;
    if (fake_num[0] > 0) {
        index_num = 0;
        for (int i=0; i<fake_num[0]; i+=2) back_sorted_index_mt_5[index_num ++] = i;
        std::sort(back_sorted_index_mt_5, back_sorted_index_mt_5 + index_num, [](int i, int j) {
            if (answer_fake_mt_5[0][i] != answer_fake_mt_5[0][j]) return answer_fake_mt_5[0][i] < answer_fake_mt_5[0][j];
            return answer_fake_mt_5[0][i+1] < answer_fake_mt_5[0][j+1];
        });
        for (int i=0; i<index_num; ++i) {
            index = back_sorted_index_mt_5[i];
            answer_mt_5[0][answer_num_mt_5[0] ++] = answer_fake_mt_5[0][index];
            answer_mt_5[0][answer_num_mt_5[0] ++] = answer_fake_mt_5[0][index+1];
        }
    }
    if (fake_num[1] > 0) {
        index_num = 0;
        for (int i=0; i<fake_num[1]; i+=3) back_sorted_index_mt_5[index_num ++] = i;
        std::sort(back_sorted_index_mt_5, back_sorted_index_mt_5 + index_num, [](int i, int j) {
            if (answer_fake_mt_5[1][i] != answer_fake_mt_5[1][j]) return answer_fake_mt_5[1][i] < answer_fake_mt_5[1][j];
            if (answer_fake_mt_5[1][i+1] != answer_fake_mt_5[1][j+1]) return answer_fake_mt_5[1][i+1] < answer_fake_mt_5[1][j+1];
            return answer_fake_mt_5[1][i+2] < answer_fake_mt_5[1][j+2];
        });
        for (int i=0; i<index_num; ++i) {
            index = back_sorted_index_mt_5[i];
            answer_mt_5[1][answer_num_mt_5[1] ++] = answer_fake_mt_5[1][index];
            answer_mt_5[1][answer_num_mt_5[1] ++] = answer_fake_mt_5[1][index+1];
            answer_mt_5[1][answer_num_mt_5[1] ++] = answer_fake_mt_5[1][index+2];
        }
    }
}

void do_front_search_mt_5(int starter) {
    int x, y, z, t, o, f;
    int u, v, w, e, index, ai;
    int back1, back2;

    visit_mt_5[starter] = mask_mt_5;
    refresh_starter_mt_5(starter, starter);
    for (int i=edge_topo_starter_mt_5[starter]; i<edge_topo_header[starter+1]; ++i) {
        u = edge_topo_edges[i]; x = edge_topo_values[i];
        visit_mt_5[u] = mask_mt_5;
        // printf("u: %d\n", u);
        refresh_starter_mt_5(u, starter);
        for (int j=edge_topo_starter_mt_5[u]; j<edge_topo_header[u+1]; ++j) {
            v = edge_topo_edges[j]; y = edge_topo_values[j];
            if (!check_x_y(x, y)) continue;
            visit_mt_5[v] = mask_mt_5;
            // printf("v: %d\n", v);
            // circle 5
            gain_sorted_back_mt_5(v);
            for (int r=back_sorted_from_mt_5[v]; r<back_sorted_to_mt_5[v]; ++r) {
                index = back_sorted_index_mt_5[r];
                o = back_value_back_mt_5[index];
                f = back_value_front_mt_5[index];
                back1 = back_1_mt_5[index];
                back2 = back_2_mt_5[index];
                if (check_x_y(f, x) && check_x_y(y, o) && visit_mt_5[back1] != mask_mt_5 && visit_mt_5[back2] != mask_mt_5) {
                    ai = answer_num_mt_5[2];
                    answer_mt_5[2][ai ++] = u;
                    answer_mt_5[2][ai ++] = v;
                    answer_mt_5[2][ai ++] = back2;
                    answer_mt_5[2][ai ++] = back1;
                    answer_num_mt_5[2] += 4;
                    total_answer_num_mt_5 ++;
                }
            }

            refresh_starter_mt_5(v, starter);
            for (int k=edge_topo_starter_mt_5[v]; k<edge_topo_header[v+1]; ++k) {
                w = edge_topo_edges[k]; z = edge_topo_values[k];
                if (!check_x_y(y, z)) continue;
                if (u == w) continue;
                visit_mt_5[w] = mask_mt_5;
                // printf("w: %d\n", w);
                // circle 6
                gain_sorted_back_mt_5(w);
                for (int r=back_sorted_from_mt_5[w]; r<back_sorted_to_mt_5[w]; ++r) {
                    index = back_sorted_index_mt_5[r];
                    o = back_value_back_mt_5[index];
                    f = back_value_front_mt_5[index];
                    back1 = back_1_mt_5[index];
                    back2 = back_2_mt_5[index];
                    if (check_x_y(f, x) && check_x_y(z, o) && visit_mt_5[back1] != mask_mt_5 && visit_mt_5[back2] != mask_mt_5) {
                        ai = answer_num_mt_5[3];
                        answer_mt_5[3][ai ++] = u;
                        answer_mt_5[3][ai ++] = v;
                        answer_mt_5[3][ai ++] = w;
                        answer_mt_5[3][ai ++] = back2;
                        answer_mt_5[3][ai ++] = back1;
                        answer_num_mt_5[3] += 5;
                        total_answer_num_mt_5 ++;
                    }
                }

                refresh_starter_mt_5(w, starter);
                for (int l=edge_topo_starter_mt_5[w]; l<edge_topo_header[w+1]; ++l) {
                    e = edge_topo_edges[l]; t = edge_topo_values[l];
                    if (!check_x_y(z, t)) continue;
                    if (e == v || e == u) continue;
                    visit_mt_5[e] = mask_mt_5;
                    // printf("e: %d\n", e);
                    // circle 7
                    gain_sorted_back_mt_5(e);
                    for (int r=back_sorted_from_mt_5[e]; r<back_sorted_to_mt_5[e]; ++r) {
                        index = back_sorted_index_mt_5[r];
                        o = back_value_back_mt_5[index];
                        f = back_value_front_mt_5[index];
                        back1 = back_1_mt_5[index];
                        back2 = back_2_mt_5[index];
                        if (check_x_y(f, x) && check_x_y(t, o) && visit_mt_5[back1] != mask_mt_5 && visit_mt_5[back2] != mask_mt_5) {
                            ai = answer_num_mt_5[4];
                            answer_mt_5[4][ai ++] = u;
                            answer_mt_5[4][ai ++] = v;
                            answer_mt_5[4][ai ++] = w;
                            answer_mt_5[4][ai ++] = e;
                            answer_mt_5[4][ai ++] = back2;
                            answer_mt_5[4][ai ++] = back1;
                            answer_num_mt_5[4] += 6;
                            total_answer_num_mt_5 ++;
                        }
                    }
                    visit_mt_5[e] = -1;
                }
                visit_mt_5[w] = -1;
            }
            visit_mt_5[v] = -1;
        }
        visit_mt_5[u] = -1;
    }
}

void do_search_mt_5(int starter) {
    mask_mt_5 ++;
    back_sorted_num_mt_5 = 0;
    do_back_search_mt_5(starter);
    if (back_num_mt_5 == 0) return;
    do_front_search_mt_5(starter);
}

void* search_mt_5(void* args) {
    malloc_mt_5();
    memcpy(edge_topo_starter_mt_5, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_counter, 1);
        if (u >= node_num) break;
        global_assign[u] = 5;
        for (int i=0; i<5; ++i) {
            answer_header[u][i] = answer_num_mt_5[i];
        }
        // printf("starter: %d\n", u);
        // printf("starter: %d %d %d %d\n", u, data_rev_mapping[u], answer_header[u][0], answer_header[u][1]);
        do_search_mt_5(u);
        // printf("\n");
        for (int i=0; i<5; ++i) {
            answer_tail[u][i] = answer_num_mt_5[i];
        }
        // printf("ender: %d %d %d\n", u, answer_tail[u][0], answer_tail[u][1]);
    }
    free_mt_5();
    return NULL;
}

// search mt_5



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
        switch (global_assign[u])
        {
        case 0:
            for (int i=answer_header[u][id]; i<answer_tail[u][id]; i+=unit_size) {
                deserialize_id(buffer, buffer_num, u);
                for (int j=i; j<i+unit_size; ++j) {
                    buffer[buffer_num ++] = ',';
                    deserialize_id(buffer, buffer_num, answer_mt_0[id][j]);
                }
                buffer[buffer_num ++] = '\n';
            }
            break;
        case 1:
            for (int i=answer_header[u][id]; i<answer_tail[u][id]; i+=unit_size) {
                deserialize_id(buffer, buffer_num, u);
                for (int j=i; j<i+unit_size; ++j) {
                    buffer[buffer_num ++] = ',';
                    deserialize_id(buffer, buffer_num, answer_mt_1[id][j]);
                }
                buffer[buffer_num ++] = '\n';
            }
            break;
        case 2:
            for (int i=answer_header[u][id]; i<answer_tail[u][id]; i+=unit_size) {
                deserialize_id(buffer, buffer_num, u);
                for (int j=i; j<i+unit_size; ++j) {
                    buffer[buffer_num ++] = ',';
                    deserialize_id(buffer, buffer_num, answer_mt_2[id][j]);
                }
                buffer[buffer_num ++] = '\n';
            }
            break;
        case 3:
            for (int i=answer_header[u][id]; i<answer_tail[u][id]; i+=unit_size) {
                deserialize_id(buffer, buffer_num, u);
                for (int j=i; j<i+unit_size; ++j) {
                    buffer[buffer_num ++] = ',';
                    deserialize_id(buffer, buffer_num, answer_mt_3[id][j]);
                }
                buffer[buffer_num ++] = '\n';
            }
            break;
        case 4:
            for (int i=answer_header[u][id]; i<answer_tail[u][id]; i+=unit_size) {
                deserialize_id(buffer, buffer_num, u);
                for (int j=i; j<i+unit_size; ++j) {
                    buffer[buffer_num ++] = ',';
                    deserialize_id(buffer, buffer_num, answer_mt_4[id][j]);
                }
                buffer[buffer_num ++] = '\n';
            }
            break;
        case 5:
            for (int i=answer_header[u][id]; i<answer_tail[u][id]; i+=unit_size) {
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

char* buffer_io_0;
int buffer_num_io_0;

void* write_to_disk_io_0(void* args) {
    buffer_io_0 = (char*) malloc(ONE_INT_CHAR_SIZE * MAX_ANSW);
    deserialize_int(buffer_io_0, buffer_num_io_0, answer_num);
    buffer_io_0[buffer_num_io_0 ++] = '\n';
    write_to_disk(buffer_io_0, buffer_num_io_0, 0, 0, node_num);
    write_to_disk(buffer_io_0, buffer_num_io_0, 1, 0, node_num);
    write_to_disk(buffer_io_0, buffer_num_io_0, 2, 0, node_num);
    return NULL;
}

char* buffer_io_1;
int buffer_num_io_1;

void* write_to_disk_io_1(void* args) {
    buffer_io_1 = (char*) malloc(ONE_INT_CHAR_SIZE * MAX_ANSW * 2);
    write_to_disk(buffer_io_1, buffer_num_io_1, 3, 0, node_num);
    return NULL;
}

char* buffer_io_2;
int buffer_num_io_2;

void* write_to_disk_io_2(void* args) {
    buffer_io_2 = (char*) malloc(ONE_INT_CHAR_SIZE * MAX_ANSW * 3);
    write_to_disk(buffer_io_2, buffer_num_io_2, 4, 0, node_num / 10);
    return NULL;
}

char* buffer_io_3;
int buffer_num_io_3;

void* write_to_disk_io_3(void* args) {
    buffer_io_3 = (char*) malloc(ONE_INT_CHAR_SIZE * MAX_ANSW * 3);
    write_to_disk(buffer_io_3, buffer_num_io_3, 4, node_num / 10, node_num/4);
    return NULL;
}

char* buffer_io_4;
int buffer_num_io_4;

void* write_to_disk_io_4(void* args) {
    buffer_io_4 = (char*) malloc(ONE_INT_CHAR_SIZE * MAX_ANSW * 3);
    write_to_disk(buffer_io_4, buffer_num_io_4, 4, node_num/4, node_num/2);
    return NULL;
}

char* buffer_io_5;
int buffer_num_io_5;

void* write_to_disk_io_5(void* args) {
    buffer_io_5 = (char*) malloc(ONE_INT_CHAR_SIZE * MAX_ANSW * 3);
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
    
    printf("before rehash\n"); fflush(stdout);

    rehash_nodes();
    

    filter_searchable_nodes();
    create_integer_buffer();

    printf("before search\n"); fflush(stdout);

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

    answer_num = total_answer_num_mt_0 + total_answer_num_mt_1 + total_answer_num_mt_2 + 
        total_answer_num_mt_3 + total_answer_num_mt_4 + total_answer_num_mt_5;

    printf("answer_num: %d\n", answer_num);

    create_writer_fd();
    write_to_disk();
    close(writer_fd);

    return 0;
}

//------------------ FBODY ---------------------


