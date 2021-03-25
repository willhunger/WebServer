#include "heaptimer.h"

void HeapTimer::swapNode(size_t i, size_t j)
{
    assert(i >= 0 && i < heap.size());
    assert(j >= 0 && j < heap.size());
    std::swap(heap[i], heap[j]);
    ref[heap[i].id] = i;
    ref[heap[j].id] = j;
}

void HeapTimer::heapifyUp(size_t i)
{
    assert(i >= 0 && i < heap.size());
    size_t j = (i - 1) / 2;
    while (j >= 0)
    {
        if (heap[j] < heap[i])
            break;
        swapNode(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

bool HeapTimer::heapifyDown(size_t index, size_t n)
{
    assert(index >= 0 && index < heap.size());
    assert(n >= 0 && n <= heap.size());
    size_t i = index;
    size_t j = i * 2 + 1;
    while (j < n) 
    {
        if (j + 1 < n && heap[j + 1] < heap[j]) ++j;
        if (heap[i] < heap[j]) break;
        swapNode(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}


void HeapTimer::remove(size_t index)
{
    assert(!heap.empty() && index >= 0 && index < heap.size());
    size_t i = index;
    size_t n = heap.size() - 1;
    assert(i <= n);
    if (i < n)
    {
        swapNode(i, n);
        if (!heapifyDown(i, n))
            heapifyUp(i);
    }
    ref.erase(heap.back().id);
    heap.pop_back();
}

void HeapTimer::add(int id, int timeOut, const timeOutCallBack& callback)
{
    assert(id >= 0);
    size_t i;
    if (ref.find(id) == end(ref))
    {
        i = heap.size();
        ref[id] = i;
        heap.emplace_back(id, Clock::now() + millisecond(timeOut), callback);
        heapifyUp(i);
    }
    else 
    {
        i = ref[id];
        heap[i].expires = Clock::now() + millisecond(timeOut);
        heap[i].callback = callback;
        if (!heapifyDown(i, heap.size()))
            heapifyUp(i);
    }
}


void HeapTimer::doWork(int id)
{
    if (heap.empty() || ref.find(id) == end(ref))
        return;
    size_t i = ref[id];
    auto node = heap[i];
    node.callback();
    remove(i);
}

void HeapTimer::tick()
{
    if (heap.empty())
    {
        return;
    }
    while (!heap.empty())
    {
        auto node = heap.front();
        if (std::chrono::duration_cast<millisecond>(node.expires - Clock::now()).count() > 0)
        {
            break;
        }
        node.callback();
        pop();
    }
}

void HeapTimer::pop()
{
    assert(!heap.empty());
    remove(0);
}

void HeapTimer::clear()
{
    ref.clear();
    heap.clear();
}

int HeapTimer::getNextTick()
{
    tick();
    size_t res = -1;
    if (!heap.empty())
    {
        res = std::chrono::duration_cast<millisecond>(heap.front().expires - Clock::now()).count();
        if (res < 0)
            res = 0;
    }
    return res;
}

bool HeapTimer::empty()
{
    return heap.empty();
}

void HeapTimer::adjust(int id, int newExpires)
{
    assert(!heap.empty() && ref.find(id) != end(ref));
    heap[ref[id]].expires = Clock::now() + millisecond(newExpires);
    heapifyDown(ref[id], heap.size());
}