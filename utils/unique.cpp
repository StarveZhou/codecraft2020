#include <iostream>
#include <unordered_set>
#include <stdio.h>

using namespace std;

int main() {
    int x, n; char ch;
    scanf("%d", &n);
    unordered_set<int> uset;
    for (int i=0; i<n; ++i) {
        while (true) {
            scanf("%d", &x);
            uset.insert(x);
            ch = getchar();
            if (ch != ',') break;
        }
        if (i == n-1) printf("finish\n");
    }
    printf("%d\n", uset.size());
}