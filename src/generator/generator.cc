#include <iostream>
#include <vector>
#include <set>
#include <random>
#include <chrono>
#include <algorithm>
#include <cmath>

using namespace std;

#define N         2800
#define M         28000

unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
mt19937 rand_num(seed);

int random32() {
    int x = rand_num();
    return x >= 0 ? x : -x;
}

vector<int> node_generator() {
    set<int> exist;
    vector<int> ids;
    while (ids.size() < N) {
        int id = random32();
        if (exist.find(id) == exist.end()) {
            ids.push_back(id);
            exist.insert(id);
        }
    }
    return ids;
}

vector<vector<int>> edge_generator(vector<int> nodes) {
    vector<vector<int>> edges(N);
    for (int i=0; i<M; ++i) {
        int u = random32() % N;
        int v = u;
        while (u == v) v = random32() % N;
        edges[u].push_back(v);
    }
    vector<vector<int>> format_edges;
    for (int i=0; i<N; ++i) {
        for (int j=0; j<edges[i].size(); ++j) {
            vector<int> lines;
            lines.push_back(nodes[i]);
            lines.push_back(nodes[edges[i][j]]);
            lines.push_back(random32());
            format_edges.emplace_back(lines);
        }
    }
    random_shuffle(format_edges.begin(), format_edges.end());
    return format_edges;
}

int main() {
    vector<int> nodes = node_generator();
    vector<vector<int>> edges = edge_generator(nodes);
    for (int i=0; i<edges.size(); ++i) {
        cout << edges[i][0] << "," << edges[i][1] << "," << edges[i][2] << endl;
    }
}