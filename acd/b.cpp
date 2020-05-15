#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <sys/stat.h>
#include <string.h>
#include <cmath>
#include "arm_neon.h"
#include <sys/mman.h>
#include <sys/types.h> 
#include <fcntl.h> 
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h> 
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

// 字符串映射到浮点数
float str2pfloat[59]; 
float str2nfloat[59];
int init_strmap()
{
    const clock_t begin_time = clock();
    float tmp;
    
    for (int i = 0; i < 10; i++)
    {
        tmp = float(i) / 10.0;
        str2pfloat[i+48] = tmp;  // '0' = 48
        str2nfloat[i+48] = -1.0 * tmp;
    } 
    float dt = float(clock() - begin_time);
    printf("init_strmap() time: %f\n", dt / CLOCKS_PER_SEC);
    return 0;
} 
int r = init_strmap();

// 获取文件大小
int get_file_size(const char *path)  
{       
    struct stat statbuff; 
    stat(path, &statbuff); 
    return statbuff.st_size;  
}  

// 数据相关变量
const char *train_path = "/data/train_data.txt";
const char *test_path = "/data/test_data.txt";
const char *predict_path = "/projects/student/result.txt";

// const char *train_path = "./data/train_data.txt";
// const char *test_path = "./data/test3.txt";
// const char *predict_path = "./data/result.txt";

int train_size = get_file_size(train_path); // n_train * 6100
int test_size = get_file_size(test_path);
int n_train = 2000;
int n_test = test_size / 6000; // 每行平均6000字节

// mmap 
int test_fd = open(test_path, O_RDONLY, S_IRUSR);
char *test_ptr = (char *)mmap(NULL, test_size, PROT_READ, MAP_SHARED, test_fd, 0);

// 定义区间 [start, end)
struct Interval
{
    int id;
    int start; 
    int end;
    int i;
    int n;
};

// 获取文件数据分段区间
Interval* get_interval(int n_split, const char* path, bool is_test)
{
    Interval* ins = new Interval[n_split];
    int data_size = get_file_size(path);
    int dsize = data_size / n_split;
    FILE *fp = fopen(path, "r");
    char *line = new char[8000];
    int ret_pos;
    int split_pos;
    for (int i = 1; i < n_split; i++)
    {
        split_pos = i * dsize;
        fseek(fp, split_pos-1, SEEK_SET);
        char curr = fgetc(fp);
        if (curr != '\n') 
        {
            // printf("%c\n", curr);
            fgets(line, 8000, fp);
            split_pos = ftell(fp);
        }
        ins[i].start = split_pos;
        ins[i-1].end = split_pos;
    }
    ins[0].start = 0; 
    ins[0].i = 0;
    ins[n_split-1].end = data_size;
    if (is_test)
    {
        for (int i = 0; i < n_split; i++)
        {
            ins[i].n = (ins[i].end - ins[i].start) / 6000;
        }
        for (int i = 1; i < n_split; i++)
        {
            ins[i].i = ins[i-1].i + ins[i-1].n;
        }
    }
    else
    {
        for (int i = 0; i < n_split; i++)
        {
            ins[i].id = i;
            ins[i].n = n_train / n_split;
            ins[i].i = i * ins[i].n;
        }
    }
    return ins;
}

// 按区间读训练集
void read_train_interval(Interval* in_ptr, float* sum_p, float *sum_n, int *counter_yp)
{
    int train_fd = open(train_path, O_RDONLY, S_IRUSR);
    char *train_ptr = (char *)mmap(NULL, train_size, PROT_READ, MAP_SHARED, train_fd, 0);
    Interval in = *in_ptr;
    int k = in.start;
    float* val = new float[300];
    for(int i = in.i; i < in.i + in.n; i++) 
    {
        for (int j = 0; j < 300; j++)
        {
            if (train_ptr[k] == '-')
            {
                val[j] = str2nfloat[train_ptr[k+3]];
                k += 7;
            }
            else
            {
                val[j] = str2pfloat[train_ptr[k+2]];
                k += 6;
            }
        }
        for (int j = 0; j < 700; j++)
        {
            if (train_ptr[k] == '-')
                k += 7;
            else
                k += 6;
        }
        if (train_ptr[k] == '0')
        {
            for (int j = 0; j < 300; j++)
            {
                sum_n[j] += val[j];
            }
        }
        else
        {
            *counter_yp += 1;
            for (int j = 0; j < 300; j++)
            {
                sum_p[j] += val[j];
            }
        }
        k += 2;
    }
}

// 按区间读测试集并预测
void read_test_interval(Interval* in_ptr, float* m1_m0, float *M1M0, char* predict_str)
{
    Interval in = *in_ptr;
    int ii;
    float dis_p;
    float dis_n;
    int k = in.start;
    float32x4_t sum_vec;
    float32x4_t vec_a, vec_b;
    float32x2_t r;
    float m1m0X;
    // 拷贝变量
    float *m1_m0_t = new float[300];
    memcpy(m1_m0_t, m1_m0, sizeof(float)*300);
    float M1M0_t = *M1M0;
    for (int i = in.i; i < in.i + in.n; i++)
    {
        // m1_m0 * X > M!M0 ? 0 : 1
        sum_vec = vdupq_n_f32(0);
        for (int j = 0; j < 300; j += 4)
        {
            vec_a[0] = str2pfloat[test_ptr[k+2]];
            vec_a[1] = str2pfloat[test_ptr[k+8]];
            vec_a[2] = str2pfloat[test_ptr[k+14]];
            vec_a[3] = str2pfloat[test_ptr[k+20]];
            vec_b = vld1q_f32(m1_m0_t + j);
            sum_vec = vmlaq_f32(sum_vec, vec_a, vec_b);
            k += 24;
        }
        k += 6 * 700;
        r = vadd_f32(vget_high_f32(sum_vec), vget_low_f32(sum_vec));
        m1m0X = vget_lane_f32(vpadd_f32(r,r),0);
        
        ii = 2 * i;
        // printf("%f, %f\n", m1m0X, M1M0);
        if (m1m0X >= M1M0_t)
            predict_str[ii] = '1';   
        else
            predict_str[ii] = '0';
        predict_str[ii+1] = '\n';
    }
}


void read_answer(const char *path, int n, float *x)
{
    FILE *fp = fopen(path, "r");
    for (int i = 0; i < n; i++)
    {
        fscanf(fp, "%f\n", x+i);
    }
    fclose(fp);
}

float accuracy(int n, float *a, float *b)
{
    float correct = 0.0;
    for (int i = 0; i < n; i++)
    {
        if (int(a[i]) == int(b[i]))
        {
            correct++;
        }
    }
    return correct / n;
}


float compute_xx(int n, float *a) 
{
    // Neon
    float32x4_t sum_vec=vdupq_n_f32(0), vec;
    for (int j = 0; j < n; j += 4)
    {
        vec = vld1q_f32(a + j);
        sum_vec = vmlaq_f32(sum_vec, vec, vec);
    }
    float32x2_t r=vadd_f32(vget_high_f32(sum_vec),vget_low_f32(sum_vec));
    float c = vget_lane_f32(vpadd_f32(r,r),0);
    return c;
}

#define WAIT 10
#define DONE 20

int main(int argc, char *argv[])
{
    // 共享变量
    int sig0_shm_id = shmget(IPC_PRIVATE, 1, 0666 | IPC_CREAT | IPC_EXCL);
    int sig1_shm_id = shmget(IPC_PRIVATE, 1, 0666 | IPC_CREAT | IPC_EXCL);
    int sig2_shm_id = shmget(IPC_PRIVATE, 1, 0666 | IPC_CREAT | IPC_EXCL);
    int sig3_shm_id = shmget(IPC_PRIVATE, 1, 0666 | IPC_CREAT | IPC_EXCL);
    int sig_shm_ids[3] = {sig1_shm_id, sig2_shm_id, sig3_shm_id};
    int sp1_shm_id = shmget(IPC_PRIVATE, sizeof(float) * 300, 0666 | IPC_CREAT | IPC_EXCL);
    int sp2_shm_id = shmget(IPC_PRIVATE, sizeof(float) * 300, 0666 | IPC_CREAT | IPC_EXCL);
    int sp3_shm_id = shmget(IPC_PRIVATE, sizeof(float) * 300, 0666 | IPC_CREAT | IPC_EXCL);
    int sn1_shm_id = shmget(IPC_PRIVATE, sizeof(float) * 300, 0666 | IPC_CREAT | IPC_EXCL);
    int sn2_shm_id = shmget(IPC_PRIVATE, sizeof(float) * 300, 0666 | IPC_CREAT | IPC_EXCL);
    int sn3_shm_id = shmget(IPC_PRIVATE, sizeof(float) * 300, 0666 | IPC_CREAT | IPC_EXCL);
    int cy1_shm_id = shmget(IPC_PRIVATE, sizeof(int) * 1, 0666 | IPC_CREAT | IPC_EXCL);
    int cy2_shm_id = shmget(IPC_PRIVATE, sizeof(int) * 1, 0666 | IPC_CREAT | IPC_EXCL);
    int cy3_shm_id = shmget(IPC_PRIVATE, sizeof(int) * 1, 0666 | IPC_CREAT | IPC_EXCL);
    int sp_shm_ids[3] = {sp1_shm_id, sp2_shm_id, sp3_shm_id};
    int sn_shm_ids[3] = {sn1_shm_id, sn2_shm_id, sn3_shm_id};
    int cy_shm_ids[3] = {cy1_shm_id, cy2_shm_id, cy3_shm_id};
    int m1_m0_shm_id = shmget(IPC_PRIVATE, sizeof(float) * 300, 0666 | IPC_CREAT | IPC_EXCL);
    int M1M0_shm_id = shmget(IPC_PRIVATE, sizeof(float) * 1, 0666 | IPC_CREAT | IPC_EXCL);
    int pstr_shm_id = shmget(IPC_PRIVATE, n_test * 2, 0666 | IPC_CREAT | IPC_EXCL);
    
    // printf("%d, %d, %d\n", sp1_shm_id, sp2_shm_id, sp3_shm_id);

    volatile char* sig; // 获得当前进程相应的信号量
    volatile char *sig_main, *sig1, *sig2, *sig3;
    float* sum_p;
    float* sum_n;
    int* counter_y;
    float *sum_p1, *sum_p2, *sum_p3, *sum_n1, *sum_n2, *sum_n3;
    int *cy1, *cy2, *cy3;
    float *m1_m0;
    float *M1M0;
    char *predict_str;

    Interval* ins_rd_train = get_interval(4, train_path, false); 
    Interval* ins_rd_test = get_interval(4, test_path, true); 

    // 读训练集
    pid_t pid;
    int i;
    for(i = 0; i < 3; i++) // 创建三个子进程
    {
        pid = fork();
        if(pid == 0)  // 子进程跳出循环
            break;
    }

    m1_m0 = (float *)shmat(m1_m0_shm_id, NULL, 0);
    M1M0 = (float *)shmat(M1M0_shm_id, NULL, 0);
    predict_str = (char *)shmat(pstr_shm_id, NULL, 0);

    if (pid == 0)
    {
        // 子进程1,2,3
        sig = (char *)shmat(sig_shm_ids[i], NULL, 0);
        sig_main = (char *)shmat(sig0_shm_id, NULL, 0);
        *sig = WAIT;
        
        sum_p = (float *)shmat(sp_shm_ids[i], NULL, 0);
        sum_n = (float *)shmat(sn_shm_ids[i], NULL, 0);
        counter_y = (int *)shmat(cy_shm_ids[i], NULL, 0);
        for (int j = 0; j < 300; j++)
        {
            *(sum_p + j) = 0;
            *(sum_n + j) = 0;
        }
        *counter_y = 0;
        read_train_interval(ins_rd_train + i, sum_p, sum_n, counter_y);

        printf("sub %d read train done.\n", i);
        *sig = DONE;
        while (1)
            if (*sig_main == DONE)
                break;
        printf("sub %d: main done.\n", i);
        printf("%f %f\n", m1_m0[0], *M1M0);

        *sig = WAIT;
        read_test_interval(ins_rd_test + i, m1_m0, M1M0, predict_str);
        printf("sub %d read test done.\n", i);
        *sig = DONE;
        while (1)
            if (*sig_main == DONE)
                break;
        printf("sub %d: main done.\n", i);

        exit(0);
    }
    else
    {
        // 主进程
        sig = (char *)shmat(sig0_shm_id, NULL, 0);
        sig1 = (char *)shmat(sig1_shm_id, NULL, 0);
        sig2 = (char *)shmat(sig2_shm_id, NULL, 0);
        sig3 = (char *)shmat(sig3_shm_id, NULL, 0);
        
        *sig = WAIT;

        sum_p = new float[300]{0};
        sum_n = new float[300]{0};
        counter_y = new int(0);
        read_train_interval(ins_rd_train + i, sum_p, sum_n, counter_y);

        printf("main read train done.\n");
        while (1)
            if ((*sig1 == DONE) && (*sig2 == DONE) && (*sig3 == DONE))
                break;
        printf("sub all read train done.\n");

        // compute 
        sum_p1 = (float *)shmat(sp1_shm_id, NULL, 0);
        sum_p2 = (float *)shmat(sp2_shm_id, NULL, 0);
        sum_p3 = (float *)shmat(sp3_shm_id, NULL, 0);
        sum_n1 = (float *)shmat(sn1_shm_id, NULL, 0);
        sum_n2 = (float *)shmat(sn2_shm_id, NULL, 0);
        sum_n3 = (float *)shmat(sn3_shm_id, NULL, 0);
        cy1 = (int *)shmat(cy1_shm_id, NULL, 0);
        cy2 = (int *)shmat(cy2_shm_id, NULL, 0);
        cy3 = (int *)shmat(cy3_shm_id, NULL, 0);
        float *mean_p = new float[300];
        float *mean_n = new float[300];
        int yp = *counter_y + *cy1 + *cy2 + *cy3;
        int yn = n_train - yp;
        for (int j = 0; j < 300; j++)
        {
            mean_n[j] = (sum_n[j] + sum_n1[j] + sum_n2[j] + sum_n3[j]) / yn;
            mean_p[j] = (sum_p[j] + sum_p1[j] + sum_p2[j] + sum_p3[j]) / yp;
            m1_m0[j] = mean_p[j] - mean_n[j];
        }
        *M1M0 = (compute_xx(300, mean_p) - compute_xx(300, mean_n)) / 2 - 0.09;
        printf("%d, %d\n", yp, yn);
        printf("%f %f\n", m1_m0[0], *M1M0);
        *sig = DONE;

        while (1)
            if ((*sig1 == WAIT) && (*sig2 == WAIT) && (*sig3 == WAIT))
                break;
        *sig = WAIT;
        read_test_interval(ins_rd_test + i, m1_m0, M1M0, predict_str);

        printf("main read test done.\n");
        while (1)
            if ((*sig1 == DONE) && (*sig2 == DONE) && (*sig3 == DONE))
                break;
        printf("sub all read test done.\n");
        *sig = DONE;

        
        predict_str[n_test * 2 - 1] = '\0';
        FILE *fp = fopen(predict_path, "w");
        fputs(predict_str, fp);
        fclose(fp);
    }
    
    // // 线下验证
    // const char *answer_path = "./data/ans3.txt";
    // float *answer = new float[n_test];
    // float *predict = new float[n_test];
    // read_answer(answer_path, n_test, answer);
    // read_answer(predict_path, n_test, predict);
    // float acc = accuracy(n_test, answer, predict);
    // printf("test acc: %f\n", acc);
    // delete[]answer;
    // delete[]predict;

    return 0;
}
