#include "webserver.h"

int WebServer::setFdNonblock(int fd)
{
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

WebServer::WebServer(
    int port, int trigMode, int timeoutMs, bool optLinger,
    int sqlPort, const char* sqlUser, const char* sqlPwd,
    const char* dbName, int connPoolNum, int threadNum
):
    _port(port),
    _optLinger(optLinger),
    _timeoutMs(timeoutMs),
    _isClose(false),
    _timer(new HeapTimer()),
    _threadpool(new ThreadPool(threadNum)),
    _epoller(new Epoller())
{
    _srcDir = getcwd(nullptr, 256);
    assert(_srcDir);
    strncat(_srcDir, "/resources", 16),
    HttpConn::userCount = 0;
    HttpConn::srcDir = _srcDir;
    sqlConnPool::instance()->init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);
    initEventMode(trigMode);
    if (!initsocket())
    {
        _isClose = true;
    }
}

WebServer::~WebServer()
{
    close(_listenFd);
    _isClose = true;
    free(_srcDir);
    sqlConnPool::instance()->closePool();
}

void WebServer::initEventMode(int trigMode)
{
    _listenEvent = EPOLLRDHUP;
    _connEvent = EPOLLONESHOT | EPOLLRDHUP;
    switch(trigMode)
    {
        case 0 :
            break;
        case 1 :
            _listenEvent |= EPOLLET;
            break;
        case 2 :
            _listenEvent |= EPOLLET;
            break;
        case 3 : 
            _listenEvent |= EPOLLET;
            _connEvent |= EPOLLET;
            break;
        default:
            _listenEvent |= EPOLLET;
            _connEvent |= EPOLLET;
            break;        
    }
    HttpConn::isET = (_connEvent & EPOLLET);
}

void WebServer::start()
{
    int timeMs = -1;
    while (!_isClose)
    {
        if (_timeoutMs > 0)
        {
            timeMs = _timer->getNextTick();
        }

        int eventCnt = _epoller->wait(timeMs);
        for (int i = 0; i < eventCnt; ++i)
        {
            int fd = _epoller->getEventFd(i);
            uint32_t events = _epoller->getEvents(i);
            if (fd == _listenFd)
            {
                dealListen();
            }
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                assert(_users.count(fd) > 0);
                closeConn(&_users[fd]);
            }
            else if (events & EPOLLIN)
            {
                assert(_users.count(fd) > 0);
                dealRead(&_users[fd]);
            }
            else if (events & EPOLLOUT)
            {
                assert(_users.count(fd) > 0);
                dealWrite(&_users[fd]);
            }
        }
    }
}

void WebServer::sendError(int fd, const char* info)
{
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    close(fd);
}

void WebServer::closeConn(HttpConn* client)
{
    assert(client);
    _epoller->delFd(client->getFd());
    client->closeConn();
}

void WebServer::addClient(int fd, sockaddr_in addr)
{
    assert(fd > 0);
    _users[fd].init(fd, addr);
    if (_timeoutMs > 0)
    {
        _timer->add(fd, _timeoutMs, std::bind(&WebServer::closeConn, this, &_users[fd]));
    }
    _epoller->addFd(fd, EPOLLIN | _connEvent);
    setFdNonblock(fd);
}


void WebServer::dealListen()
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do
    {
        int fd = accept(_listenFd, (struct sockaddr*)&addr, &len);
        if (fd < 0) 
            return;
        else if (HttpConn::userCount >= MAXFD)
        {
            sendError(fd, "server busy");
            return;
        }
        addClient(fd, addr);
    } while(_listenEvent & EPOLLET);
}

void WebServer::dealWrite(HttpConn* client)
{
    assert(client);
    extendTimer(client);
    _threadpool->addTask(std::bind(&WebServer::OnWrite, this, client));
}

void WebServer::dealRead(HttpConn* client)
{
    assert(client);
    extendTimer(client);
    _threadpool->addTask(std::bind(&WebServer::OnRead, this, client));
};

void WebServer::extendTimer(HttpConn* client)
{
    assert(client);
    if (_timeoutMs > 0)
    {
        _timer->adjust(client->getFd(), _timeoutMs);
    }
}

void WebServer::OnRead(HttpConn* client)
{
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);
    if (ret <= 0 && readErrno != EAGAIN)
    {
        closeConn(client);
        return;
    }
    OnProcess(client);
}

void WebServer::OnWrite(HttpConn* client)
{
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    
    if (client->toWriteBytes() == 0)
    {
        if (client->isKeepAlive())
        {
            OnProcess(client);
            return;
        }
    }
    else if (ret < 0)
    {
        if (writeErrno == EAGAIN)
        {
            _epoller->modFd(client->getFd(), _connEvent | EPOLLOUT);
            return;
        }
    }
    closeConn(client);
}

void WebServer::OnProcess(HttpConn* client)
{
    if (client->process())
    {
        _epoller->modFd(client->getFd(), _connEvent | EPOLLOUT);
    }
    else
    {
        _epoller->modFd(client->getFd(), _connEvent | EPOLLIN);
    }
}

bool WebServer::initsocket()
{
    int ret;
    struct sockaddr_in addr;
    if (_port > 65535 || _port < 1024)
    {
        std::cout << "port failed" << std::endl;
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(_port);
    struct linger optLinger;
    if (_optLinger)
    {
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }

    _listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenFd < 0)
    {
        std::cout << "socket() failed" << std::endl;
        return false;
    }

    ret = setsockopt(_listenFd, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if (ret < 0)
    {
        std::cout << "setsockopt() failed" << std::endl;
        close(_listenFd);
        return false;
    }

    int optval = 1;
    ret = setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if (ret < 0)
    {
        std::cout << "setsockopt() failed" << std::endl;
        close(_listenFd);
        return false;
    }

    ret = bind(_listenFd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0)
    {
        std::cout << "bind() failed" << std::endl;
        close(_listenFd);
        return false;
    }

    ret = listen(_listenFd, 6);
    if (ret < 0)
    {
        std::cout << "listen() failed" << std::endl;
        close(_listenFd);
        return false;
    }

    ret = _epoller->addFd(_listenFd, _listenEvent | EPOLLIN);
    if (ret == 0)
    {
        std::cout << "epoller failed" << std::endl;
        close(_listenFd);
        return false;
    }

    setFdNonblock(_listenFd);
    return true;
}
