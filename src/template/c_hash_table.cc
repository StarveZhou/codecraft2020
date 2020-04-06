#include <iostream>
#include <vector>
#include <unordered_map>
#include <random>
#include <chrono>

using namespace std;

#define MAX_NODE                 280000


#include <time.h>
clock_t start_time, end_time;
#define PER_SEC     1000
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

int c_hash_map_insert_if_absent_with_inc(c_hash_map hash, int key, int& value) {
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
    return value ++;
}

// *************** hash map over *********************

unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
mt19937 rand_num(seed);
int random32() {
    int x = rand_num();
    return x >= 0 ? x : -x;
}

vector<int> random_data(int size) {
    vector<int> data(size);
    for (int i=0; i<size; ++i) {
        data[i] = random32();
    }
    return data;
}
vector<int> sequetial_data(int size) {
    vector<int> data(size);
    for (int i=0; i<size; ++i) {
        data[i] = i;
    }
    return data;
}

c_hash_map_t hash_map_t; c_hash_map hash_map = &hash_map_t;

void correctness_test(int size, bool random) {
    unordered_map<int, int> std_map;

    vector<int> data;
    if (random) data = random_data(size);
    else data = sequetial_data(size);

    for (int i=0; i<size; ++i) {
        std_map[i] = data[i];
        c_hash_map_insert_if_absent(hash_map, i, data[i]);
    }

    int cnt = 0;
    for (int i=0; i<size; ++i) {
        if (std_map[i] != c_hash_map_get(hash_map, i)) {
            cout << "error: " << i << ": " << std_map[i] << ", " << c_hash_map_get(hash_map, i) << endl << flush;
        } else cnt ++;
    }
    cout << "correctness: " << cnt << endl;
}

void time_test(int size, int random) {
    cout << "start time test" << endl << flush;
    
    unordered_map<int, int> std_map;

    vector<int> data;
    if (random) data = random_data(size);
    else data = sequetial_data(size);

    start_time = clock();
    for (int i=0; i<size; ++i) {
        if (std_map.find(i) == std_map.end()) std_map[i] = data[i];
    }
    end_time = clock();
    cout << "std running: " << (end_time - start_time) << "s" << endl << flush;

    start_time = clock();
    for (int i=0; i<size; ++i) {
        c_hash_map_insert_if_absent(hash_map, i, data[i]);
    }
    end_time = clock();
    cout << "chm running: " << (end_time - start_time) << "s" << endl << flush;
}

int main() {
    cout << "what ?" << endl << flush;
    // correctness_test(MAX_NODE, false);
    time_test(MAX_NODE, false);
}