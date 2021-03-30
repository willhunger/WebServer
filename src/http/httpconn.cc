#include "httpconn.h"
const char* HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
bool HttpConn::isET;

HttpConn::HttpConn() :
    _fd(-1),
    _isClose(true)
{
        _addr = {0};
}

HttpConn::~HttpConn()
{
    closeConn();
}

void HttpConn::init(int sockfd, const sockaddr_in& addr)
{
    assert(sockfd > 0);
    userCount++;
    _addr = addr;
    _fd = sockfd;
    _writeBuffer.retrieveAll();
    _readBuffer.retrieveAll();
    _isClose = false;
}

void HttpConn::closeConn()
{
    _response.unmapFile();
    if (_isClose == false)
    {
        _isClose = true;
        --userCount;
        close(_fd);
    }
}

int HttpConn::getFd() const
{
    return _fd;
}

int HttpConn::getPort() const
{
    return _addr.sin_port;
}

const char* HttpConn::getIP() const
{
    return inet_ntoa(_addr.sin_addr);
}

sockaddr_in HttpConn::getAddr() const
{
    return _addr;
}

ssize_t HttpConn::read(int* _errno)
{
    ssize_t len = -1;
    do
    {
        len = _readBuffer.readfd(_fd, _errno);
        if (len <= 0)
            break;
    } while(isET);
    return len;
}

ssize_t HttpConn::write(int* _errno)
{
    ssize_t len = -1;
    do
    {
        len = writev(_fd, _iov, _iovCnt);
        if (len <= 0) 
        {
            *_errno = errno;
            break;
        }
        if (_iov[0].iov_len + _iov[1].iov_len == 0)
        {
            break;
        }
        else if (static_cast<size_t>(len) > _iov[0].iov_len)
        {
            _iov[1].iov_base = (uint8_t*)_iov[1].iov_base + (len - _iov[0].iov_len);
            _iov[1].iov_len -= (len - _iov[0].iov_len);
            if (_iov[0].iov_len)
            {
                _writeBuffer.retrieveAll();
                _iov[0].iov_len = 0;
            }
        }
        else
        {
            _iov[0].iov_base = (uint8_t*)_iov[0].iov_base + len;
            _iov[0].iov_len -= len;
            _writeBuffer.retrieve(len);
        }
    } while (isET || toWriteBytes() > 10240);
    return len;
}

bool HttpConn::process()
{
    _request.init();
    if (_readBuffer.readableBytes() <= 0)
    {
        return false;
    }
    else if (_request.parse(_readBuffer))
    {
        _response.init(srcDir, _request.path(), _request.isKeepAlive(), 200);
    }
    else
    {
        _response.init(srcDir, _request.path(), false, 400);
    }

    _response.response(_writeBuffer);
    _iov[0].iov_base = const_cast<char*>(_writeBuffer.peek());
    _iov[0].iov_len = _writeBuffer.readableBytes();
    _iovCnt = 1;

    if (_response.fileLen() > 0 && _response.file())
    {
        _iov[1].iov_base = _response.file();
        _iov[1].iov_len = _response.fileLen();
        _iovCnt = 2;
    }

    return true;
}