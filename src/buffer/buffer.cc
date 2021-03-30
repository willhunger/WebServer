#include "buffer.h"

Buffer::Buffer(int initBufferSize): buffer(initBufferSize), readPos(0), writePos(0) 
{
}

size_t Buffer::readableBytes() const
{
    return writePos - readPos;
}

size_t Buffer::writableBytes() const
{
    return buffer.size() - writePos;
}

size_t Buffer::appendableBytes() const
{
    return readPos;
}

const char* Buffer::peek() const
{
    return beginPtr() + readPos;
}

void Buffer::retrieve(size_t len)
{
    assert(len <= readableBytes());
    readPos += len;
}

void Buffer::retrieveUntil(const char* end)
{
    assert(peek() <= end);
    retrieve(end - peek());
}

void Buffer::retrieveAll()
{
    bzero(&buffer[0], buffer.size());
    readPos = 0;
    writePos = 0;    
}

std::string Buffer::retrieveAllToStr()
{
    std::string str(peek(), readableBytes());
    retrieveAll();
    return str;
}

const char* Buffer::beginWriteConst() const
{
    return beginPtr() + writePos;
}

char* Buffer::beginWrite()
{
    return beginPtr() + writePos;
}

void Buffer::append(const std::string& str)
{
    append(str.data(), str.size());
}

void Buffer::append(const char* str, size_t len)
{
    assert(str);
    checkWritable(len);
    std::copy(str, str + len, beginWrite());
    haswrittenBytes(len);
}

void Buffer::append(const void* data, size_t len)
{
    assert(data);
    append(static_cast<const char*>(data), len);
}

void Buffer::append(const Buffer& _buff)
{
    append(_buff.peek(), _buff.readableBytes());
}

void Buffer::haswrittenBytes(size_t len)
{
    writePos += len;
}

void Buffer::checkWritable(size_t len)
{
    if (writableBytes() < len) 
        allocate(len);
    assert(writableBytes() >= len);
}

ssize_t Buffer::readfd(int fd, int* _errno)
{
    char buff[65535];
    struct iovec iov[2];
    const size_t writable = writableBytes();
    iov[0].iov_base = beginPtr() + writePos;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if (len < 0)
    {
        *_errno = errno;
    }
    else if (static_cast<size_t>(len) <= writable)
    {
        writePos += len;
    }
    else
    {
        writePos = buffer.size();
        append(buff, len - writable);
    }
    
    return len;
}

ssize_t Buffer::writefd(int fd, int* _errno)
{
    size_t readSize = readableBytes();
    ssize_t len = write(fd, peek(), readSize);
    if (len < 0)
    {
        *_errno = errno;
        return len;
    }
    readPos += len;
    return len;
}

char* Buffer::beginPtr()
{
    return &*buffer.begin();
}

const char* Buffer::beginPtr() const
{
    return &*buffer.begin();
}

void Buffer::allocate(size_t len)
{
    if (writableBytes() + appendableBytes() < len)
    {
        buffer.resize(writePos + len + 1);
    }
    else
    {
        size_t readable = readableBytes();
        std::copy(beginPtr() + readPos, beginPtr() + writePos, beginPtr());
        readPos = 0;
        writePos = readPos + readable;
        assert(readable == readableBytes());
    }
}