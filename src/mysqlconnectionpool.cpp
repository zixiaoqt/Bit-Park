#include "mysqlconnectionpool.h"
#include "log.h"

namespace parkingserver {
MysqlConnectionPool::MysqlConnectionPool(const std::string& db_server,
                                         const std::string& db_name,
                                         const std::string& db_user,
                                         const std::string& db_password,
                                         unsigned int max_idle_time)
        : db_server_(db_server)
        , db_name_(db_name)
        , db_user_(db_user)
        , db_password_(db_password)
        , max_idle_time_(max_idle_time){
}

MysqlConnectionPool::~MysqlConnectionPool() {
    clear();
}

mysqlpp::Connection* MysqlConnectionPool::grab() {
    return mysqlpp::ConnectionPool::grab();
}

void MysqlConnectionPool::release(const mysqlpp::Connection* pc) {
    mysqlpp::ConnectionPool::release(pc);
}

mysqlpp::Connection* MysqlConnectionPool::create() {
    auto conn = new mysqlpp::Connection(true);
    conn->set_option(new mysqlpp::SetCharsetNameOption("utf8"));
    conn->connect(
        db_name_.empty() ? 0 : db_name_.c_str(),
        db_server_.empty() ? 0 : db_server_.c_str(),
        db_user_.empty() ? 0 : db_user_.c_str(),
        db_password_.empty() ? "" : db_password_.c_str());
	log()->trace("Create a new connection to mysql");
    return conn;
}

void MysqlConnectionPool::destroy(mysqlpp::Connection* cp) {
    delete cp;
	log()->trace("Delete a connection to mysql");
}

}
