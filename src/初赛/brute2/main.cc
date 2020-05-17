#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <tuple>
#include <cstdlib>
using namespace std;

// ----------------------------- MACRO ---------------------------------------------

#define TEST

#ifdef TEST

#include <time.h>
clock_t start_time, end_time;
#define INPUT_PATH "../../resources/test_data.txt"
#define OUTPUT_PATH "test_output.txt"

#else

#define INPUT_PATH "/data/test_data.txt"
#define OUTPUT_PATH "/projects/student/result.txt"
#endif

// ------------------------------ DECLARATION ---------------------------------------

vector<vector<int>> read_input();
void write_output(vector<vector<int>> ret);

// ------------------------------ MAIN BODY -----------------------------------------

struct GNode{
	vector<int> adj;
	vector<int> value;
};

bool available[6010] = {0}; 
bool dead[6010] = {0}; 
bool visit[6010] = {0};
bool visit_global[6010] = {0};
vector<int> path;
vector<vector<int>> all_path;

GNode* build_graph(vector<vector<int>> data){
	GNode* graph = new GNode[6010];
	int i, j;
	for(i=0;i<data.size();i++){
		graph[data[i][0]].adj.emplace_back(data[i][1]);
		graph[data[i][0]].value.emplace_back(data[i][2]);
		available[data[i][0]] = 1;
		available[data[i][1]] = 1;
	}
	return graph;
}

void dfs(GNode* graph, int index){
	int i=0;
	if(visit[index]==1){
		while(path[i]!=index) i++;
		vector<int> temp(path.begin()+i, path.end());
		all_path.emplace_back(temp);
		
		return;
	}
	if(visit_global[index]==1) return;  // 有待商榷，理论上应该每次dfs完了以后再更新global 
	
	visit_global[index] = 1;
	
	visit[index] = 1;
	path.emplace_back(index);
	
	int next_index;
	bool do_something = 0;
	for(i=0;i<graph[index].adj.size();i++){
		next_index = graph[index].adj[i];
		if(available[next_index] && !dead[next_index]) {
			do_something = 1;
			dfs(graph, next_index);
		}
	}
	if(!do_something) dead[index] = 1;  // 路过式剪枝 
	
	path.pop_back();
	visit[index] = 0;
	return;
}



int main() {
#ifdef TEST
    start_time = clock();
#endif
    
    vector<vector<int>> data = read_input();
    
    GNode* graph = build_graph(data); 
    int i;
    for(i=0;i<6010;i++){
    	if(!visit_global[i] && available[i]) dfs(graph, i);
	}
    
    write_output(all_path);

#ifdef TEST
    end_time = clock();
    cout << "running: " << ((double)(end_time - start_time)) / CLOCKS_PER_SEC << endl;
#endif
    return 0;
}

// ------------------------------ FUNC BODY -----------------------------------------

vector<vector<int>> read_input() {
    ifstream infile(INPUT_PATH);
    
    if (!infile) {
        cout << "??????!" << endl;
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

void write_output(vector<vector<int>> ret) {
    ofstream fout(OUTPUT_PATH);
    if (!fout.is_open()) {
        cout << "????" << endl;
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
