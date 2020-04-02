#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <tuple>
#include <cstdlib>
#include <unordered_map>
#include <unordered_set>


#include "utils.h"

using namespace std;


// ----------------------------- MACRO ---------------------------------------------

#define TEST

#ifdef TEST

#include <ctime>
#include <algorithm>

clock_t start_time, end_time;

#else

#define INPUT_PATH "/data/test_data.txt"
#define OUTPUT_PATH "/projects/student/result.txt"
#endif

// ------------------------------ DECLARATION ---------------------------------------

//vector<vector<int>> read_input();
//
//void write_output(vector<vector<int>> ret);
#define MAX_LENGTH 7
#define MIN_LENGTH 3
typedef vector<vector<int>> Graph;
typedef vector<vector<int>> Paths;
typedef vector<int> Path;

struct Mapping
{
    unordered_map<int, int> origin_to_index;
    unordered_map<int, int> index_to_origin;
};

Mapping index_mapping(vector<vector<int>> &data)
{
    Mapping mapping = Mapping();
    vector<int> all_index;
    int count = 0;
    for (auto line:data)
    {
        for (int i = 0; i < 2; i++)
        {
            if (mapping.origin_to_index.find(line[i]) == mapping.origin_to_index.end())
            {
                all_index.emplace_back(line[i]);
                mapping.origin_to_index[line[i]] = count;
                count++;
            }
        }
    }
    sort(all_index.begin(), all_index.end());
    for (int i=0; i < all_index.size(); i++)
    {
        mapping.origin_to_index[all_index[i]] = i;
        mapping.index_to_origin[i] = all_index[i];
    }
    return mapping;
}

Graph build_graph(vector<vector<int>> &data, Mapping &mapping)
{
    int node_num = mapping.origin_to_index.size();
    Graph graph(node_num);

    for (auto line: data)
    {
        int p_index = mapping.origin_to_index[line[0]],
                q_index = mapping.origin_to_index[line[1]];
        graph[p_index].emplace_back(q_index);
    }
    return graph;
}

void dfs(Path &path, unordered_set<int> &path_nodes, Graph &graph, Paths &result)
{
    if (path.size() > MAX_LENGTH)
        return;
    int source_node = path.front();
    int current_node = path.back();

    for (auto i : graph[current_node])
    {
        if (i == source_node)
        {
            if (path.size() > MIN_LENGTH - 1)
                result.emplace_back(path);
        }
        else if (i > source_node && path_nodes.find(i) == path_nodes.end())
        {
            path.emplace_back(i);
            path_nodes.insert(i);
            dfs(path, path_nodes, graph, result);
            path_nodes.erase(i);
            path.pop_back();
        }

    }
}

bool cmp_path(Path &a, Path &b)
{
    if (a.size() != b.size())
        return a.size() < b.size();
    else
        for (int i = 0; i < a.size(); i++)
            if (a[i] != b[i])
                return a[i] < b[i];
}

Paths solve(vector<vector<int>> &data)
{
    Paths result;
    Mapping mapping = index_mapping(data);
    Graph graph = build_graph(data, mapping);

    Path path;
    unordered_set<int> path_nodes;


    for (int i = 0; i < graph.size(); i++)
    {
        path.emplace_back(i);
        path_nodes.insert(i);
        dfs(path, path_nodes, graph, result);
        path_nodes.erase(i);
        path.pop_back();
    }

    for (auto &x : result)
        for (int &i : x)
            i = mapping.index_to_origin[i];

    sort(result.begin(), result.end(), cmp_path);
    return result;
}


// ------------------------------ MAIN BODY -----------------------------------------

int main()
{
#ifdef TEST
    start_time = clock();
#endif

    vector<vector<int>> data = read_input();
    Paths result = solve(data);
    write_output(result);

#ifdef TEST
    end_time = clock();
    cout << "running: " << ((double) (end_time - start_time)) / CLOCKS_PER_SEC << endl;
#endif
    return 0;
}

// ------------------------------ FUNC BODY -----------------------------------------




