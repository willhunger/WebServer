#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <queue>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <chrono>
#include <assert.h>
#include <time.h>
#include <arpa/inet.h>

using timeOutCallBack = std::function<void()>;
using Clock = std::chrono::high_resolution_clock;
using millisecond = std::chrono::milliseconds;
using timestamp = Clock::time_point;

struct TimerNode 
{   
    TimerNode(int _id, timestamp _expires, timeOutCallBack _callback):
        id(_id), expires(_expires), callback(_callback) {}
        
    bool operator< (const TimerNode& rhs) const
    {
        return this->expires < rhs.expires;        
    }

    int id;
    timestamp expires;
    timeOutCallBack callback;
};



class HeapTimer
{
public:
    HeapTimer()
    {
        heap.reserve(64);
    }

    ~HeapTimer()
    {
        clear();
    }

    void adjust(int id, int newExpires);

    void add(int id, int timeOut, const timeOutCallBack& callback);

    void doWork(int id);

    void clear();

    void tick();

    void pop();

    int getNextTick();

    bool empty();
    
private:
    void remove(size_t index);

    void heapifyUp(size_t i);

    bool heapifyDown(size_t index, size_t n);

    void swapNode(size_t i, size_t j);

    std::vector<TimerNode> heap;

    std::unordered_map<int, size_t> ref;
};

#endif // !HEAP_TIMER_H

