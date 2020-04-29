//
// Created by DavidWang on 2020/4/2.
//

#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;

#define INPUT_PATH "../../../resources/test_data.txt"
#define OUTPUT_PATH "test_output.txt"


vector<vector<int>> read_input()
{
    ifstream infile(INPUT_PATH);

    if (!infile)
    {
        cout << "打开训练文件失败" << endl;
        exit(0);
    }

    vector<vector<int>> data;

    while (infile)
    {
        string line;
        char comma;
        int x, y, value;
        getline(infile, line);
        if (line.empty()) break;
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

void write_output(vector<vector<int>> ret)
{
    ofstream fout(OUTPUT_PATH);
    if (!fout.is_open())
    {
        cout << "打开模型参数文件失败" << endl;
    }

    fout << ret.size() << endl;
    for (auto &item : ret)
    {
        for (int j = 0; j < item.size(); ++j)
        {
            fout << item[j] << (j == item.size() - 1 ? '\n' : ',');
        }
    }

    fout.close();
}

#endif //TEST_UTILS_H
