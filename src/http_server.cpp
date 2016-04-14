#include "http_server.h"
#include "util.h"
#include <json/json.h>
#include "device_message.h"
#include <fstream>
#include "database.h"

namespace parkingserver {

using boost::format;
using std::vector;
using std::string;

namespace dm = device_message;

/* class define */
class http_server
{
  public:
    http_server(options_pointer opts);
    void run();
    void stop();
    void set_handler(Handler handler);
    void set_picture_handler(Handler handler);
  private:
    Handler handler;
    Handler picture_handler;
    shared_ptr<Server> svr;
    options_pointer opts;
};

static vector<string> parse_destination(const string& dest);
bool is_device_id(const string& str);

http_server::http_server(options_pointer opts)
{
    this->opts = opts;
}

void http_server::set_handler(Handler handler)
{
    this->handler = handler;
}

void http_server::set_picture_handler(Handler handler) {
    picture_handler = handler;
}

void http_server::run()
{
    // run http server
    svr = make_shared<Server>(opts->http_port, 10);

    // svr = make_shared<server>(options);
    log()->info("running http server at {}", opts->http_port);


    svr->resource["^/COMPANY/Devices/(.*)/(.*)$"]["POST"] =
            [this](Response& response, std::shared_ptr<Request> request)
            {
                handler(response, request);
            };
    svr->resource["^/PassVehicle/.*$"]["GET"] =
            [this](Response& response, std::shared_ptr<Request> request)
            {
                picture_handler(response, request);
            };
    svr->resource["^/Voucher/.*$"]["GET"] =
            [this](Response& response, std::shared_ptr<Request> request)
            {
                picture_handler(response, request);
            };

    svr->start();
}

void http_server::stop()
{
    svr->stop();
}

void http_server_set_handler( http_server_pointer ptr, Handler handler)
{
    ptr->set_handler(handler);
}

void http_server_set_picture_handler( http_server_pointer ptr, Handler handler)
{
    ptr->set_picture_handler(handler);
}

http_server_pointer make_http_server(options_pointer opts)
{
    return boost::make_shared<http_server>(opts);
}

vector<string> parse_destination(const string& dest)
{
    return util::split(dest, '/');
}

bool is_device_id(const string& str)
{
    return true;
}

void http_server_run(http_server_pointer ptr)
{
    ptr->run();
}

void http_server_stop(http_server_pointer ptr)
{
    ptr->stop();
}

}
