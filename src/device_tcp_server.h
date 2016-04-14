#ifndef DEVICE_TCP_SERVER_H
#define DEVICE_TCP_SERVER_H

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <string>
#include <list>
#include <iostream>

namespace parkingserver {
namespace device_tcp_server {

using boost::asio::ip::tcp;
using boost::shared_ptr;
using std::string;
using std::list;
using std::pair;
using boost::mutex;

class client_error : public std::runtime_error
{
  public:
    client_error(const string& str) : std::runtime_error(str) {
        
    }
};

typedef shared_ptr<tcp::iostream> stream_ptr;
typedef shared_ptr<mutex> mtxptr;
typedef pair<stream_ptr, mtxptr> client_type;

client_type make_client(stream_ptr, mtxptr);

struct server
{
    boost::asio::io_service ios;		// 用来执行异步操作.须有一个io_service对象. io_service对象负责连接应用程序与操作系统的IO服务.
    shared_ptr<tcp::acceptor> acceptor; // 接收器用来侦听到来的链接请求.acceptor来接收client发来的连接。
};

typedef shared_ptr<server> server_pointer;


server_pointer make_server(unsigned short port);

typedef std::function<void (client_type)> new_client_notify;

void server_run(server_pointer ptr, new_client_notify notify);
void server_stop(server_pointer ptr);

bool client_is_valid(client_type c);
stream_ptr client_stream(client_type c);
mtxptr client_mutex(client_type c);
string client_address(client_type sock);
unsigned short client_port(client_type sock);
string client_send_recv(client_type sock, const string& message);

typedef shared_ptr<std::list<client_type>> clients_type;

clients_type make_clients();
client_type clients_find_client(clients_type clients, const string& ipaddr);
clients_type clients_add_client(clients_type clients, client_type client);
clients_type clients_remove_client(clients_type clients, client_type client);
clients_type clients_remove_client_if(clients_type clients, std::function<bool(client_type)> pred);

}
}

#endif  /* DEVICE_TCP_SERVER_H */
