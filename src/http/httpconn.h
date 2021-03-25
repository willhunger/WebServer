#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <cstdlib>
#include <cassert>
#include <error.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include "../pool/sqlconnRAII.cc"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn
{
public:
    HttpConn();

    ~HttpConn();

    void init(int sockfd, const sockaddr_in& addr);

    ssize_t read(int* _errno);

    ssize_t write(int* _errno);

    void closeConn();

    int getFd() const;

    int getPort() const;

    const char* getIP() const;

    sockaddr_in getAddr() const;

    bool process();

    int toWriteBytes()
    {
        return _iov[0].iov_len + _iov[1].iov_len;
    }

    bool isKeepAlive() const
    {
        return _request.isKeepAlive();
    }

    static bool isET;
    static const char* srcDir;
    static std::atomic<int> userCount;

private:
    int _fd;
    struct sockaddr_in _addr;

    bool _isClose;
    int _iovCnt;
    struct iovec _iov[2];

    Buffer _readBuffer;
    Buffer _writeBuffer;

    HttpRequest _request;
    HttpResponse _response;
};

#endif // !HTTP_CONN_H