#include "device_tcp_server.h"
#include <boost/make_shared.hpp>
#include <boost/format.hpp>
#include "device_message.h"
#include "log.h"

namespace parkingserver {
namespace device_tcp_server {
using boost::make_shared;
using boost::format;

client_type invalid_client_object = std::make_pair(nullptr, nullptr);

static void error(const string& err) {
    throw client_error(err);
}

server_pointer make_server(unsigned short port)
{
    auto ptr = make_shared<server>();
    ptr->acceptor = make_shared<tcp::acceptor>(ptr->ios, tcp::endpoint(tcp::v4(), port));
    return ptr;
}

client_type make_client(stream_ptr s, mtxptr m)
{
    return std::make_pair(s, m);
}

void server_run(server_pointer ptr, new_client_notify notify)
{
    boost::system::error_code ec;
    while(!ptr->ios.stopped()) {
        auto conn = make_shared<tcp::iostream>();
        ptr->acceptor->accept(*conn->rdbuf(), ec);
        if (ec) {
            parkingserver::log()->warn("server_run, accept error: {}", ec);
        } else {
            try {
                notify(make_client(conn, make_shared<mutex>()));
            }
            catch(std::exception& err) {
                parkingserver::log()->warn("server_run, notify new client: {}", err.what());
            }
        }
    }
}

void server_stop(server_pointer ptr)
{
    ptr->ios.stop();
    ptr->acceptor->close();
}

bool client_is_valid(client_type c)
{
    return c != invalid_client_object;
}

string client_address(client_type client)
{
    return client.first->rdbuf()->remote_endpoint().address().to_string();
}

unsigned short client_port(client_type client)
{
    return client.first->rdbuf()->remote_endpoint().port();
}

string client_send_recv(client_type client, const string& message)
{
    auto stream = client_stream(client);
    auto mtx = client_mutex(client);
    if(!stream) {
        error("invalid stream object");
    }

    boost::unique_lock<boost::mutex> lock(*mtx);
    
    (*stream) << message;
    stream->flush();

    stream->expires_from_now(boost::posix_time::seconds(3));
    
    string s;
    // ipnc
    if (!getline(*stream, s, ',')) {
        error(str(format("recv ipnc error, %1%") % stream->error().message()));
    }
    if (s != "IPNC") {
        error("parse ipnc response error, no 'IPNC'");
    }

    // addr
    if (!getline(*stream, s, ',')) {
        error(str(format("recv addr error, %1%") % stream->error().message()));
    }
    string addr = s;

    // type
    if (!getline(*stream, s, ',')) {
        error(str(format("recv type error, %1%") % stream->error().message()));
    }

    if (stoi(s) != device_message::ict_response) {
        error("parse ipnc response error, type is not respose");
    }
    // checksum
    if (!getline(*stream, s, ',')) {
        error(str(format("recv checksum error, %1%") % stream->error().message()));
    }

    auto checksum = stoi(s);

    // length
    if (!getline(*stream, s, ',')) {
        error(str(format("recv length error, %1%") % stream->error().message()));
    }
    auto length = stoi(s);

    char *buf = new char[length+1];
    stream->read(buf, length);
    if (!stream) {
        error(str(format("recv body error, %1%") % stream->error().message()));
    }
    if (stream->gcount() != length) {
        delete[] buf;
        error(str(format("recv body error, gcount(%1%) != length(%2%)")
                  % stream->gcount()
                  % length));
    }
    buf[length] = '\0';
    string ret = buf;
    delete[] buf;

    if (checksum != device_message::check_sum(ret)) {
        error(str(format("checksum error(%1% != %2%)")
                  % checksum
                  % device_message::check_sum(ret)));
    }
    return ret;
}

stream_ptr client_stream(client_type c)
{
    return c.first;
}

mtxptr client_mutex(client_type c)
{
    return c.second;
}


clients_type make_clients()
{
    return make_shared<std::list<client_type>>();
}

client_type clients_find_client(clients_type clients, const string& ipaddr)
{
    for(auto client : *clients) {
        if (client_address(client) == ipaddr) {
            return client;
        }
    }
    return invalid_client_object;
}

clients_type clients_add_client(clients_type clients, client_type client)
{
    clients->push_back(client);
    return clients;
}

clients_type clients_remove_client(clients_type clients, client_type client)
{
    clients->remove(client);
    return clients;
}

clients_type clients_remove_client_if(clients_type clients, std::function<bool(client_type)> pred)
{
    clients->remove_if(pred);
    return clients;
}


}
}
