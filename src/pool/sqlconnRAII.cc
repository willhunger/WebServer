#ifndef SQL_CONN_RAII
#define SQL_CONN_RAII
#include "sqlconnpool.h"

class sqlConnRAII
{
public:
    sqlConnRAII(MYSQL** sql, sqlConnPool* _connPool)
    {
        assert(_connPool);
        *sql = _connPool->getConn();
        sql = *sql;
        connPool = _connPool;
    }

    ~sqlConnRAII()
    {
        if (sql)
            connPool->freeConn(sql);
    }
private:
    MYSQL *sql;
    sqlConnPool connPool;
};

#endif // !SQL_CONN_RAII