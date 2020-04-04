#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <unordered_map>

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
// #define INPUT_PATH  "resources/test_data.txt"
#define INPUT_PATH  "gen_data.txt"
#define OUTPUT_PATH "test_output.txt"

#else
// 输入输出路径
#define INPUT_PATH  "/data/test_data.txt"
#define OUTPUT_PATH "/projects/student/result.txt"

#endif

// 估计数据的大小，来确定缓存的大小
#define MAX_LINE                 280000 // 数据最多28W行
#define MAX_FOR_EACH_LINE        33     // 每个32位正整数10个字符 + 2个逗号 + 一个换行

//------------------ DECLR ---------------------


char buffer[MAX_LINE * MAX_FOR_EACH_LINE];
int  data_num = 0;
int  data[MAX_LINE][3];

void read_input();
void write_output();

//------------------ MBODY ---------------------
int main() {
#ifdef TEST
    start_time = clock();
#endif
    
    read_input();
    say_out("data size: %d", data_num);

#ifdef TEST
    end_time = clock();
    say_out("running: %.6lf", ((double)(end_time - start_time) / PER_SEC));
#endif
    return 0;
}

//------------------ FBODY ---------------------


void read_input() {
    int fd = open(INPUT_PATH, O_RDONLY);
    size_t size = read(fd, &buffer, MAX_LINE * MAX_FOR_EACH_LINE);
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

void write_output() {
    // todo 需要确定输出格式，或者解析部分另外写
}