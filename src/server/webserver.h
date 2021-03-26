#ifndef WEBWERVER_H
#define WEBWERVER_H

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../timer/heaptimer.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"
#include "../pool/threadpool.h"
#include "../http/httpconn.h"

class WebServer
{
public:
    WebServer(
        int port, int trigMode, int timeoutMs, bool optLinger,
        int sqlPort, const char* sqlUser, const char* sqlPwd,
        const char* dbName, int connPoolNum, int threadNum
    );

    ~WebServer();

    void start();

private:
    bool initsocket();
    void initEventMode(int trigMode);
    void addClient(int fd, sockaddr_in addr);

    void dealListen();
    void dealWrite(HttpConn* client);
    void dealRead(HttpConn* client);

    void sendError(int fd, const char* info);
    void extendTimer(HttpConn* client);
    void closeConn(HttpConn* client);

    void OnRead(HttpConn* client);
    void OnWrite(HttpConn* client);    
    void OnProcess(HttpConn* client);

    static const int MAXFD = 65536;

    static int setFdNonblock(int fd);

    int _port;
    int _optLinger;
    int _timeoutMs;
    int _isClose;
    int _listenFd;
    char* _srcDir;

    uint32_t _listenEvent;
    uint32_t _connEvent;

    std::unique_ptr<HeapTimer> _timer;
    std::unique_ptr<ThreadPool> _threadpool;
    std::unique_ptr<Epoller> _epoller;
    std::unordered_map<int, HttpConn> _users;
};

#endif // !WEBWERVER_H