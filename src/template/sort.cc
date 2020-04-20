#include <iostream>
#include <algorithm>
#include <time.h>
#include <random>
#include <chrono>
#include <cassert>
#include <string.h>

using namespace std;

#define N    20

unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
mt19937 rand_num(seed);
int random32() {
    int x = rand_num();
    return x >= 0 ? x : -x;
}

int a[N], b[N];
void generate_data() {
    for (int i=0; i<N; ++i) a[i] = i;
    random_shuffle(a, a+N);
    for (int i=0; i<N; ++i) b[i] = a[i];
}



inline void swap_int(int* x, int* y) {
    if (x == y) return;
    *x ^= *y; *y ^= *x; *x ^= *y;
}


/**
 * NOTE: l and r are inclusive
 */
void select_sort(int* arr, int l, int r) {
    int index, i, j;
    for (i=l; i<r; ++i) {
        index = i;
        for (j=i+1; j<=r; ++j) {
            if (arr[index] > arr[j]) index = j;
        }
        swap_int(arr + index, arr + i);
    }
}
/**
 * NOTE: l and r are inclusive
 */
void qsort(int* arr, int l, int r) {
    if (r-l == 1) {
        if (arr[l] > arr[r]) swap_int(arr+l, arr+r);
        return;
    }
    int i, j, mid;
    i = l;
    j = r;
    mid = arr[(i+j)/2];
    while (i < j) {
        while (arr[i] < mid) ++ i;
        while (arr[j] > mid) -- j;
        if (i <= j) {
            swap_int(arr + i, arr + j);
            ++ i; -- j;
        }
    }
    if (i < r) qsort(arr, i, r);
    if (l < j) qsort(arr, l, j);
}

void qsort_rev(int* arr, int l, int r) {
    if (r-l == 1) {
        if (arr[l] < arr[r]) swap_int(arr+l, arr+r);
        return;
    }
    int i, j, mid;
    i = l;
    j = r;
    mid = arr[(i+j)/2];
    while (i < j) {
        while (arr[i] > mid) ++ i;
        while (arr[j] < mid) -- j;
        if (i <= j) {
            swap_int(arr + i, arr + j);
            ++ i; -- j;
        }
    }
    if (i < r) qsort(arr, i, r);
    if (l < j) qsort(arr, l, j);
}

void c_sort(int* arr, int st, int ed) {
    qsort(arr, st, ed-1);
    // select_sort(arr, st, ed-1);
}

void c_with_timer(int* arr, int size) {
    time_t start, end;
    start = clock();
    c_sort(arr, 0, size);
    end = clock();
    cout << "time: " << end - start << endl; 
}

void std_with_timer(int* arr, int size) {
    time_t start, end;
    start = clock();
    sort(arr, arr+size);
    end = clock();
    cout << "time: " << end - start << endl; 
}

#define MAX_TIME    1000000
void test_time() {
    time_t st, ed;
    time_t std_time = 0, c_time = 0;
    for (int i=0; i<MAX_TIME; ++i) {
        random_shuffle(a, a+N);
        memcpy(b, a, N * sizeof(int));
        st = clock();
        c_sort(a, 0, N);
        ed = clock();
        c_time += ed - st;
        
        st = clock();
        sort(b, b + N);
        ed = clock();
        std_time += ed - st;

        for (int j=0; j<N; ++j) {
            assert(j == a[j] && j == b[j]);
        }
    }
    cout << "c_time: " << c_time << endl;
    cout << "s_time: " << std_time <<endl;
}



int main() {
    generate_data();
    test_time();
}