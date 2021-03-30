#include <iostream>
#include <memory>
#include <unistd.h>
#include "threadpool.h"

static void task() {
    for (int i = 1; i < 100; ++i) {
        if (rand() % 2) {
            sleep(1);
        }
        std::cout << i << std::endl;
    }
}

int main()
{
    ThreadPool tp(8);
    for (int i = 0; i < 8; ++i) {
        tp.addTask(task);
    }
    while(1) {
        
    }
}