#include "transaction.h"
#include <boost/filesystem.hpp>
#include "log.h"
#include "gateway.h"
#include "parkingvehicle.h"
#include "database.h"
#include <fstream>
#include <chrono>
#include <future>
#include "chargerule.h"
#include "util.h"
#include "error.h"
#include "scopeguard.h"
#include "chargefactory.h"
#include "releasefactory.h"
#include <sstream>

namespace parkingserver {
const char* RESPONSE_ERROR = "ERROR";
const char* RESPONSE_OK = "OK";

using boost::format;
using namespace std::chrono;

std::string current_time();
void write_to_file(const string& filename, const string& data);
inline void throw_error(error::value v);

static const int led_show_charge_time = 5;
static const int led_show_platenumber_time = 5;

Transaction::Transaction(options_pointer opts,
                         http_server_pointer http_server,
                         user_server_pointer user_server,
                         dts::server_pointer dts_server)
        : opts_(opts)
        , http_server_(http_server)
        , user_server_(user_server)
        , dts_server_(dts_server)
        , dts_clients_(dts::make_clients())
        , work_(ios_)
        , timer_(ios_)
        , dts_timer_(ios_) {
	log()->info("Transaction init");
    device_list_ = QueryDeviceList();

    user_server_->set_text_request_handler(std::bind(&Transaction::client_request,
                                                     this, std::placeholders::_1,
                                                     std::placeholders::_2));
    user_server_->set_binary_request_handler(std::bind(&Transaction::client_submit_picture,
                                                       this, std::placeholders::_1,
                                                       std::placeholders::_2));

    dts_thd_ = boost::thread(&dts::server_run, dts_server_,
                             std::bind(&Transaction::new_client_notify, this,
                                       std::placeholders::_1));

    http_server_set_handler(http_server_, std::bind(&Transaction::http_server_handler, this,
                                                    std::placeholders::_1,
                                                    std::placeholders::_2));

    http_server_set_picture_handler(http_server_,
                                    std::bind(&Transaction::http_server_picture_handler, this,
                                              std::placeholders::_1,
                                              std::placeholders::_2));

    ios_thd_ = boost::thread(boost::bind(&boost::asio::io_service::run, &ios_));

    timer_.expires_from_now(boost::posix_time::seconds(util::SecondsToNextDay()));
    timer_.async_wait(std::bind(&Transaction::clear_pic, this, std::placeholders::_1));

    dts_timer_.expires_from_now(boost::posix_time::seconds(3));
    dts_timer_.async_wait(std::bind(&Transaction::dts_heartbeat, this, std::placeholders::_1));
}

Transaction::~Transaction()
{
    dts::server_stop(dts_server_);
    dts_thd_.join();

    dts_timer_.cancel();
    timer_.cancel();
    ios_.stop();
    ios_thd_.join();

	led_show_new_client_thd_.join();
}

void Transaction::client_submit_picture(UserTerminalServer::connection_hdl hdl,
                                        const std::string& payload) {
    try {
        picture_storage_manage(VoucherPictureType);
        auto id = payload.substr(0, 32);
        log()->info("Receive voucher about: {}", id);
        auto filetype = payload.substr(32, 3);
        auto t = util::GetCurrentTime();
        auto filename = str(format("%1%_%2%.%3%") % id
                            % util::FormatLocalTime(t, "%Y%m%d%H%M%S")
                            % filetype);
        auto filedirectory = util::PictureDirectory(opts_->voucher_pic_path, t);
        auto filepath = filedirectory + "/" + filename;
        log()->info("Save voucher picture to {}", filepath);
        std::ofstream ostr(filepath,
                           std::ios_base::out|std::ios_base::binary|std::ios_base::trunc);
        ostr.write(payload.substr(37, std::string::npos).c_str(), payload.length()-37);
        auto fileurl = str(format("http://%1%:%2%/Voucher/%3%/%4%") % opts_->server_ip
                           % opts_->http_port
                           % util::GetPictureDirectoryName(filedirectory)
                           % filename);
        UpdateVoucherPictureURL(id, fileurl);
    }
    catch (std::exception& e) {
        log()->error("Submit picture exception: {}", e.what());
    }
}

void Transaction::client_request(UserTerminalServer::connection_hdl hdl,
                                 const std::string& request_msg) {
    Json::Value root;
    try {
        log()->info("Receive client {} msg: {}",
                    user_server_->get_client_address(hdl),
                    request_msg);
        Json::Reader reader;
        bool rt = reader.parse(request_msg, root);
        if (!rt) {
            throw_error(error::invalid_json);
        }

        auto cmd = root["CmdType"].asString();
        if (cmd == "Login") {
            on_login(root, hdl);
        }
        else if (cmd == "Logout") {
            on_logout(root, hdl);
        }
        else if (cmd == "Shift") {
            on_shift(root, hdl);
        }
        else if (cmd == "QueryGateway") {
            on_query_gateway(root, hdl);
        }
        else if (cmd == "QueryDevice") {
            on_query_device(root, hdl);
        }
        else if (cmd == "QueryParkingVehicle") {
            on_query_parkingvehicle(root, hdl);
        }
        else if (cmd == "RevisePlate") {
            on_revise_plate(root, hdl);
        }
        else if (cmd == "MatchPlate") {
            on_match_plate(root, hdl);
        }
        else if (cmd == "OperateGateway") {
            on_operate_gateway(root, hdl);
        }
        else if (cmd == "Checkout") {
            on_checkout(root, hdl);
        }
        else if (cmd == "QueryParkingNumber") {
            on_query_parkingnumber(root, hdl);
        }
        // else if (cmd == "ReviseRemainParkingNumber") {
        //     on_revise_remainparkingnumber(root, hdl);
        // }
        else if (cmd == "BindDevice") {
            on_bind_device(root, hdl);
        }
        else if (cmd == "OperateDevcie") {
            on_operate_device(root, hdl);
        }
        else if (cmd == "LockDevice") {
            on_lock_device(root, hdl);
        }
        else if (cmd == "EscRecord") {
            on_esc_record(root, hdl);
        }
        else {
            throw_error(error::invalid_cmdtype);
        }
    }
    catch (std::system_error& se) {
        log()->error("Exception code: {}, message:{}", se.code().value(),
                     se.what());
        Json::Value response;
        response["CmdType"] = root["CmdType"].asString()+"Response";
        response["Result"] = RESPONSE_ERROR;
        response["ErrorCode"] = se.code().value();
        response["ErrorInfo"] = se.what();
        user_server_->send_response(hdl, response);
    }
    catch (std::exception& e) {
        log()->error("Exception {}", e.what());
        Json::Value response;
        response["CmdType"] = root["CmdType"].asString()+"Response";
        response["Result"] = RESPONSE_ERROR;
        response["ErrorInfo"] = e.what();
        user_server_->send_response(hdl, response);
    }
}

void Transaction::on_login(const Json::Value& json,
                           UserTerminalServer::connection_hdl hdl) {
    log()->info("User {} start to login", json["User"].asString());
    Json::Value response;
    response["CmdType"] = "LoginResponse";
    auto user_id = json["User"].asString();
    auto user = QueryUser(user_id);
    if (user) {
        if (user_id == user->user_ &&
            json["Password"].asString() == user->password_) {
            log()->info("Login success");
            response["Result"] = RESPONSE_OK;
            user->toJson(response);
            user_server_->add_connection_user(hdl, user);
            user_server_->send_response(hdl, response);
            return;
        }
    }
    log()->warn("Invalid user or password");
    response["Result"] = RESPONSE_ERROR;
    response["ErrorCode"] = error::invalid_user_password;
    response["ErrorInfo"] = error::get_category().message(static_cast<int>(
        error::invalid_user_password));
    user_server_->send_response(hdl, response);
    user_server_->close_connection(hdl);
    log()->info("Close the connection");
}

void Transaction::on_logout(const Json::Value& json,
                            UserTerminalServer::connection_hdl hdl) {
    auto user = user_server_->find_user(hdl);
    log()->info("User {} logout", user->user_);
    Json::Value response;
    response["CmdType"] = "LogoutResponse";
    response["Result"] = RESPONSE_OK;
    user_server_->send_response(hdl, response);
    user_server_->close_connection(hdl);
    user_server_->erase_connection(hdl);
}

void Transaction::on_shift(const Json::Value& json,
                           UserTerminalServer::connection_hdl hdl) {
    auto user = user_server_->find_user(hdl);
    log()->info("User {} shift to User {}", user->user_, json["NextUser"]);
    Json::Value response;
    response["CmdType"] = "ShiftResponse";
    bool success = false;
    if (json["CurrentUser"].asString() == user->user_ &&
        json["CurrentPassword"].asString() == user->password_) {
        auto next_user_id = json["NextUser"].asString();
        auto new_user = QueryUser(next_user_id);
        if (new_user) {
            if(next_user_id == new_user->user_ &&
               json["NextPassword"].asString() == new_user->password_) {
                response["Result"] = RESPONSE_OK;
                new_user->toJson(response);
                user_server_->add_connection_user(hdl, new_user);
                success = true;
                InsertShiftLog(json);
                user_server_->send_response(hdl, response);
            }
            else {
                log()->warn("Next user is invalid");
            }
        }
    }
    else {
        log()->warn("Current user is invalid");
    }
    if(!success) {
        throw_error(error::invalid_user_password);
    }
}

void Transaction::on_query_gateway(const Json::Value& json,
                                   UserTerminalServer::connection_hdl hdl) {
    auto user = user_server_->find_user(hdl);
    log()->info("User {} query gateway", user->user_);
    Json::Value response;
    response["CmdType"] = "QueryGatewayResponse";
    response["Result"] = RESPONSE_OK;
    response["List"] = QueryGateway();
    user_server_->send_response(hdl, response);
}

void Transaction::on_query_device(const Json::Value& json,
                                  UserTerminalServer::connection_hdl hdl) {
    auto user = user_server_->find_user(hdl);
    log()->info("User {} query device", user->user_);
    Json::Value response;
    response["CmdType"] = "QueryDeviceResponse";
    response["Result"] = RESPONSE_OK;
    auto devices = QueryDeviceByBindIP(user_server_->get_client_address(hdl));
    if (devices.empty()) {
        response["List"] = QueryDevice();
    }
    else {
        response["List"] = devices;
    }
    user_server_->send_response(hdl, response);
}

void Transaction::on_query_parkingvehicle(const Json::Value& json,
                                          UserTerminalServer::connection_hdl hdl) {
    auto user = user_server_->find_user(hdl);
    log()->info("User {} query parking vehicle", user->user_);
    Json::Value response;
    response["CmdType"] = "QueryParkingVehicleResponse";
    response["Result"] = RESPONSE_OK;
    response["EnterVehicleList"] = QueryParkingVehicle(json);
    user_server_->send_response(hdl, response);
}

void Transaction::on_revise_plate(const Json::Value& json,
                                  UserTerminalServer::connection_hdl hdl) {
    auto user = user_server_->find_user(hdl);
    auto passid = json["PassID"].asString();
    auto plate_number = json["PlateNumber"].asString();
    log()->info("User {} revise plate of pass {} to {}",
                user->user_, passid, plate_number);
    ModifyPlateNumber(passid, plate_number, user->user_);
    Json::Value response;
    response["CmdType"] = "RevisePlateResponse";
    response["Result"] = RESPONSE_OK;
    user_server_->send_response(hdl, response);
}

void Transaction::on_match_plate(const Json::Value& json,
                                 UserTerminalServer::connection_hdl hdl) {
    auto user = user_server_->find_user(hdl);
    auto enterid = std::string(json["EnterID"].asString());
    auto exitid = std::string(json["ExitID"].asString());
    log()->info("User {} match plate in pass record [{}] and [{}]", user->user_,
                enterid, exitid);
    if (enterid.empty() || exitid.empty()) {
        throw_error(error::match_platenumber);
    }
    auto enter_vehicle = QueryPassVehicleByID(enterid);
    auto exit_vehicle = QueryPassVehicleByID(exitid);
    if (enter_vehicle.pass_time_ == 0 || exit_vehicle.pass_time_ == 0) {
        throw_error(error::invalid_passvehicle_record);
    }
    Json::Value response;
    response["CmdType"] = "MatchPlateResponse";
    response["Result"] = RESPONSE_OK;

    auto dev = QueryDeviceByID(exit_vehicle.device_id_);
    if (dev->device_id.empty()) {
        log()->error("Not find device {}", exit_vehicle.device_id_);
    }

    auto charge = handle_charge(enter_vehicle, exit_vehicle, response);
    try {
        if (charge == 0) {
            led_show_charge(dev->ip, charge, led_show_charge_time);
            operate_gateway(dev->ip, dm::bgc_open, dm::bgm_auto);
        }
        else {
            led_show_charge(dev->ip, charge, boost::none);
        }
    }
    catch (std::exception& e) {
        log()->error(e.what());
    }

    user_server_->send_response(hdl, response);
}

void Transaction::on_operate_gateway(const Json::Value& json, UserTerminalServer::connection_hdl hdl)
{
    auto user = user_server_->find_user(hdl);
    auto devid = json["DeviceID"].asString();
    log()->info("User {} operate gateway {}", user->user_, devid);

    auto dev = QueryDeviceByID(devid);
    if (dev->device_id.empty()) {
        log()->error("Not find device {}", devid);
        throw_error(error::not_find_gateway_device);
    }

    operate_gateway(dev->ip, json["ActionType"].asInt(), dm::bgm_manual);

    // 操作纪录入数据库
    ManualGatewayOperationInfo operation{json["PassID"].asString(),
                devid, json["ActionType"].asInt(), user->user_};
    InsertGatewayOperationLog(operation);

    Json::Value response;
    response["CmdType"] = "OperateGatewayResponse";
    response["Result"] = RESPONSE_OK;
    user_server_->send_response(hdl, response);
}

void Transaction::on_checkout(const Json::Value& json,
                              UserTerminalServer::connection_hdl hdl) {
    auto user = user_server_->find_user(hdl);
    auto enterid = json["EnterID"].asString();
    auto exitid = json["ExitID"].asString();
    auto exit_vehicle = QueryPassVehicleByID(exitid);
    Json::Value response;
    response["CmdType"] = "CheckoutResponse";
    response["Result"] = RESPONSE_OK;
    log()->info("User {} checkout platenumber {}", user->user_,
                exit_vehicle.plate_number1_);
    std::string strbill;
    float discount = 1;
    if (!json["Bill"].isNull()) {
        Json::FastWriter writer;
        strbill = writer.write(json["Bill"]);
        discount = json["Bill"]["Discount"].asFloat();
    }
    ChargeRecord record;
    record.exit_id = exitid;
    record.enter_id = enterid;
    record.charge = json["Charge"].asFloat();
    record.actual_charge = json["ActualCharge"].asFloat();
    record.pay_type = json["PayType"].asInt();
    record.user = user->user_;
    record.parking_type = json["ParkingType"].asInt();
    record.exit_type = json["ExitType"].asInt();
    record.exit_comment = json["ExitComment"].asString();
    record.charge_detail = strbill;
    record.discount = discount;

    if (json["ParkingType"].asInt() == STORED_VALUE_PARKING) {
        VIPVehicle vip;
        auto findVIP = QueryVIPVehicle(exit_vehicle.plate_number1_, vip);
        record.remain_money = vip.remain_money;
    }
    auto charge_id = json["ChargeID"].asString();
    if (charge_id.empty()) {
        charge_id = GenerateChargeRecordID();
    }
    InsertChargeRecord(charge_id, record);
	// delete_temporary_passrecord(enterid);
	//	add by shenzixiao 2016/4/8  无入场记录不删除场内表
	log()->info("\346\227\240\345\205\245\345\234\272\350\256\260\345\275\225");
	auto error = "\346\227\240\345\205\245\345\234\272\350\256\260\345\275\225";
	if (record.exit_comment != error)
	{
		delete_temporary_passrecord(enterid);
	}
	//	add end 
    // 开闸
    auto dev = QueryDeviceByPassID(exitid);
    if (dev->device_id.empty()) {
        log()->error("Not find device by passid: {}", exitid);
        throw_error(error::not_find_gateway_device);
    }

    // 开闸
    operate_gateway(dev->ip, dm::bgc_open, dm::bgm_auto);
    show_led_after_charge(dev->ip);

    user_server_->send_response(hdl, response);
}

void Transaction::on_query_parkingnumber(const Json::Value& json,
                                         UserTerminalServer::connection_hdl hdl) {
    auto user = user_server_->find_user(hdl);
    log()->info("User {} query parking number", user->user_);

    auto number = QueryParkingNumber();

    Json::Value response;
    response["CmdType"] = "QueryParkingNumberResponse";
    response["Result"] = RESPONSE_OK;
    response["TotalParkingNumber"] = number.total_parking;
    response["RemainParkingNumber"] = number.remain_parking;
    user_server_->send_response(hdl, response);
}

// void Transaction::on_revise_remainparkingnumber(const Json::Value& json,
//                                                 UserTerminalServer::connection_hdl hdl) {
//     auto user = user_server_->find_user(hdl);
//     log()->info("User {} query parking number", user->user_);

//     UpdateRemainParkingNumber(json["RemainParkingNumber"].asInt());

//     Json::Value response;
//     response["CmdType"] = "ReviseRemainParkingNumberResponse";
//     response["Result"] = RESPONSE_OK;
//     user_server_->send_response(hdl, response);
// }

void Transaction::on_bind_device(const Json::Value& json,
                                 UserTerminalServer::connection_hdl hdl) {
    auto user = user_server_->find_user(hdl);
    log()->info("User {} bind device", user->user_);

    auto device_id = json["DeviceID"].asString();
    user_server_->bind_device(hdl, json["PassDirection"].asInt(), device_id);

    // 解锁设备
    try {
        log()->info("User {} unlock device {}", user->user_, device_id);
        auto device = QueryDeviceByID(device_id);
        if (device != nullptr) {
            unlock_device(device->ip);
            std::string greeting;
            if (device->type == ParkEntrance) {
                greeting = QueryEntranceGreeting();
            }
            else if (device->type == ParkExit) {
                greeting = QueryExitGreeting();
            }
            if (!greeting.empty()) {
                // 在LED上显示祝福语
                led_show_greeting(device->ip, greeting);
            }
        }
    }
    catch (const std::exception& e) {
        log()->error("Unlock device {} error: {}", device_id, e.what());
    }

    Json::Value response;
    response["CmdType"] = "BindDeviceResponse";
    response["Result"] = RESPONSE_OK;
    user_server_->send_response(hdl, response);
}

void Transaction::on_operate_device(const Json::Value& json,
                              UserTerminalServer::connection_hdl hdl) {
    auto user = user_server_->find_user(hdl);
    log()->info("User {} operate device", user->user_);

    auto device_ip = QueryDeviceIPByID(json["DeviceID"].asString());
    if (device_ip.empty()) {
        throw_error(error::not_find_gateway_device);
    }
    auto direction = json["PassDirection"].asInt();
    std::string plate_number = json["PlateNumber"].asString();
    auto diretion = json["PassDirection"].asInt();
    if (direction == PassIn) {
        led_show_plate_number(device_ip, plate_number);
    }
    else if (direction == PassOut) {
        auto charge = json["Charge"].asDouble();
        if (charge == 0) {
            led_show_charge(device_ip, charge, led_show_charge_time);
        }
        else {
            led_show_charge(device_ip, charge, boost::none);
        }
    }
    else {
        throw_error(error::invalid_direction);
    }
    if (json["OpenGate"].asInt() == 1) {
        operate_gateway(device_ip, dm::bgc_open, dm::bgm_auto);
    }
    Json::Value response;
    response["CmdType"] = "OperateDeviceResponse";
    response["Result"] = RESPONSE_OK;
    user_server_->send_response(hdl, response);
}

void Transaction::on_lock_device(const Json::Value& json,
                                 UserTerminalServer::connection_hdl hdl) {
    auto user = user_server_->find_user(hdl);
    log()->info("User {} lock device", user->user_);

    auto device_ip = QueryDeviceIPByID(json["DeviceID"].asString());
    if (device_ip.empty()) {
        throw_error(error::not_find_gateway_device);
    }
    log()->info("Lock device {}", device_ip);
    lock_device(device_ip);

    Json::Value response;
    response["CmdType"] = "LockDeviceResponse";
    response["Result"] = RESPONSE_OK;
    user_server_->send_response(hdl, response);
}



void Transaction::on_esc_record(const Json::Value& json,
                                UserTerminalServer::connection_hdl hdl) {
    auto user = user_server_->find_user(hdl);
    log()->info("User {} record esc", user->user_);

    auto match_nids = [json]() {
        auto size = json["MatchNids"].size();
        std::ostringstream ostr;
        if (size > 0) {
            for(int i=0; i<size-1; i++) {
                ostr << json["MatchNids"][i].asString() <<",";
            }
            ostr << json["MatchNids"][size-1].asString();
        }
        return ostr.str();
    };

    RecordUnusualRecord(user->user_,
                        json["Time"].asString(),
                        json["LeaveNid"].asString(),
                        match_nids(),
                        json["CaclFee"].asDouble());

    Json::Value response;
    response["CmdType"] = "EscRecordResponse";
    response["Result"] = RESPONSE_OK;
    user_server_->send_response(hdl, response);
}
//	add by shenzixiao 2016/3/16
void Transaction::led_show_new_client(const string& device_ip)
{
	try {
		log()->error(" new client {} notify  LED   ", device_ip);
		auto device = QueryDeviceByIP(device_ip);

		if (device != nullptr)
		{
			unlock_device(device->ip);
			std::string greeting;
			if (device->type == ParkEntrance) {
				greeting = QueryEntranceGreeting();
			}
			else if (device->type == ParkExit) {
				greeting = QueryExitGreeting();
			}
			if (!greeting.empty()) {
				// 在LED上显示祝福语
				led_show_greeting(device->ip, greeting);
			}
			else
			{
				notify_remain_parking_to_all_device();	//	显示剩余车尾数
			}
		}
		//	add by shenzixiao 2016/4/6  有等待车辆放行
		auto parking_number = QueryParkingNumber();

		std::vector<WaitVehicleInfo> wait_vec;
		if (QueryWaitVehicleInfoRes(wait_vec))
		{
			if (!wait_vec.empty())
			{
				log()->info("wait_vehicle_list  size{} ", wait_vec.size());
				
				 
				log()->info("real_remain_parking={} ", parking_number.real_remain_parking);
				if (parking_number.real_remain_parking > 0 || parking_number.real_remain_parking == 0)
				{


					// 寻找时间最近的入口过车纪录
					auto wait_vehicle = std::max_element(wait_vec.begin(),
						wait_vec.end(),
						[](const WaitVehicleInfo& l,
						const WaitVehicleInfo& r){
						return l.start_wait_time < r.start_wait_time;
					});
					auto_open_entrance_gateway(wait_vehicle->device_ip,
						wait_vehicle->wait_vehicle_plate_number,
						wait_vehicle->wait_vehicle_plate_type);
					DeleteWaitVehicleByDevice_ip(wait_vehicle->device_ip);
				}
			}
		}
		else if (parking_number.real_remain_parking == 0 || parking_number.real_remain_parking < 0)
		{
			log()->info("wait_vehicle_list  size 0，no wait car", device_ip);
			operate_gateway(device_ip, dm::bgc_keep_close, dm::bgm_manual);
		}
		else
		{
			log()->info("wait_vehicle_list  size 0，{} send  6", device_ip);
			operate_gateway(device_ip, dm::bgc_cancle_close, dm::bgm_manual);
		}
		//	add end 
	}
	catch (const std::exception& ex) {
		log()->error("new client notify Showing  is error: {}",
			ex.what());
	}
}
//	add end
void Transaction::new_client_notify(dts::client_type client) {
	
	
    boost::unique_lock<boost::mutex> lock(dts_clients_mtx_);

    log()->info("new dts client {}", dts::client_address(client));
	
    // remove same ip connection
    dts::clients_remove_client_if(dts_clients_, [client](dts::client_type c) {
            string ip;
            try {
                ip = dts::client_address(c);
            }
            catch( std::exception& err) {
                log()->info("get client address exception {}", err.what());
                return true;
            }
            return ip == dts::client_address(client);
        });
    // auto c = dts::clients_find_client(dts_clients_, dts::client_address(client));
    // if (c) {
    //     dts_clients_ = dts::clients_remove_client(dts_clients_, c);
    // }
    dts_clients_ = dts::clients_add_client(dts_clients_, client);
	//	add by shenzixiao 2016/3/16
	led_show_new_client_thd_ = boost::thread(std::bind(&Transaction::led_show_new_client, this, dts::client_address(client)));
	//	add end 
}

dts::client_type Transaction::find_dts_client(const string &ip)
{
    boost::unique_lock<boost::mutex> lock(dts_clients_mtx_);
    return dts::clients_find_client(dts_clients_, ip);
}

void Transaction::remove_dts_client(dts::client_type client)
{
    boost::unique_lock<boost::mutex> lock(dts_clients_mtx_);
    dts_clients_ = dts::clients_remove_client(dts_clients_, client);
}


void Transaction::on_device_keepalive(const string& source, const string& devid, time_t t)
{
    log()->info("device {} from {} keepalive, time: {}", devid, source, t);
    for(auto& dev : device_list_) {
        if (dev->ip == source) {
            dev = device_keepalive(dev, t);
        }
    }
}

class http_error : public std::runtime_error
{
  public:
    http_error(int code, const string& what) : runtime_error(what) {
        this->code = code;
    }
    int code;
};

inline void raise_http_error(int code, const string& reason) {
    throw http_error(code, reason);
}

string content_type_get_boundary(const string& value) {
    auto pos = value.find("boundary=");
    if (pos == string::npos) {
        raise_http_error(400, "no boundary");
    }
    auto end = value.find(';', pos+9);
    return value.substr(pos+9, end);
}

vector<string> make_vector_string(const string& str, const vector<string>& rest) {
    vector<string> vec;
    vec.push_back(str);
    vec.insert(vec.end(), rest.begin(), rest.end());
    return vec;
}
vector<string> string_split(const string& src, const string& delim) {
    if (src.empty()) {
        return vector<string>();
    }
    auto pos = src.find(delim);
    if (pos == string::npos) {
        return vector<string>();
    }
    return make_vector_string(src.substr(0, pos), string_split(src.substr(pos+delim.size()), delim));
}

void Transaction::http_server_picture_handler(Response& response,
                                              std::shared_ptr<Request> request) {
    try {
        string pattern = request->path_match[0];
        log()->info("Request http picture {}", pattern);

        std::string filename;
        std::vector<std::string> segments;
        boost::split(segments, pattern, boost::is_any_of("/"));
        if (segments.size() >= 4) {
            if (segments[1] == "PassVehicle") {
                filename = opts_->pic_path + segments[2] + "/" + segments[3];
            }
            else if (segments[1] == "Voucher") {
                filename = opts_->voucher_pic_path + segments[2] +
                           "/" + segments[3];
            }
            else {
                // 404 error
                std::string content = "Resource " + pattern + " not found";
                response << "HTTP/1.1 404 Resource not found\r\nContent-Length: " <<
                        content.length() << "\r\n\r\n" << content;
                return;
            }
        }
        else {
            // 400 error
            std::string content = "The requested pattern " + pattern + " was invalid";
            response << "HTTP/1.1 400 Bad Request\r\nContent-Length: " <<
                    content.length() << "\r\n\r\n" << content;
            return;
        }

        std::ifstream file;
        file.open(filename.c_str(), std::ios::in|std::ios::binary);
        if (!file) {
            // 404 error
            std::string content = "Resource " + filename + " not found";
            response << "HTTP/1.1 404 Resource not found\r\nContent-Length: " <<
                    content.length() << "\r\n\r\n" << content;
            return;
        }
        auto file_gud = makeGuard([&](){
                if (file) {
                    file.close();
                }
            });
        file.seekg(0, std::ios::end);
        size_t length=file.tellg();
        file.seekg(0, std::ios::beg);

        response << "HTTP/1.1 200 OK\r\nContent-Length: " << length << "\r\n\r\n";

        if (length > 0) {
            //read and send 128 KB at a time
            size_t buffer_size = 131072;
            vector<char> buffer;
            buffer.resize(buffer_size);
            size_t read_length;
            while ((read_length = file.read(&buffer[0], buffer_size).gcount()) > 0) {
                response.write(&buffer[0], read_length);
                response.flush();
            }
        }
    }
    catch(const std::exception &e) {
        log()->error("http server handles picture of pass vehicle exception {}", e.what());
        response << "HTTP/1.1 500 Internal Error" << "\r\n\r\n";
    }
}

void Transaction::http_server_handler(Response& response, std::shared_ptr<Request> request)
{
    string devid = request->path_match[1];
    string method = request->path_match[2];

    std::stringstream ss;
    request->content >> ss.rdbuf();
    string content=ss.str();

    try {
        log()->info("Transaction::http_server_handler");
	
        if (method == "Keepalive") {
            on_device_keepalive(request->remote_endpoint_address, devid, dm::json_parse_keepalive(content));
            response << "HTTP/1.1 201 created\r\nContent-Length: 0\r\n\r\n";
        }
        if (method == "Datas") {
            log()->info("accept new pass vehicle");
            auto iter = request->header.find("Content-Type");
            if (iter==request->header.end()) {
                raise_http_error(400, "not find content type");
            }
            auto content_type = iter->second;
            auto bound = content_type_get_boundary(content_type);
            log()->info("parse data bound: {}", bound);

            auto strvec = string_split(content, "--"+bound);

            log()->debug(util::debug_string_length(strvec));
            if (strvec.size() < 1) {
                raise_http_error(400, "parse body error");
            }
            string type = "Content-Type: application/json;charset=UTF-8\r\n";
            auto pos = strvec[1].find(type);
            if (pos == string::npos) {
                raise_http_error(400, "json msg no header");
            }
            auto data = dm::json_parse_device_data(strvec[1].substr(pos+type.length()));

            vector<string> pics;
            if (!data.is_pic_url) {
                for(int i=2; i<strvec.size(); ++i) {
                    string type = "Content-Type: image/jpeg\r\n";
                    auto pos = strvec[i].find(type);
                    if (pos != string::npos) {
                        pics.push_back(strvec[i].substr(pos+type.length()));
                    }
                }
                if (pics.size() != data.pic_count) {
                    raise_http_error(400, "pic size != pic_count");
                }
            }
            std::async(std::launch::async,
                       &Transaction::on_device_data, this,
                       request->remote_endpoint_address, devid, data, pics);
            //on_device_data(request->remote_endpoint_address, devid, data, pics);
            response << "HTTP/1.1 201 created\r\nContent-Length: 0\r\n\r\n";
        }
        if (method == "DeviceStatus") {
            on_device_status(request->remote_endpoint_address, devid, dm::json_parse_device_status(content));
            response << "HTTP/1.1 201 created\r\nContent-Length: 0\r\n\r\n";
        }
    }
    catch(dm::parse_error& err) {
        log()->error(format("parse error: %1%") % err.err);
        response << "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
    }
    catch(http_error &err) {
        log()->error(format("http error, code: %1% reason: %2%") % err.code % err.what());
        response << "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
    }
    catch(std::exception &err) {
        log()->error(format("http request failed: %1%") % err.what());
        response << "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
    }
    catch(...) {
        log()->error(format("http request failed: unknown error"));
        response << "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
        throw;
    }
}
//	add by shenzixiao 2016/3/23
/*
*      str为日期字符串
*      formatStr 为时间对应的格式,
*      如2012-07-04 15:33:52对应的格式为%d-%d-%d %d:%d:%d
*/

time_t string2time(const char * str, const char * formatStr)
{
	struct tm tm1;
	int year, mon, mday, hour, min, sec;
	if (-1 == sscanf(str, formatStr, &year, &mon, &mday, &hour, &min, &sec)) return -1;
	tm1.tm_year = year - 1900;
	tm1.tm_mon = mon - 1;
	tm1.tm_mday = mday;
	tm1.tm_hour = hour;
	tm1.tm_min = min;
	tm1.tm_sec = sec;
	return mktime(&tm1);
}
//	add end 
void Transaction::on_device_data(const string& source, const string& devid,
                                 const dm::device_data& data, const vector<string>& pics)
{
    try {
        log()->info("device {} from {} post data, {}", devid, source, dm::json_encode_device_data(data));

        auto device_info = QueryDeviceByIP(source);

        PassVehicle passvehicle(data);
		//	存储管理
        picture_storage_manage(PassVehiclePictureType);

        auto pic_directory = util::PictureDirectory(opts_->pic_path, data.pass_time);
        for(int i=0; i<pics.size(); ++i) {
            string pic_path_name = util::PictureName(device_info->device_id,
                                                 data.pass_time,
                                                 data.pic_type[i])+".jpg";
            string pic_save_path = pic_directory + "/" + pic_path_name;
			//	写图片文件
            util::WriteFile(pic_save_path, pics[i]);
            auto picture_url = str(boost::format("http://%1%:%2%/PassVehicle/%3%/%4%")
                                   % opts_->server_ip % opts_->http_port
                                   % util::GetPictureDirectoryName(pic_directory)
                                   % pic_path_name);
            if (i==0) {
                passvehicle.picture_url1_ = picture_url;
            }
            if (i==1) {
                passvehicle.picture_url2_ = picture_url;
            }
        }
        passvehicle.device_id_ = device_info->device_id;
		//	记录添加到数据库
        auto pass_id = InsertPassVehicleInfo(passvehicle);
        // 非机动车特殊处理
        if (util::IsNoMotorVehicle(passvehicle.plate_number1_)) {
            log()->info("No-Motor vehicle");
            if (passvehicle.pass_direction_ == PassIn) {
                DeleteInnerVehicle(pass_id);
            }
            return;
        }
        if (device_info->type == ParkExit &&
            passvehicle.pass_direction_ == PassIn) {
            // 如果在出口，而车的方向是驶入，则表示此车退回停车场
            log()->info("Vehicle get back to the parking lot at exit");
            // 删除场内表中过车数据
            log()->info("Delete vehicle {} from parking lot at exit",
                        passvehicle.plate_number1_);
            DeleteInnerVehicle(pass_id);
            // 查找收费记录
            auto charge_record = QueryLastestChargeByPlateNumber(passvehicle.plate_number1_);
			
			//	add by shenzixiao 2016/3/23
			//	10分钟之内相同车牌记录删除，并恢复inner_car
			auto record_time_ = string2time(charge_record.record_time.c_str(), "%d-%d-%d %d:%d:%d");
		
			if ((passvehicle.pass_time_ -
				record_time_) / 60>10)
			{
				log()->info("last charge record time is greater than 10 minute, don't delete");
				auto greeting = QueryEntranceGreeting();
				// 如果入口没有配置祝福语，所有入口LED上实时显示剩余车位数
				if (greeting.empty()) {
					notify_remain_parking_to_all_device();
				}
				return;
			}
			//	add end 
            // 删除收费记录
            if (!charge_record.charge_id.empty()) {
                log()->info("Delete charge record about vehicle {}",
                            passvehicle.plate_number1_);
                DeleteChargeRecord(charge_record.charge_id);
            }
            // 在场内表中恢复入口车数据
            if (!charge_record.enter_id.empty()) {
                log()->info("Recover parking info about vehicle {}",
                            passvehicle.plate_number1_);
                InsertInnerVehicle(charge_record.enter_id);
            }
            auto greeting = QueryEntranceGreeting();
            // 如果入口没有配置祝福语，所有入口LED上实时显示剩余车位数
            if (greeting.empty()) {
                notify_remain_parking_to_all_device();
            }
        }
        else if (device_info->type == ParkEntrance &&
                 passvehicle.pass_direction_ == PassOut) {
            // 如果在入口，而车的方向是驶出，则表示此车离开停车场

			//	add by shenzixiao 2016/4/6
			WaitVehicleInfo waitinfo;
			auto findwaitinfo = QueryWaitVehicleByPlate_num(passvehicle.plate_number1_, waitinfo);
			if (findwaitinfo)
			{
				DeleteWaitVehicleByPlate_num(passvehicle.plate_number1_);
			}
			//	add end 
            log()->info("Vehicle do not enter the parking lot at entrance");
            // 删除场内表中过车数据
            log()->info("Delete vehicle {} from parking lot at entrance",
                        passvehicle.plate_number1_);
            DeleteInnerVehicleByPlateNumber(passvehicle.plate_number1_);
            // 显示车牌
            led_show_plate_number(device_info->ip,
                                  passvehicle.plate_number1_);
        }
        else {
            passvehicle.pass_id_ = pass_id;
            auto release_mode = ReleaseFactory::Create(device_info, passvehicle, this);
            if (release_mode != nullptr) {
                release_mode->Release(passvehicle);
            }
		
			//	add by shenzixiao 2016/4/6
			if (passvehicle.pass_direction_ == PassOut)
			{
				log()->info("Vehicle already pass ");
				int judge_ok = QueryJudgeRemainParkingNumber();
				if (judge_ok == 0)
				{
					log()->info("judge_ok== 0");	 
					return;	
				}
				else
				{
					auto parking_number = QueryParkingNumber();
					if (judge_ok == 1)
					{
						log()->info("judge_ok== 1");
						wait_vehicle_release(device_info->ip,
							passvehicle.plate_number1_,
							parking_number.real_remain_parking,
							judge_ok);
						return;
					}
					if (judge_ok == 2)
					{
						log()->info("judge_ok== 2");			
						wait_vehicle_release(device_info->ip,
							passvehicle.plate_number1_,
							parking_number.real_temporary_parking,
							judge_ok);
						return;
					}
				}
			}
			//	add end 
        }
    }
    catch(std::exception& err) {
        log()->error("process device data error: {}", err.what());
        // throw;
    }
}
void Transaction::wait_vehicle_release(const std::string& device_ip,
	const std::string& plate_number,
	int remain_num,
	int bind_type)
{
	log()->info("wait_vehicle_release");
	if (bind_type == 2)
	{
		VIPVehicle vip;
		auto findVIP = QueryVIPVehicle(plate_number, vip);
		if (findVIP)
		{
			return;
		}
	}
	std::vector<WaitVehicleInfo> wait_vec;
	if (QueryWaitVehicleInfoRes(wait_vec))
	{
		if (!wait_vec.empty())
		{
			log()->info("wait_vehicle_list  size{} ", wait_vec.size());
			log()->info("real_remain={} ", remain_num);

			if ((remain_num + 1)> 0)
			{
				// 寻找时间最近的入口过车纪录
				auto wait_vehicle = std::max_element(wait_vec.begin(),
					wait_vec.end(),
					[](const WaitVehicleInfo& l,
					const WaitVehicleInfo& r){
					return l.start_wait_time < r.start_wait_time;
				});
				operate_gateway(wait_vehicle->device_ip, dm::bgc_cancle_close, dm::bgm_manual);
				operate_gateway(wait_vehicle->device_ip, dm::bgc_open, dm::bgm_manual);
				DeleteWaitVehicleByDevice_ip(wait_vehicle->device_ip);
			}
			if ((remain_num + 1) == 0)
			{
				// 寻找时间最近的入口过车纪录
				auto wait_vehicle = std::max_element(wait_vec.begin(),
					wait_vec.end(),
					[](const WaitVehicleInfo& l,
					const WaitVehicleInfo& r){
					return l.start_wait_time < r.start_wait_time;
				});
				operate_gateway(wait_vehicle->device_ip, dm::bgc_cancle_close, dm::bgm_manual);
				operate_gateway(wait_vehicle->device_ip, dm::bgc_open, dm::bgm_manual);
				auto devices = QueryEntranceDeviceList();
				for (auto dev : devices) {
					try {
						operate_gateway(dev->ip, dm::bgc_keep_close, dm::bgm_manual);

						log()->info("=0 bgc_keep_close {}", dev->ip);
					}
					catch (const std::exception& ex) {
						log()->error("Showing real_temporary_parking number is error: {}",
							ex.what());
					}
				}
				DeleteWaitVehicleByDevice_ip(wait_vehicle->device_ip);
			}
		}
	}
}
bool Transaction::notify_pass_vehicle(const Json::Value& msg,
                                      const std::string& device_id) {
    return user_server_->notify_pass_vehicle_to_client(msg, device_id);
}

double Transaction::handle_charge(const PassVehicle& enter_vehicle,
                                  const PassVehicle& exit_vehicle,
                                  Json::Value& json) {
    auto charging = ChargeFactory::Create(enter_vehicle,
                                          exit_vehicle,
                                          json);
    charging->SetFreeTimeMode(QueryFreeTimeMode());
    charging->SetChargePattern(QueryChargePattern());
    charging->SetTranscation(this);
    charging->CalCharge();
    return json["RequireCharge"].asDouble();
}

void Transaction::notify_remain_parking_to_all_device() {
    auto parking_number = QueryParkingNumber();
    auto devices = QueryEntranceDeviceList();
    for (auto dev : devices) {
        try {
            led_show_remain_parking_number(dev->ip,
                                           parking_number.remain_parking);
        }
        catch (const std::exception& ex) {
            log()->error("Showing remain parking number is error: {}",
                         ex.what());
        }
    }
}

void Transaction::handle_entrance_led_show_timer(const boost::system::error_code& e,
                                                 const std::string& device_ip,
                                                 std::shared_ptr<boost::asio::deadline_timer> timer) {
    if (e) {
        log()->warn("Timer of led show at entrance is error, {}",
                     e.message());
        return;
    }
    try {
        log()->debug("Timer of entrance {} led showing is coming", device_ip);
        auto greeting = QueryEntranceGreeting();
        if (greeting.empty()) {
            //通知所有入口LED上实时显示剩余车位数
            notify_remain_parking_to_all_device();
        }
        else {
            // 在LED上显示祝福语
            led_show_greeting(device_ip, greeting);
        }
    }
    catch (const std::exception& ex) {
        log()->error("LED showing at entrace error: {}",
                     ex.what());
    }
}

void Transaction::handle_exit_led_show_timer(const boost::system::error_code& e,
                                             const std::string& device_ip,
                                             std::shared_ptr<boost::asio::deadline_timer> timer) {
    if (e) {
        log()->warn("Timer of led show at exit is error, {}",
                     e.message());
        return;
    }
    try {
        log()->debug("Timer of exit {} led showing is coming", device_ip);
        show_led_after_charge(device_ip);
    }
    catch (const std::exception& ex) {
        log()->error("LED showing at exit error: {}",
                     ex.what());
    }
}

void Transaction::delete_temporary_passrecord(const std::string& pass_id) {
    if (!pass_id.empty()) {
        // 删除场内车辆临时表数据函数
        DeleteTemporaryPassRecord(pass_id);
        log()->debug("Delete temporary record, enter pass: {}",
                     pass_id);
        auto greeting = QueryEntranceGreeting();
        // 如果入口没有配置祝福语，所有入口LED上实时显示剩余车位数
        if (greeting.empty()) {
            notify_remain_parking_to_all_device();
        }
		else{
			//	add by shenzixiao 2016/3/22
			log()->debug("Delete temporary record, greeting: {}",
				greeting);
			//	add end 
		}
    }
}

void Transaction::on_device_status(const string& source,
                                   const string& devid,
                                   const device_status& data) {
    log()->info("device {} from {} reports status, {}", devid, source, dm::json_encode_device_status(data));

    for(auto &dev : device_list_) {
        if (dev->ip == source) {
            dev = device_update_status(dev, data);
        }
    }
}

device_pointer Transaction::find_device(const string& id)
{
    for(auto dev : device_list_) {
        if (dev->device_id == id) {
            return dev;
        }
    }
    return nullptr;
}

void Transaction::dts_operate(const string& device_ip, std::function<Json::Value()> f) {
    auto client = find_dts_client(device_ip);
    if (!dts::client_is_valid(client)) {
        log()->error("Device {} not connected", device_ip);
        throw_error(error::gateway_device_not_connected);
    }

    try {
        auto msg = dm::json_encode_ipnc("", dm::ict_control, f());
        log()->info("dts client send {}", msg);
        auto ret = dts::client_send_recv(client, msg);
        log()->info("dts client recv {}", ret);
    }
    catch(std::exception& err) {
        log()->error("dts client send recv error: {}", err.what());
        log()->info("remove dts client: {}", dts::client_stream(client));
        remove_dts_client(client);
        throw_error(error::operate_gateway_error);
    }
}

void Transaction::operate_gateway(const string& device_ip, int action_type, int mode) {
    dts_operate(device_ip, [=]() {
            Json::Value root;
            root["RoadGate"] = dm::json_encode_road_gate_control(action_type, 2, mode);
            return root;
        });
}

void Transaction::operate_led(const string& device_ip,
                              int action,  // dm::la_display or dm::la_clear
                              const string &content,
                              int color,  // dm::lc_red, dm::lc_green or dm::lc_blue
                              int size,   // undefined
                              int mode,   // dm::lm_fixed or dm::lm_scrolling
                              boost::optional<int> duration) {  // seconds
    dts_operate(device_ip, [=]() {
            log()->info("Send {} to LED", content);
            Json::Value root;
            root["Led"] = dm::json_encode_led_control(action, content, color, size, mode, duration);
            return root;
        });
}

void Transaction::lock_device(const string& device_ip)
{
    dts_operate(device_ip, [=]() {
            Json::Value root;
            root["Lock"] = dm::json_encode_lock_action(dm::la_lock);
            return root;
        });
}

void Transaction::unlock_device(const string& device_ip)
{
    dts_operate(device_ip, [=]() {
            Json::Value root;
            root["Lock"] = dm::json_encode_lock_action(dm::la_unlock);
            return root;
        });
}
void StringSplit(string s, char splitchar, vector<string>& vec)
{
	if (vec.size()>0)//保证vec是空的  
		vec.clear();
	int length = s.length();
	int start = 0;
	for (int i = 0; i<length; i++)
	{
		if (s[i] == splitchar && i == 0)//第一个就遇到分割符  
		{
			start += 1;
		}
		else if (s[i] == splitchar)
		{
			vec.push_back(s.substr(start, i - start));
			start = i + 1;
		}
		else if (i == length - 1)//到达尾部  
		{
			vec.push_back(s.substr(start, i + 1 - start));
		}
	}
}
void Transaction::auto_open_entrance_gateway(const std::string& device_ip,
                                             const std::string& plate_number,
											 int plate_type) {
    try {
        led_show_plate_number(device_ip, plate_number);
		//operate_gateway(device_ip, dm::bgc_open, dm::bgm_auto);//	add by shenzixiao 2016/3/28
		log()->info("limit_platetype={}", plate_type);
		auto limit_platetype=QueryLimitPlateType();
		vector<string>  vec;
		StringSplit(limit_platetype, ';', vec);
		for (auto it = vec.begin(); it != vec.end(); it++) {
			log()->info("QueryLimitPlateType limit_platetype={}", std::atoi(it->c_str()));
			if (plate_type == std::atoi(it->c_str()))
			{
				log()->info("match return ");
				return;
			}
		}
		//	add by shenzixiao 2016/4/7
		int judge_ok = QueryJudgeRemainParkingNumber();
		if (judge_ok == 0)	//不启用，直接开闸
		{ 
			log()->info("judge_ok== 0");
			operate_gateway(device_ip, dm::bgc_open, dm::bgm_auto);
		}
		else
		{
			auto parking_number = QueryParkingNumber();
			if (judge_ok == 1)
			{
				log()->info("judge_ok== 1");
				remain_parking_number_bind(device_ip, plate_number, parking_number.real_remain_parking, judge_ok);
			}
			if (judge_ok == 2)
			{
				log()->info("judge_ok== 2");
				remain_parking_number_bind(device_ip, plate_number, parking_number.real_temporary_parking, judge_ok);
			}
		}
		//	add end 
    }
    catch (...) {
        log()->error("Auto open entrance gateway exception");
    }
}
void Transaction::remain_parking_number_bind(const std::string& device_ip,
											 const std::string& plate_number,
											 int remain_num,
											 int bind_type)
{
	log()->info("remain_parking_number_bind");
	if (bind_type==2)
	{
		VIPVehicle vip;
		auto findVIP = QueryVIPVehicle(plate_number, vip);
		if (findVIP)
		{
			operate_gateway(device_ip, dm::bgc_cancle_close, dm::bgm_manual);
			operate_gateway(device_ip, dm::bgc_open, dm::bgm_auto);
			if (remain_num > 0)
			{
				return;
			}
			else
			{
				operate_gateway(device_ip, dm::bgc_keep_close, dm::bgm_manual);
				return;
			}
		}
	}
	if (remain_num == 0)
	{
		log()->info("=0 the {} receive a record{} ,remain_num  is {}", device_ip, plate_number, remain_num);
		std::vector<WaitVehicleInfo> wait_vec;
		if (QueryWaitVehicleInfoRes(wait_vec))
		{
			log()->info("=0 {}---send 5  to  6 ", device_ip);
			operate_gateway(device_ip, dm::bgc_cancle_close, dm::bgm_manual);
		}
		operate_gateway(device_ip, dm::bgc_open, dm::bgm_auto);

		auto devices = QueryEntranceDeviceList();
		for (auto dev : devices) {
			try {
				led_show_remain_parking_number(dev->ip,
					remain_num);

				operate_gateway(dev->ip, dm::bgc_keep_close, dm::bgm_manual);

				log()->info("=0 bgc_keep_close {}", dev->ip);
			}
			catch (const std::exception& ex) {
				log()->error("Showing real_temporary_parking number is error: {}",
					ex.what());
			}
		}
		return;
	}
	else if (remain_num < 0)
	{
		log()->info("<0  the {} receive a record {} ,remain_num is {}", device_ip, plate_number, remain_num);
		WaitVehicleInfo waitinfo;
		auto findwaitinfo = QueryWaitVehicleByDevice_ip(device_ip, waitinfo);
		if (findwaitinfo)
		{
			log()->info("<0 {}already have delete", device_ip);
			DeleteWaitVehicleByDevice_ip(device_ip);
		}

		WaitVehicleInfo waitinfotemp{ device_ip, plate_number };
		InsertWaitVehicleInfo(waitinfotemp);
		return;
	}
	else
	{
		log()->info(">0 the {} receive a record {},remain_num is {}", device_ip, plate_number, remain_num);
		std::vector<WaitVehicleInfo> wait_vec;
		if (QueryWaitVehicleInfoRes(wait_vec))
		{
			log()->info(">0 {}---send 5  to  6 ", device_ip);
			operate_gateway(device_ip, dm::bgc_cancle_close, dm::bgm_manual);
			operate_gateway(device_ip, dm::bgc_open, dm::bgm_auto);
		}
		return;
	}
}
void Transaction::auto_open_exit_gateway(const std::string& device_ip) {
    try {
        led_show_charge(device_ip, 0, led_show_charge_time);
        operate_gateway(device_ip, dm::bgc_open, dm::bgm_auto);
    }
    catch (...) {
        log()->error("Auto open gateway exception");
    }
}

void Transaction::led_show_charge(const std::string& device_ip, double charge,
                                  boost::optional<int> duration) {
    {
        boost::unique_lock<boost::mutex> lock(exit_led_timer_mtx_);
        auto& timers = exit_led_timers_[device_ip];
        log()->info("Canncel timer of exit LED:{}", timers.size());
        for(auto timer : timers) {
            timer->cancel();
        }
        timers.clear();
    }

    auto content = str(format("交费%1%") % charge);
    log()->info("LED of device {} show charge:{}", device_ip, content);
    operate_led(device_ip, dm::la_display, content, dm::lc_red, 10, dm::lm_fixed, duration);
    if (duration != boost::none) {
        // 交费信息显示完后显示其它信息
        auto led_show_timer = std::make_shared<boost::asio::deadline_timer>(ios_,
                                                                            boost::posix_time::seconds(*duration));
        {
            boost::unique_lock<boost::mutex> lock(exit_led_timer_mtx_);
            exit_led_timers_[device_ip].push_back(led_show_timer);
        }
        led_show_timer->async_wait(std::bind(&Transaction::handle_exit_led_show_timer,
                                             this,
                                             std::placeholders::_1,
                                             device_ip,
                                             led_show_timer));
    }
}

void Transaction::led_show_remain_parking_number(const std::string& device_ip, int number) {
    auto content = str(format("余位%1%") % number);
    log()->info("LED of device {} show remain parking number:{}",
                device_ip, content);
    operate_led(device_ip, dm::la_display, content, dm::lc_red, 10, dm::lm_fixed, boost::none);
}

void Transaction::led_show_plate_number(const std::string& device_ip,
                                        const std::string& plate_number) {
    {
        boost::unique_lock<boost::mutex> lock(entrance_led_timer_mtx_);
        auto& timers = entrance_led_timers_[device_ip];
        log()->info("Canncel timer of entrance LED:{}", timers.size());
        for(auto timer : timers) {
            timer->cancel();
        }
        timers.clear();
    }
    log()->info("LED of device {} show plate number:{}",
                device_ip, plate_number);
    operate_led(device_ip, dm::la_display, plate_number, dm::lc_red, 10,
                dm::lm_fixed, led_show_platenumber_time);
    // 车牌显示5秒后显示其它信息
    auto led_show_timer = std::make_shared<boost::asio::deadline_timer>(ios_,
                                                                        boost::posix_time::seconds(led_show_platenumber_time));
    {
        boost::unique_lock<boost::mutex> lock(entrance_led_timer_mtx_);
        entrance_led_timers_[device_ip].push_back(led_show_timer);
    }
    led_show_timer->async_wait(std::bind(&Transaction::handle_entrance_led_show_timer,
                                         this,
                                         std::placeholders::_1,
                                         device_ip,
                                         led_show_timer));
}

void Transaction::led_show_greeting(const std::string& device_ip,
                                        const std::string& greeting) {
    log()->info("LED of device {} show greeting:{}", device_ip, greeting);
    operate_led(device_ip, dm::la_display, greeting, dm::lc_red, 10, dm::lm_fixed, boost::none);
}

void Transaction::show_led_after_charge(const std::string& device_ip) {
    auto greeting = QueryExitGreeting();
    if (!greeting.empty()) {
        // 在LED上显示祝福语
        led_show_greeting(device_ip, greeting);
    }
    else {
        // 清屏
        operate_led(device_ip, dm::la_clear, "", dm::lc_red, 0, dm::lm_fixed, 0);
    }
};

void Transaction::delete_passvehicle_pic(const std::vector<PassVehiclePicture>& vehicles) {
    for(auto &vehicle :  vehicles) {
        if (!vehicle.picture_url1_.empty()) {
            string pic_parent_directory;
            string pic_save_path = opts_->pic_path +
                                   util::GetPictureName(vehicle.picture_url1_,
                                                        &pic_parent_directory);
            boost::system::error_code ec;
            auto file_exist = boost::filesystem::exists(pic_save_path, ec);
            if (ec) {
                log()->warn("Checking file[{}] existing error:{}", pic_save_path,
                            ec.message());
            }
            if (file_exist) {
                log()->info("remove file {}", pic_save_path);
                int ret = std::remove(pic_save_path.c_str());
                if (ret!=0) {
                    log()->warn("remove file {} error: {}", pic_save_path, ret);
                }
                else {
                    EmptyPassVehiclePicPath(vehicle.nid, 1);
                    // 删除空文件夹
                    if (!pic_parent_directory.empty()) {
                        util::RemoveEmptyDirectory(opts_->pic_path +
                                             pic_parent_directory);
                    }
                }
            }
            else {
                EmptyPassVehiclePicPath(vehicle.nid, 1);
            }
        }
        if (!vehicle.picture_url2_.empty()) {
            string pic_parent_directory;
            string pic_save_path = opts_->pic_path +
                                   util::GetPictureName(vehicle.picture_url2_,
                                                        &pic_parent_directory);
            boost::system::error_code ec;
            auto file_exist = boost::filesystem::exists(pic_save_path, ec);
            if (ec) {
                log()->warn("Checking file[{}] existing error:{}", pic_save_path,
                            ec.message());
            }
            if (file_exist) {
                log()->info("remove file {}", pic_save_path);
                int ret = std::remove(pic_save_path.c_str());
                if (ret!=0) {
                    log()->warn("remove file {} error: {}", pic_save_path, ret);
                }
                else {
                    EmptyPassVehiclePicPath(vehicle.nid, 2);
                    // 删除空文件夹
                    if (!pic_parent_directory.empty()) {
                        util::RemoveEmptyDirectory(opts_->pic_path +
                                             pic_parent_directory);
                    }
                }
            }
            else {
                EmptyPassVehiclePicPath(vehicle.nid, 2);
            }
        }
    }
};

void Transaction::clear_expired_vehicle_pic(int expiry) {
    log()->info("Clear expired vehicle pictures, time expiration:{}", expiry);
    delete_passvehicle_pic(QueryPassVehiclePicNeedClear(expiry));
}

void Transaction::clear_earliest_day_vehicle_pic() {
    log()->info("Clear vehicle pictures on earliest day");
    delete_passvehicle_pic(QueryEarliestPassVehiclePicNeedClear());
}

void Transaction::clear_vehicle_pic_by_counts() {
    log()->info("Clear 20% vehicle pictures");
    auto counts = GetPassVehiclePictureCounts();
    delete_passvehicle_pic(QueryPassVehiclePicByCounts(counts * 0.2));
}

void Transaction::delete_voucher_pic(const std::vector<ChargeVoucherPicture>& vouchers) {
    for (auto& voucher : vouchers) {
        if (!voucher.picture_url.empty()) {
            string pic_parent_directory;
            string pic_save_path = opts_->voucher_pic_path +
                                   util::GetPictureName(voucher.picture_url,
                                                        &pic_parent_directory);
            boost::system::error_code ec;
            auto file_exist = boost::filesystem::exists(pic_save_path, ec);
            if (ec) {
                log()->warn("Checking file[{}] existing error:{}", pic_save_path,
                            ec.message());
            }
            if (file_exist) {
                log()->info("remove file {}", pic_save_path);
                int ret = std::remove(pic_save_path.c_str());
                if (ret!=0) {
                    log()->warn("remove file {} error: {}", pic_save_path, ret);
                }
                else {
                    EmptyVoucherPicPath(voucher.nid);
                    // 删除空文件夹
                    if (!pic_parent_directory.empty()) {
                        util::RemoveEmptyDirectory(opts_->voucher_pic_path +
                                             pic_parent_directory);
                    }
                }
            }
            else {
                EmptyVoucherPicPath(voucher.nid);
            }
        }
    }
};

void Transaction::clear_expired_voucher_pic(int expiry) {
    log()->info("Clear expired voucher pictures, time expiration:{}", expiry);
    delete_voucher_pic(QueryVoucherPicNeedClear(expiry));
}

void Transaction::clear_earliest_day_voucher_pic() {
    log()->info("Clear voucher pictures on earliest day");
    delete_voucher_pic(QueryEarliestVoucherPicNeedClear());
}

void Transaction::clear_voucher_pic_by_counts() {
    log()->info("Clear 20% voucher pictures");
    auto counts = GetVoucherPictureCounts();
    delete_voucher_pic(QueryVoucherPicByCounts(counts * 0.2));
}

void Transaction::clear_pic(const boost::system::error_code& error)
{
    using std::placeholders::_1;
    if (!error) {
        log()->info("Transaction::clear_pic run one");
        auto picture_expiry = QueryPictureExpiry();
        if (picture_expiry > 0) {
            clear_expired_vehicle_pic(picture_expiry);
            clear_expired_voucher_pic(picture_expiry);
        }
    }
    timer_.expires_from_now(boost::posix_time::seconds(util::SecondsToNextDay()));
    timer_.async_wait(std::bind(&Transaction::clear_pic, this, _1));
}

void Transaction::dts_heartbeat(const boost::system::error_code& error)
{
    using std::placeholders::_1;
    if (!error) {
        // log()->trace("Transaction::dts_heartbeat run one");

        // make copy of clients
        auto clients = dts::make_clients();
        {
            boost::unique_lock<boost::mutex> lock(dts_clients_mtx_);
            *clients = *dts_clients_;
        }

        // remove heartbeat failed client
        clients = dts::clients_remove_client_if(clients, [](dts::client_type client) {
                try {
                    auto msg = dm::json_encode_ipnc("", dm::ict_query, Json::Value());
                    log()->debug("dts client send heartbeat {}", msg);
                    auto ret = dts::client_send_recv(client, msg);
                    log()->debug("dts client recv heartbeat return {}", ret);
                    // auto parsed_res = dm::json_parse_ipnc_response(ret);
                    // std::cout << "ret: " << parsed_res.first << ", desc: " << parsed_res.second << std::endl;
                }
                catch(std::exception& err) {
                    log()->error("dts client send recv error: {}", err.what());
                    log()->info("remove dts client: {}", dts::client_stream(client));
                    return true;
                }
                return false;
            });

        // set clients
        boost::unique_lock<boost::mutex> lock(dts_clients_mtx_);
        dts_clients_ = clients;
    }

    dts_timer_.expires_from_now(boost::posix_time::seconds(3));
    dts_timer_.async_wait(std::bind(&Transaction::dts_heartbeat, this, _1));
}

void Transaction::picture_storage_manage(PictureType pic_type) {
    log()->info("Detecting storage of picture");
    string directory;
    if (pic_type == PassVehiclePictureType) {
        directory = opts_->pic_path;
    }
    else {
        directory = opts_->voucher_pic_path;
    }

    boost::system::error_code ec;
    auto info = boost::filesystem::space(directory, ec);// 得到指定路径下的空间信息
    if (ec) {
        log()->error("Get space of directory[{}] error: {}", directory, ec.message());
        return;
    }
    log()->debug("Capacity:{}  Free space:{} of directory[{}]", info.capacity,
                 info.available, directory);
    auto capacity = QueryStoreCapacity();
    log()->trace("Max capacity of storing pictures is {}", capacity);
    if (capacity > 0) {
        if (info.capacity - info.available > capacity * 1048576) {
            log()->warn("The picture capacity is over max capacity, start to clean");
            if (pic_type == PassVehiclePictureType) {
                clear_earliest_day_vehicle_pic();
            }
            else {
                clear_earliest_day_voucher_pic();
            }
        }
    }
    // 小于5M时，删除20%图片
    if (info.available < 5242880) {
        log()->warn("The spcace of file system [{}] is not enough", info.available);
        clear_vehicle_pic_by_counts();
        clear_voucher_pic_by_counts();
    }
}

void throw_error(error::value v)
{
    throw std::system_error(static_cast<int>(v), error::get_category());
}

}
