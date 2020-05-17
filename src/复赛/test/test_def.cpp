#include <iostream>

using namespace std;

#define def_fire(tid) \
int a_##tid = 0; \
void fire_##tid(int x) { \
    x += a_##tid; \
    cout << x << endl; \
}

def_fire(0)
def_fire(1)
def_fire(2)


int main() {
    fire_0(0);
    fire_1(1);
    fire_2(2);
}