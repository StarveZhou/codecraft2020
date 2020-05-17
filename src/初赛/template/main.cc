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
#define INPUT_PATH "resources/test_data.txt"
#define OUTPUT_PATH "test_output.txt"

#else

#define INPUT_PATH "/data/test_data.txt"
#define OUTPUT_PATH "/projects/student/result.txt"
#endif

// ------------------------------ DECLARATION ---------------------------------------

vector<vector<int>> read_input();
void write_output(vector<vector<int>> ret);

// ------------------------------ MAIN BODY -----------------------------------------

int main() {
#ifdef TEST
    start_time = clock();
#endif
    
    vector<vector<int>> data = read_input();
    write_output(data);

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

void write_output(vector<vector<int>> ret) {
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