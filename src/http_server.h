#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#define BOOST_SPIRIT_USE_PHOENIX_V3 1
#include "server_http.hpp"

/* #include <boost/network/include/http/server.hpp> */
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/format.hpp>
#include "log.h"
#include "device.h"
#include <list>
#include "options.h"
#include "device_message.h"
#include <vector>

namespace parkingserver {

/* namespace */
/* namespace http = boost::network::http; */
namespace dm = device_message;

using boost::shared_ptr;
using boost::make_shared;
using std::string;
using std::vector;

typedef SimpleWeb::Server<SimpleWeb::HTTP> Server;
typedef Server::Request Request;
typedef Server::Response Response;

typedef std::function<void(Response& response, std::shared_ptr<Request> request)> Handler;

class http_server;
typedef boost::shared_ptr<http_server> http_server_pointer;

/* public function */
http_server_pointer make_http_server(options_pointer opts);
void http_server_set_handler( http_server_pointer ptr, Handler handler);
void http_server_set_picture_handler( http_server_pointer ptr, Handler handler);
void http_server_run(http_server_pointer ptr);
void http_server_stop(http_server_pointer ptr);

}

#endif /* HTTP_SERVER_H */
