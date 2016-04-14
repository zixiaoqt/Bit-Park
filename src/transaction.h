#ifndef _PARKINGSERVER_TRANSACTION_
#define _PARKINGSERVER_TRANSACTION_
#include "http_server.h"
#include "userterminal_server.h"
#include "device_tcp_server.h"
#include "options.h"
#include "device.h"
#include "database.h"
#include <list>
#include <boost/thread.hpp>

namespace parkingserver {
namespace dts = device_tcp_server;
/* namespace dm = device_message; */

class Transaction {
  public:
    Transaction(options_pointer opts,
                http_server_pointer http_server,
                user_server_pointer user_server,
                dts::server_pointer dts_server);
    ~Transaction();

  public:
    double handle_charge(const PassVehicle& enter_vehicle,
                         const PassVehicle& exit_vehicle,
                         Json::Value& json);
    bool notify_pass_vehicle(const Json::Value& msg,
                                          const std::string& device_id);
    void auto_open_exit_gateway(const std::string& device_ip);
    void auto_open_entrance_gateway(const std::string& device_ip,
                                    const std::string& plate_number,
									int plate_type);	//	modify by shenzixiao 2016/4/16 添加车牌类型
	void remain_parking_number_bind(const std::string& device_ip,
									const std::string& plate_number,
									int remain_num,
									int bind_type);	//	add by shenzixiao 2016/4/7
	void wait_vehicle_release(const std::string& device_ip,
		const std::string& plate_number,
		int remain_num,
		int bind_type);	//	add by shenzixiao 2016/4/8
    void led_show_charge(const std::string& device_ip, double charge,
                         boost::optional<int> duration);
    void led_show_remain_parking_number(const std::string& device_ip, int number);
    void led_show_plate_number(const std::string& device_ip,
                                            const std::string& plate_number);
    void led_show_greeting(const std::string& device_ip,
                           const std::string& greeting);
    void delete_temporary_passrecord(const std::string& pass_id);
  private:
    enum PictureType {
        PassVehiclePictureType,
        VoucherPictureType
    };

    void client_submit_picture(UserTerminalServer::connection_hdl hdl,
                               const std::string& payload);
    void client_request(UserTerminalServer::connection_hdl hdl,
                        const std::string& request_msg);

    void on_login(const Json::Value& json,
                  UserTerminalServer::connection_hdl hdl);
    void on_logout(const Json::Value& json,
                   UserTerminalServer::connection_hdl hdl);
    void on_shift(const Json::Value& json,
                  UserTerminalServer::connection_hdl hdl);
    void on_query_gateway(const Json::Value& json,
                          UserTerminalServer::connection_hdl hdl);
    void on_query_device(const Json::Value& json,
                         UserTerminalServer::connection_hdl hdl);
    void on_query_parkingvehicle(const Json::Value& json,
                                 UserTerminalServer::connection_hdl hdl);
    void on_revise_plate(const Json::Value& json,
                         UserTerminalServer::connection_hdl hdl);
    void on_operate_gateway(const Json::Value& json, UserTerminalServer::connection_hdl hdl);

    void on_checkout(const Json::Value& json,
                     UserTerminalServer::connection_hdl hdl);
    void on_match_plate(const Json::Value& json,
                        UserTerminalServer::connection_hdl hdl);
    void on_query_parkingnumber(const Json::Value& json,
                                UserTerminalServer::connection_hdl hdl);
    /* void on_revise_remainparkingnumber(const Json::Value& json, */
    /*                                    UserTerminalServer::connection_hdl hdl); */
    void on_bind_device(const Json::Value& json,
                               UserTerminalServer::connection_hdl hdl);
    void on_operate_device(const Json::Value& json,
                                  UserTerminalServer::connection_hdl hdl);
    void on_lock_device(const Json::Value& json,
                                     UserTerminalServer::connection_hdl hdl);

    void http_server_handler(Response& response, std::shared_ptr<Request> request);
    void http_server_picture_handler(Response& response, std::shared_ptr<Request> request);

    void operate_gateway(const string& device_ip, int action_type, int mode);

    void delete_passvehicle_pic(const std::vector<PassVehiclePicture>& vehicles);
    void clear_expired_vehicle_pic(int expiry);
    void clear_earliest_day_vehicle_pic();
    void clear_vehicle_pic_by_counts();

    void delete_voucher_pic(const std::vector<ChargeVoucherPicture>& vouchers);
    void clear_expired_voucher_pic(int expiry);
    void clear_earliest_day_voucher_pic();
    void clear_voucher_pic_by_counts();

    void picture_storage_manage(PictureType pic_type);

    void handle_entrance_led_show_timer(const boost::system::error_code& e,
                                        const std::string& device_ip,
                                        std::shared_ptr<boost::asio::deadline_timer> timer);

    void handle_exit_led_show_timer(const boost::system::error_code& e,
                                    const std::string& device_ip,
                                    std::shared_ptr<boost::asio::deadline_timer> timer);
    void show_led_after_charge(const std::string& device_ip);

    void notify_remain_parking_to_all_device();
    void on_esc_record(const Json::Value& json, UserTerminalServer::connection_hdl hdl);
  private:
    options_pointer opts_;
    http_server_pointer http_server_;
    user_server_pointer user_server_;
    dts::server_pointer dts_server_;
    boost::asio::io_service ios_;  // for timer
    boost::asio::io_service::work work_;
    boost::thread ios_thd_;        // timer thread
    boost::asio::deadline_timer timer_;  // timEr
    boost::asio::deadline_timer dts_timer_;  // timEr

    boost::thread dts_thd_;
    dts::clients_type dts_clients_;
    boost::mutex dts_clients_mtx_;
    std::list<device_pointer> device_list_;

    boost::mutex entrance_led_timer_mtx_;
    std::map<std::string, std::list<std::shared_ptr<boost::asio::deadline_timer>>> entrance_led_timers_;

    boost::mutex exit_led_timer_mtx_;
    std::map<std::string, std::list<std::shared_ptr<boost::asio::deadline_timer>>> exit_led_timers_;

    void new_client_notify(dts::client_type);
    dts::client_type find_dts_client(const string &ip);
    void remove_dts_client(dts::client_type);
    void on_device_keepalive(const string& source, const string& devid, time_t t);
    void on_device_data(const string& source, const string& devid,
                        const dm::device_data& data, const vector<string>& pics);
    void on_device_status(const string& source, const string& devid, const device_status& data);
    device_pointer find_device(const string& id);
    /* device_pointer find_device(const string& ip); */
    void dts_operate(const string& device_ip, std::function<Json::Value()> f);
    void operate_led(const string& device_ip,
                     int action,  // dm::la_display or dm::la_clear
                     const string &content,
                     int color,  // dm::lc_red, dm::lc_green or dm::lc_blue
                     int size,   // undefined
                     int mode,   // dm::lm_fixed or dm::lm_scrolling
                     boost::optional<int> duration);  // seconds
    void lock_device(const string& device_ip);
    void unlock_device(const string& device_ip);
    void clear_pic(const boost::system::error_code& error);
    void dts_heartbeat(const boost::system::error_code& error);
	void led_show_new_client(const string& device_ip);
	boost::thread led_show_new_client_thd_;

};
}
#endif
