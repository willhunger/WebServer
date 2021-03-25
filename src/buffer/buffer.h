#ifndef BUFFER_H
#define BUFFER_H

#include <cstring>
#include <iostream>
#include <vector>
#include <atomic>
#include <cassert>
#include <unistd.h>
#include <sys/uio.h>

class Buffer 
{
public:
    Buffer(int initBufferSize = 1024);
    ~Buffer() = default;

    size_t writableBytes() const;
    size_t readableBytes() const;
    size_t appendableBytes() const;

    const char* peek() const;
    void checkWritable(size_t len);
    void haswrittenBytes(size_t len);

    void retrieve(size_t len);
    void retrieveUntil(const char* end);

    void retrieveAll();
    std::string retrieveAllToStr();

    const char* beginWriteConst() const;
    char* beginWrite();

    void append(const std::string& str);
    void append(const char* str, size_t len);
    void append(const void* data, size_t len);
    void append(const Buffer& _buff);

    ssize_t readfd(int fd, int* _errno);
    ssize_t writefd(int fd, int* _errno);

private:
    char* beginPtr();
    const char* beginPtr() const;
    void allocate(size_t len);

    std::vector<char> buffer;
    std::atomic<std::size_t> readPos;
    std::atomic<std::size_t> writePos;
};

#endif // !BUFFER_H