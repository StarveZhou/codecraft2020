#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>

#include <vector>
#include <unordered_map>
#include <queue>

#include <algorithm>
#include <string>

#include <time.h>

#include <cassert>

using namespace std;

// ----------------------------- MACRO ---------------------------------------------





// #define INPUT_PATH "resources/pre_test.txt"
// #define INPUT_PATH "resources/test_data.txt"

// #define INPUT_PATH  "resources/data1.txt"
#define INPUT_PATH  "resources/2861665.txt"
#define OUTPUT_PATH "stl_brute_output.txt"

// #define INPUT_PATH "/data/test_data.txt"
// #define OUTPUT_PATH "/projects/student/result.txt"


#define MAX_DEPTH         7

// ------------------------------ DECLARATION ---------------------------------------

vector<vector<int>> read_input();
void write_output(unordered_map<int, int>& mapper);

unordered_map<int, int> solve(vector<vector<int>>& data);
unordered_map<int, int> normalize(vector<vector<int>>& data);
vector<vector<int>> data_to_graph(int n, vector<vector<int>>& data);
vector<vector<int>> bfs_shortest_path(int n, vector<vector<int>>& graph);
vector<vector<vector<vector<int>>>> brute_search(int n, vector<vector<int>>& graph, vector<vector<int>>& s_dist);
void brute_dfs(vector<int>& path, int n, vector<vector<int>>& graph, vector<vector<int>>& result, vector<vector<int>>& s_dist);
bool compare_between_path(int x, int y);

// ------------------------------ MAIN BODY -----------------------------------------
vector<vector<vector<vector<int>>>> global_result;

int global_x, global_y;
int global_ret_size = 0;

int main() {
    clock_t start_time, end_time;
    start_time = clock();
    
    vector<vector<int>> data = read_input();
    cout << "data size: " << data.size() << endl;
    unordered_map<int, int> mapper = solve(data);

    end_time = clock();
    cout << "seaching: " << ((double)(end_time - start_time)) / CLOCKS_PER_SEC << "s" << endl;

    write_output(mapper);

    end_time = clock();
    cout << "running: " << ((double)(end_time - start_time)) / CLOCKS_PER_SEC << "s" << endl;

    return 0;
}

// ------------------------------ FUNC BODY -----------------------------------------

unordered_map<int, int> normalize(vector<vector<int>>& data) {
    vector<int> all_node;
    for (int i=0; i<data.size(); ++i) {
        all_node.emplace_back(data[i][0]);
        all_node.emplace_back(data[i][1]);
    }

    sort(all_node.begin(), all_node.end());
    vector<int>::iterator new_end = unique(all_node.begin(), all_node.end());
    all_node.erase(new_end, all_node.end());

    unordered_map<int, int> mapper, rev_mapper;
    for (int i=0; i<all_node.size(); ++i) {
        mapper[i] = all_node[i];
        rev_mapper[all_node[i]] = i;
    }

    for (int i=0; i<data.size(); ++i) {
        data[i][0] = rev_mapper[data[i][0]];
        data[i][1] = rev_mapper[data[i][1]];
    }
    return mapper;
}

vector<vector<int>> data_to_graph(int n, vector<vector<int>>& data) {
    vector<vector<int>> graph(n);
    for (int i=0; i<data.size(); ++i) {
        graph[data[i][0]].emplace_back(data[i][1]);
    }
    for (int i=0; i<graph.size(); ++i) {
        sort(graph[i].begin(), graph[i].end());
    }
    return graph;
}

void brute_dfs(vector<int>& path, int n, vector<vector<int>>& graph, vector<vector<vector<vector<int>>>>& result, vector<vector<int>>& s_dist) {
    if (path.size() > 3 &&  path.front() == path.back()) {
        global_ret_size ++;
        path.pop_back();
        result[path.front()][path.size() - 3].push_back(path);
        path.push_back(path.front());
        return;
    }

    if (path.size() > MAX_DEPTH) {
        return;
    }

    if (path.size() == 0) {
        for (int i=0; i<n; ++i) {
            path.emplace_back(i);
            brute_dfs(path, n, graph, result, s_dist);
            path.pop_back();
        }
    } else {
        for (int i=0; i<graph[path.back()].size(); ++i) {
            int v = graph[path.back()][i];
            if (s_dist[v][path.front()] + path.size() > 7) continue;
            if (path.size() < 3 && path.front() == v) continue;
            if (v < path.front()) continue;
            for (int i=1; i<path.size(); ++i) {
                if (path[i] == v) continue;
            }

            if (find(path.begin()+1, path.end(), v) == path.end()) {
                path.emplace_back(v);
                brute_dfs(path, n, graph, result, s_dist);
                path.pop_back();
            }
        }
    }
}

vector<vector<vector<vector<int>>>> brute_search(int n, vector<vector<int>>& graph, vector<vector<int>>& s_dist) {
    vector<int> path;
    vector<vector<vector<vector<int>>>> result(n, vector<vector<vector<int>>>(5));
    brute_dfs(path, n, graph, result, s_dist);
    return result;
}

vector<vector<int>> bfs_shortest_path(int n, vector<vector<int>>& graph) {
    vector<vector<int>> dist(n, vector<int>(n, -1));
    vector<int> visit(n, -1);
    queue<int> to_visit;
    int u, v;
    for (int i=0; i<n; ++i) {
        dist[i][i] = 0; visit[i] = i;
        while (!to_visit.empty()) to_visit.pop();
        to_visit.push(i);
        while (!to_visit.empty()) {
            u = to_visit.front();
            to_visit.pop();
            for (int j=0; j<graph[u].size(); ++j) {
                v = graph[u][j];
                if (visit[v] != i) {
                    visit[v] = i;
                    dist[i][v] = dist[i][u] + 1;
                    if (dist[i][v] < 7) to_visit.push(v);
                }
            }
        }
    }
    return dist;
}



unordered_map<int, int> solve(vector<vector<int>>& data) {
    unordered_map<int, int> mapper = normalize(data);
    vector<vector<int>> graph = data_to_graph(mapper.size(), data);
    vector<vector<int>> s_dist = bfs_shortest_path(mapper.size(), graph);
    global_result = brute_search(mapper.size(), graph, s_dist);
    return mapper;
}

bool compare_between_path(int x, int y) {
    vector<int>& a = global_result[global_x][global_y][x];
    vector<int>& b = global_result[global_x][global_y][y];
    
    for (int i=0; i<a.size(); ++i) {
        if (a[i] != b[i]) {
            return a[i] < b[i];
        }
    }

    return false;
}









// 直接返回graph，先hash，在排序，在重新洗牌
vector<vector<int>> read_input() {
    ifstream infile(INPUT_PATH);
    
    if (!infile) {
        cout << "打开训练文件失败" << endl;
        exit(0);
    }

    vector<vector<int>> data;

    while (infile) {
        string line;
        char comma;
        int x, y, value;
        getline(infile, line);
        if (line.size() == 0) break;
        stringstream sin(line);
        sin >> x >> comma >> y >> comma >> value;

        vector<int> item;
        item.emplace_back(x);
        item.emplace_back(y);
        item.emplace_back(value);
        data.emplace_back(item);
    }
    infile.close();

    return data;
}

void write_output(unordered_map<int, int>& mapper) {
    ofstream fout(OUTPUT_PATH);
    if (!fout.is_open()) {
        cout << "打开模型参数文件失败" << endl;
    }

    fout << global_ret_size << endl;

    for (int y=0; y<5; ++y) {
        for (int x=0; x<global_result.size(); ++x) {
            vector<vector<int>>& local_result = global_result[x][y];
            if (local_result.size() == 0) continue;
            for (int i=0; i<local_result.size(); ++i) {
                vector<int> &path = local_result[i];
                for (int j=0; j<path.size(); ++j) {
                    fout << mapper[path[j]] << (j == path.size() - 1 ? '\n' : ',');
                }
            }
        }
    }

    fout.close();
}