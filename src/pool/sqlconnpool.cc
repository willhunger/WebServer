#include "sqlconnpool.h"

sqlConnPool::sqlConnPool()
{
}

sqlConnPool* sqlConnPool::instance()
{
    static sqlConnPool connPool;
    return &connPool;
}

void sqlConnPool::init(const char* host, int port,
            const char* user, const char* pwd,
            const char* dbName, int connSize)
{
    assert(connSize > 0);
    for (int i = 0; i < connSize; ++i)
    {
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);
        if (!sql) 
        {
            assert(sql);
        }
        sql = mysql_real_connect(sql, host, 
                                user, pwd, 
                                dbName, port, nullptr, 0);
        if (!sql)
        {
            return;
        }
        connQueue.push(sql);
    }
    maxConn = connSize;
    sem_init(&semId, 0, maxConn);
}

MYSQL* sqlConnPool::getConn()
{
    MYSQL *sql = nullptr;
    if (connQueue.empty()) {
        return nullptr;
    }
    sem_wait(&semId);
    {
        std::lock_guard<std::mutex> locker(mtx);
        sql = connQueue.front();
        connQueue.pop();
    }
    return sql;
}

void sqlConnPool::freeConn(MYSQL* sql)
{
    assert(sql);
    std::lock_guard<std::mutex> locker(mtx);
    connQueue.push(sql); 
    sem_post(&semId);
}

void sqlConnPool::closePool()
{
    std::lock_guard<std::mutex> locker(mtx);
    while (!connQueue.empty())
    {
        auto item = connQueue.front();
        connQueue.pop();
        mysql_close(item);
    }
    mysql_library_end();
}

int sqlConnPool::getFreeConnCount()
{
    std::lock_guard<std::mutex> locker(mtx);
    return connQueue.size();
}

sqlConnPool::~sqlConnPool()
{
    closePool();
}