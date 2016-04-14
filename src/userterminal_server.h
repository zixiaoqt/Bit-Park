#ifndef _USERTERMINAL_SERVER__
#define _USERTERMINAL_SERVER__
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <map>
#include <string>
#include <json/json.h>
#include "user.h"
#include "session.h"
#include "passvehicle.h"
#include <fstream>	//	add by shenzixiao 2016/3/10	
namespace parkingserver {

  class UserTerminalServer {
  public:
    using connection_hdl = websocketpp::connection_hdl;
    using server = websocketpp::server<websocketpp::config::asio>;
    using scoped_lock = websocketpp::lib::lock_guard<websocketpp::lib::mutex>;
    using message_ptr = server::message_ptr;
    using user_ptr = std::shared_ptr<User>;
    using session_ptr = std::shared_ptr<Session>;
    using request_handler = std::function<void(connection_hdl, const std::string&)>;

    UserTerminalServer(const std::string& ip,
                       unsigned short port,
                       const std::string& passvehicle_pic_path,
                       const std::string& voucher_pic_path);
    ~UserTerminalServer();

    void run();

    void on_message(connection_hdl hdl, message_ptr msg);
    void on_http(connection_hdl hdl);
    void on_open(connection_hdl hdl);
    void on_close(connection_hdl hdl);
    void on_fail(connection_hdl hdl);
    bool on_ping(connection_hdl hdl, std::string msg);

    /* void on_pong_timeout(connection_hdl hdl, std::string msg); */

    void check_timer(const websocketpp::lib::error_code& ec);

    void notify_pass_vehicle_to_all_client(const Json::Value& msg);
    bool notify_pass_vehicle_to_client(const Json::Value& msg,
                                       const std::string& device_id);

    void send_response(connection_hdl hdl, const Json::Value& json);
    void close_connection(connection_hdl hdl);
    user_ptr find_user(connection_hdl hdl);
    void add_connection_user(connection_hdl hdl, user_ptr user);
    void erase_connection(connection_hdl hdl);

    void bind_device(connection_hdl hdl, int direction,
                     const std::string& device_id);

    void set_text_request_handler(request_handler h) {request_text_handler_ = h;}
    void set_binary_request_handler(request_handler h) {request_binary_handler_ = h;}

    std::string get_client_address(connection_hdl hdl) {
      scoped_lock lock(mtx_);
      auto iter = connections_.find(hdl);
      if (iter != connections_.end()) {
        return iter->second->ip_;
      }
      return "";
    }
  private:
    using con_list = std::map<connection_hdl, session_ptr, std::owner_less<connection_hdl>>;

    server endpoint_;
    con_list connections_;
    websocketpp::lib::mutex mtx_;
    std::vector<websocketpp::lib::thread> threads_;
    //websocketpp::lib::thread run_thread_;
    request_handler request_text_handler_;
    request_handler request_binary_handler_;

    std::string ip_;
    unsigned short port_;
    std::string passvehicle_pic_path_;
    std::string voucher_pic_path_;
	std::ofstream log_;		//	add by shenzixiao 2016/3/10	
  };

  using user_server_pointer = std::shared_ptr<UserTerminalServer>;

  user_server_pointer make_user_server(const std::string& ip,
                                       unsigned short port,
                                       const std::string& passvehicle_pic_path,
                                       const std::string& voucher_pic_path);

}
#endif
