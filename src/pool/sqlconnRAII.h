#ifndef SQL_CONN_RAII
#define SQL_CONN_RAII
#include "sqlconnpool.h"

class sqlConnRAII
{
public:
    sqlConnRAII(MYSQL** sql, sqlConnPool* connPool)
    {
        assert(connPool);
        *sql = connPool->getConn();
        _sql = *sql;
        _connPool = connPool;
    }

    ~sqlConnRAII()
    {
        if (_sql)
            _connPool->freeConn(_sql);
    }
private:
    MYSQL *_sql;
    sqlConnPool* _connPool;
};

#endif // !SQL_CONN_RAII