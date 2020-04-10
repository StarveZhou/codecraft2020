#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>

#include <vector>
#include <map>

#include <algorithm>
#include <string>

using namespace std;

// ----------------------------- MACRO ---------------------------------------------

#ifdef TEST

#include <time.h>
clock_t start_time, end_time;
// #define INPUT_PATH "resources/test_data.txt"
// #define INPUT_PATH "gen_data.txt"
// #define INPUT_PATH "resources/pre_test.txt"
#define INPUT_PATH  "resources/data1.txt"
// #define OUTPUT_PATH "test_output.txt"
#define OUTPUT_PATH "o_brute_output.txt"

#else

#define INPUT_PATH "/data/test_data.txt"
#define OUTPUT_PATH "/projects/student/result.txt"
#endif

#define MAX_DEPTH         7

// ------------------------------ DECLARATION ---------------------------------------

#ifdef TEST

void say_out(string s);

#endif

vector<vector<int>> read_input();
void write_output(vector<vector<int>>& ret);

vector<vector<int>> solve(vector<vector<int>>& data);
map<int, int> normalize(vector<vector<int>>& data);
vector<vector<int>> data_to_graph(int n, vector<vector<int>>& data);
vector<vector<int>> brute_search(int n, vector<vector<int>>& graph);
void brute_dfs(vector<int>& path, int n, vector<vector<int>>& graph, vector<vector<int>>& result);
bool compare_between_path(vector<int>& a, vector<int>& b);

// ------------------------------ MAIN BODY -----------------------------------------

int main() {
#ifdef TEST
    start_time = clock();
#endif
    
    vector<vector<int>> data = read_input();
    vector<vector<int>> ret = solve(data);
#ifdef TEST
    end_time = clock();
    cout << "seaching: " << ((double)(end_time - start_time)) / CLOCKS_PER_SEC << "s" << endl;
#endif
    sort(ret.begin(), ret.end(), compare_between_path);

    write_output(ret);
#ifdef TEST
    end_time = clock();
    cout << "running: " << ((double)(end_time - start_time)) / CLOCKS_PER_SEC << "s" << endl;
#endif
    return 0;
}

// ------------------------------ FUNC BODY -----------------------------------------

map<int, int> normalize(vector<vector<int>>& data) {
    vector<int> all_node;
    for (int i=0; i<data.size(); ++i) {
        all_node.emplace_back(data[i][0]);
        all_node.emplace_back(data[i][1]);
    }

    sort(all_node.begin(), all_node.end());
    vector<int>::iterator new_end = unique(all_node.begin(), all_node.end());
    all_node.erase(new_end, all_node.end());

    map<int, int> mapper, rev_mapper;
    for (int i=0; i<all_node.size(); ++i) {
        mapper[i] = all_node[i];
        rev_mapper[all_node[i]] = i;
    }

    for (int i=0; i<data.size(); ++i) {
        data[i][0] = rev_mapper[data[i][0]];
        data[i][1] = rev_mapper[data[i][1]];
    }
    // cout << "unique: " << mapper.size() << endl << flush;
    return mapper;
}

vector<vector<int>> data_to_graph(int n, vector<vector<int>>& data) {
    vector<vector<int>> graph(n);
    for (int i=0; i<data.size(); ++i) {
        graph[data[i][0]].emplace_back(data[i][1]);
    }
    return graph;
}

void brute_dfs(vector<int>& path, int n, vector<vector<int>>& graph, vector<vector<int>>& result) {
    // string out_str = "";
    // for (int i=0; i<path.size(); ++i) {
    //     out_str += to_string(path[i]+1) + (i == path.size()-1 ? "" : ", ");
    // }

    // say_out("into path: " + to_string(path.size()) + ", back: " + out_str);
    if (path.size() > 3 &&  path.front() == path.back()) {
        path.pop_back();
        result.push_back(path);
        path.push_back(path.front());
        return;
    }

    if (path.size() > MAX_DEPTH) {
        return;
    }

    if (path.size() == 0) {
        for (int i=0; i<n; ++i) {
            path.emplace_back(i);
            brute_dfs(path, n, graph, result);
            path.pop_back();
        }
    } else {
        for (int i=0; i<graph[path.back()].size(); ++i) {
            int v = graph[path.back()][i];

            bool flag = !(path.size() < 3 && path.front() == v);
            flag &= v >= path.front();
            for (int i=1; i<path.size() && flag; ++i) {
                flag &= path[i] != v;
            }
            if (!flag) continue;

            if (find(path.begin()+1, path.end(), v) == path.end()) {
                path.emplace_back(v);
                brute_dfs(path, n, graph, result);
                path.pop_back();
            }
        }
    }
}

vector<vector<int>> brute_search(int n, vector<vector<int>>& graph) {
    vector<int> path;
    vector<vector<int>> result;
    brute_dfs(path, n, graph, result);
    return result;
}

bool compare_between_path(vector<int>& a, vector<int>& b) {
    if (a.size() != b.size()) {
        return a.size() < b.size();
    } else {
        for (int i=0; i<a.size(); ++i) {
            if (a[i] != b[i]) {
                return a[i] < b[i];
            }
        }
    }
    return false;
}

vector<vector<int>> solve(vector<vector<int>>& data) {
    // say_out("data size: " + to_string(data.size()));
    map<int, int> mapper = normalize(data);
    // say_out("sizeof mapper: " + to_string(mapper.size()));
    vector<vector<int>> graph = data_to_graph(mapper.size(), data);
    // for (int i=0; i<graph.size(); ++i) {
    //     string str = "";
    //     for (int j=0; j<graph[i].size(); ++j) {
    //         str += to_string(graph[i][j] + 1) + (j == graph[i].size()-1 ? "" : ", ");
    //     }
    //     say_out(to_string(i+1) + ": " + str);
    // }
    // say_out("before search");
    vector<vector<int>> result = brute_search(mapper.size(), graph);
    // say_out("after search");
    for (int i=0; i<result.size(); ++i) {
        for (int j=0; j<result[i].size(); ++j) {
            result[i][j] = mapper[result[i][j]];
        }
    }
    return result;
}



















void say_out(string s) {
    cout << "[SAY] " << s << endl << flush;
}

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

void write_output(vector<vector<int>>& ret) {
    ofstream fout(OUTPUT_PATH);
    if (!fout.is_open()) {
        cout << "打开模型参数文件失败" << endl;
    }

    fout << ret.size() << endl;
    for (int i=0; i<ret.size(); ++i) {
        vector<int> &item = ret[i];
        for (int j=0; j<item.size(); ++j) {
            fout << item[j] << (j == item.size()-1 ? '\n' : ',');
        }
    }

    fout.close();
}