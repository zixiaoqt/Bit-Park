#ifndef _PARKINGSERVER_CONNECTION_POOL_
#define _PARKINGSERVER_CONNECTION_POOL_
#include <mysql++.h>

namespace parkingserver {
class MysqlConnectionPool : public mysqlpp::ConnectionPool {
  public:
    MysqlConnectionPool(const std::string& db_server,
                        const std::string& db_name,
                        const std::string& db_user,
                        const std::string& db_password,
                        unsigned int max_idle_time);
    ~MysqlConnectionPool();
    mysqlpp::Connection* grab();
    void release(const mysqlpp::Connection* pc);
protected:
    mysqlpp::Connection* create();
    void destroy(mysqlpp::Connection* cp);
    unsigned int max_idle_time() {return max_idle_time_;}
private:
    static MysqlConnectionPool pool_;
    unsigned int max_idle_time_;
    std::string db_server_, db_name_, db_user_, db_password_;
};
}

#endif
