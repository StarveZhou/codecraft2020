#include <bits/stdc++.h>

using namespace std;

int main() {
    int now = 0;
    for (int i=0; i<280000 / 7; ++i) {
        for (int j=0; j<6; ++j) {
            cout << (now + j) * 5 << "," << (now + j + 1) * 5 << "," << 12121 << endl;
        }
        cout << (now + 6) * 5 << "," << (now) * 5 << "," << 12121 << endl;
        now += 7;
    }
}