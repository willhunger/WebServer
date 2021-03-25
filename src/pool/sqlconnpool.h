#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <assert.h>
#include <semaphore.h>
#include <mysql/mysql.h>

class sqlConnPool 
{
public:
    static sqlConnPool* instance();

    MYSQL *getConn();

    void freeConn(MYSQL* conn);

    int getFreeConnCount();

    void init(const char* host, int port,
              const char* user, const char* pwd,
              const char* dbName, int connSize);
    
    void closePool();

private:
    sqlConnPool();
    ~sqlConnPool();

    int maxConn;

    std::queue<MYSQL *> connQueue;
    std::mutex mtx;
    sem_t semId;
};

#endif // !SQLCONNPOOL_H