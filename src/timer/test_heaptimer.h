#include "heaptimer.cc"
#include <bits/stdc++.h>
using namespace std;

int main() {
    HeapTimer hp;
    std::function<void()> cb= []() {
        cout << rand() % 8 << endl;
    };
    const int n = 100;
    for (int i = 0; i < n; ++i) {
        hp.add(i, i * 100, cb);
    }
    while (!hp.empty())
    {
        hp.getNextTick();
    }
}