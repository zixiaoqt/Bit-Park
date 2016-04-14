#include "userterminal_server.h"
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <fstream>
#include <streambuf>
#include <exception>
#include "log.h"
#include "util.h"

namespace parkingserver {

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

static const int checking_time_interval = 300;

UserTerminalServer::UserTerminalServer(const std::string& ip,
                                       unsigned short port,
                                       const std::string& passvehicle_pic_path,
                                       const std::string& voucher_pic_path)
        : ip_(ip)
        , port_(port)
        , passvehicle_pic_path_(passvehicle_pic_path)
        , voucher_pic_path_(voucher_pic_path){


	//	add by shenzixiao  2016/3/10
	log_.open("websocket.log");
	endpoint_.set_access_channels(websocketpp::log::alevel::all);
	endpoint_.set_error_channels(websocketpp::log::alevel::all);
	endpoint_.get_alog().set_ostream(&log_);
	endpoint_.get_elog().set_ostream(&log_);
	//	add end 
    log()->info("UserTerminalServer init");

   /* endpoint_.set_access_channels(websocketpp::log::alevel::none);*/
	//	Modify by shenzixiao  2016/3/9
	/* none ===>all*/
	endpoint_.set_access_channels(websocketpp::log::alevel::all);
	//	Modify end 
    endpoint_.set_reuse_addr(true);
    endpoint_.init_asio();

    endpoint_.set_open_handler(bind(&UserTerminalServer::on_open,this,_1));
    endpoint_.set_close_handler(bind(&UserTerminalServer::on_close,this,_1));
    endpoint_.set_message_handler(bind(&UserTerminalServer::on_message,this,_1,_2));
    endpoint_.set_fail_handler(bind(&UserTerminalServer::on_fail,this,_1));
    endpoint_.set_ping_handler(bind(&UserTerminalServer::on_ping,this,_1,_2));

    // endpoint_.set_pong_timeout_handler(bind(&UserTerminalServer::on_pong_timeout,
    //                                         this,_1,_2));

    endpoint_.set_timer(checking_time_interval,
                        std::bind(&UserTerminalServer::check_timer,this,_1));

}

UserTerminalServer::~UserTerminalServer() {
	log_.close();	//	add by shenzixiao  2016/3/10
    if (endpoint_.is_listening()) {
        endpoint_.stop_listening();
    }
    endpoint_.stop_perpetual();
    {
        scoped_lock lock(mtx_);
        for (auto iter = begin(connections_); iter != end(connections_); ++iter ) {
            close_connection(iter->first);
        }
    }
    endpoint_.stop();
    //run_thread_.join();
    for(auto& t: threads_) {
        t.join();
    }
}

UserTerminalServer::user_ptr UserTerminalServer::find_user(connection_hdl hdl) {
    scoped_lock lock(mtx_);
    auto iter = connections_.find(hdl);
    if (iter == connections_.end()) {
        throw std::out_of_range("Do Not Find User");
    }
    return iter->second->user_;
}

void UserTerminalServer::send_response(connection_hdl hdl,
                                         const Json::Value& json) {
    Json::FastWriter writer;
    auto msg = writer.write(json);
    websocketpp::lib::error_code ec;
    endpoint_.send(hdl, msg, websocketpp::frame::opcode::text, ec);
    if (ec) {
        log()->error("Error send: {}", ec.message());
    }
    else {
        log()->info("Response to client: {}", msg);
    }
}

void UserTerminalServer::run() {
    std::stringstream ss;
    log()->info("Running server on port {}", port_);

    // listen on specified port
    endpoint_.listen(port_);

    // Start the server accept loop
    endpoint_.start_accept();

    //run_thread_ = websocketpp::lib::thread(&server::run, &endpoint_);
    threads_.clear();
    for(size_t c=0; c < 8; c++) {
        threads_.emplace_back([this](){
                endpoint_.run();
            });
    }
}

void UserTerminalServer::on_message(connection_hdl hdl, message_ptr msg) {
    if( msg->get_opcode() == websocketpp::frame::opcode::TEXT ) {
        request_text_handler_(hdl, msg->get_payload());
    }
    else if( msg->get_opcode() == websocketpp::frame::opcode::BINARY ) {
        request_binary_handler_(hdl, msg->get_payload());
    }
}

void UserTerminalServer::on_open(connection_hdl hdl) {
    log()->info("Open a new user terminal connection");
}

void UserTerminalServer::on_close(connection_hdl hdl) {
    server::connection_ptr con = endpoint_.get_con_from_hdl(hdl);
    log()->error("User terminal connection is closed, state:{}, close code:{}({})",
                   con->get_state(),
                   con->get_remote_close_code(),
                   websocketpp::close::status::get_string(con->get_remote_close_code()));
	//	Modify by shenzixiao  2016/3/8
	/*
		增加关闭原因和本地关闭原因
	*/
	log()->error("close reason:{},local reason:{}", con->get_remote_close_reason(),con->get_local_close_reason());
    
	// Modeify end 
	erase_connection(hdl);
}

void UserTerminalServer::on_fail(connection_hdl hdl) {
    server::connection_ptr con = endpoint_.get_con_from_hdl(hdl);
    log()->error("User terminal connection is fail, state:{}, close code:{}({})",
                   con->get_state(),
                   con->get_remote_close_code(),
                   websocketpp::close::status::get_string(con->get_remote_close_code()));
    erase_connection(hdl);
}

bool UserTerminalServer::on_ping(connection_hdl hdl, std::string msg) {
    scoped_lock lock(mtx_);
    auto iter = connections_.find(hdl);
    if (iter != connections_.end()) {
        iter->second->heart_time_ = util::GetCurrentTime();
        return true;
    }
    else {
        return false;
    }
}
// void UserTerminalServer::on_pong_timeout(connection_hdl hdl, std::string msg) {
// }

void UserTerminalServer::notify_pass_vehicle_to_all_client(const Json::Value& msg) {
    try {
        log()->info("Notify pass vehicle {} to all client", msg);
        Json::FastWriter writer;
        auto notify_msg = writer.write(msg);
        scoped_lock lock(mtx_);
        for (auto& con : connections_) {
            endpoint_.send(con.first, notify_msg,
                           websocketpp::frame::opcode::text);
        }
    }
    catch (std::exception& e) {
        log()->error("Notify exception {}", e.what());
    }
}

bool UserTerminalServer::notify_pass_vehicle_to_client(const Json::Value& msg,
                                                       const std::string& device_id) {
    bool has_send = false;
    try {
        Json::FastWriter writer;
        auto notify_msg = writer.write(msg);
        scoped_lock lock(mtx_);
        for (auto& con : connections_) {
            if (con.second->has_bind_device(device_id)) {
                log()->info("Notify pass vehicle {} of device {} to client {}",
                            msg, device_id, con.second->ip_);
                endpoint_.send(con.first, notify_msg,
                               websocketpp::frame::opcode::text);
                has_send = true;
            }
        }
    }
    catch (std::exception& e) {
        log()->error("Notify exception {}", e.what());
    }
    log()->info("Status of notifing pass vehicle of device {} to client: {}",
                device_id, has_send);
    return has_send;
}

void UserTerminalServer::close_connection(connection_hdl hdl) {
    log()->info("Close the connection");
    websocketpp::lib::error_code ec;
    endpoint_.close(hdl, websocketpp::close::status::going_away, "", ec);
    if (ec) {
        log()->error("Error closing: {}", ec.message());
    }
    erase_connection(hdl);
}

void UserTerminalServer::add_connection_user(connection_hdl hdl, user_ptr user) {
    auto session = std::make_shared<Session>(user);
    server::connection_ptr con = endpoint_.get_con_from_hdl(hdl);
    const static boost::regex pattern("\\d{1,3}(\\.\\d{1,3}){3}");
    boost::smatch result;
    auto address = con->get_remote_endpoint();
    if (boost::regex_search(address, result, pattern)) {
        if (!result.empty()) {
            session->ip_ = result[0].str();
        }
    }
    session->heart_time_ = util::GetCurrentTime();
    log()->info("Add connection of User {}", session->ip_);
    scoped_lock lock(mtx_);
    connections_[hdl] = session;
}

void UserTerminalServer::erase_connection(connection_hdl hdl) {
    log()->debug("Erase the connection from connections");
    scoped_lock lock(mtx_);
    connections_.erase(hdl);
}

void UserTerminalServer::check_timer(const websocketpp::lib::error_code& ec) {
    if (ec) {
        log()->warn("Timer of checking client heart beart is error, {}",
                     ec.message());
    }

    scoped_lock lock(mtx_);
    for (auto iter = connections_.begin(); iter != connections_.end();) {
        if (util::GetCurrentTime() - iter->second->heart_time_ > 60) {
            log()->error("Heart beart of the client is over time");
            log()->info("Close connection {}", get_client_address(iter->first));
            websocketpp::lib::error_code ec;
            endpoint_.close(iter->first, websocketpp::close::status::going_away, "", ec);
            if (ec) {
              log()->error("Error closing: {}", ec.message());
            }
            connections_.erase(iter++);
        }
        else {
            ++iter;
        }
    }
    endpoint_.set_timer(checking_time_interval,
                        std::bind(&UserTerminalServer::check_timer,this,_1));
}

void UserTerminalServer::bind_device(connection_hdl hdl,
                                     int direction,
                                     const std::string& device_id) {
    auto iter = connections_.find(hdl);
    if (iter != connections_.end()) {
        iter->second->bind_device(direction, device_id);
    }
}

user_server_pointer make_user_server(const std::string& ip,
                                     unsigned short port,
                                     const std::string& passvehicle_pic_path,
                                     const std::string& voucher_pic_path) {
    return std::make_shared<UserTerminalServer>(ip, port,
                                                passvehicle_pic_path,
                                                voucher_pic_path);
}

}
