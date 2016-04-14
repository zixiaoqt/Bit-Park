#ifndef _PARKINGSERVER_DATABASE_
#define _PARKINGSERVER_DATABASE_
#include "user.h"
#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>
#include <json/json.h>
#include <mysql++.h>
#include <vector>
#include <map>
#include "singleton.h"
#include "mysqlconnectionpool.h"
#include "log.h"
#include "user.h"
#include "gateway.h"
#include "device.h"
#include "passvehicle.h"
#include "parkingvehicle.h"
#include "chargerule.h"
#include "error.h"

namespace parkingserver {

std::map<std::string, std::string> QuerySysConfig();

int QueryChargePattern();

int QueryFreeTimeMode();

int QueryPictureExpiry();

int QueryStoreCapacity();

std::string QueryEntranceGreeting();

std::string QueryExitGreeting();
// + add by shenzixiao 2016/3/28 查询是否启用余位判断
int QueryJudgeRemainParkingNumber();
// + add end

// + add by shenzixiao 2016/4/16 查询限制车牌类型
std::string QueryLimitPlateType();
// + add end

// + add by zhengweixue 20160118 查询数据中中白名单匹配模式
int QueryWhitelistMatchMode(); 
// + add end

// 查询用户
std::shared_ptr<User> QueryUser(const std::string& userid);

// 查询出入口
Json::Value QueryGateway();

// 查询设备
Json::Value QueryDevice();

// 根据绑定IP查询设备
Json::Value QueryDeviceByBindIP(const std::string& bind_ip);

std::list<device_pointer> QueryDeviceList();

std::list<device_pointer> QueryEntranceDeviceList();

std::list<device_pointer> QueryDeviceListByBindIP(const std::string& bind_ip);

// 根据设备ID查询设备信息
device_pointer QueryDeviceByID(const std::string& id);

// 根据设备IP查询设备信息
device_pointer QueryDeviceByIP(const std::string& ip);

// 根据过车记录ID查询设备
device_pointer QueryDeviceByPassID(const std::string& pass_id);

// 根据设备IP查询设备ID
std::string QueryDeviceIDByIP(const std::string& ip);

// 根据设备ID查询设备IP
std::string QueryDeviceIPByID(const std::string& id);

Json::Value QueryParkingVehicle(const Json::Value& json);

// 根据车牌号查询入口车辆通行纪录
std::vector<PassVehicle> QueryEnterVehicle(const std::string& exit_vehicle_plate);

PassVehicle QueryPassVehicleByID(const std::string& pass_id);

struct VIPVehicle {
    std::string id;
    std::string plate_number;
    std::string vehicle_type;
    int type = 0;
    std::time_t expiry_date = 0;
    double remain_money = 0;
    std::string delete_status;
    std::string freeze_status;
    std::time_t freeze_date = 0;
    std::time_t valid_begintime = 0;
    double discount = 1;
};

// 查询VIP信息
bool QueryVIPVehicle(const std::string& plate_number, VIPVehicle& vip);

// + add by zhengweixue 20160118  
// 忽略省份信息，模糊查询VIP车辆信息
bool QueryVIPVehicle_IgnoreProvinceMatch(const std::string& plate_number, std::vector<VIPVehicle>& vip_vec);

// 根据号牌查询忽略省份信息的VIP车辆记录
std::vector<VIPVehicle> QueryVIPVehicleRecord_IgnoreProvince(const std::string& plate_number);

// 模糊查询满足只有一位字符不匹配VIP信息
bool QueryVIPVehicle_FuzzyMatch(const std::string& plate_number, std::vector<VIPVehicle>& vip_vec);

// 根据号牌查询忽略省份信息的VIP车辆记录
std::vector<VIPVehicle> QueryVIPVehicleRecord_FuzzyMatch(const std::string& plate_number);
// + add end

// 根据通行纪录ID查询车牌
std::string QueryPlateNumber(const std::string& pass_id);

int QueryParkPosition(const std::string& device_id);

bool QueryChargeRule(const std::string& date, int park_position, ChargeRule& rule);

bool QueryMinPriorityRule(int park_position, ChargeRule& rule);

struct ParkingNumber {
    int total_parking = 0;
    int remain_parking = 0;
	int real_remain_parking = 0;	//	add by shenzixiao 2016/3/31 真实的余位数可为负值
	int real_temporary_parking = 0;	//	add by shenzixiao 2016/4/6  真实的临时车余位数可为负值
	int rectify_count=0;	//	add by shenzixiao 2016/4/14	矫正余位数

};

ParkingNumber QueryParkingNumber();

struct TaxiInfo {
    std::string plate_regex_pattern;
};

std::vector<TaxiInfo> QueryTaxiInfo();

void InsertShiftLog(const Json::Value& shift);
/*
等待车辆表
*/
//	add by shenzixiao 2016/4/6
struct WaitVehicleInfo {
	std::string   device_ip;
	std::string   wait_vehicle_plate_number;
	int wait_vehicle_plate_type;
	time_t start_wait_time;
};
void InsertWaitVehicleInfo(const WaitVehicleInfo& info);
std::vector<WaitVehicleInfo> QueryWaitVehicleInfo();
bool QueryWaitVehicleInfoRes(std::vector<WaitVehicleInfo>& wait_vec);

bool QueryWaitVehicleByDevice_ip(std::string device_ip, WaitVehicleInfo& info);
void DeleteWaitVehicleByDevice_ip(std::string device_ip);

bool QueryWaitVehicleByPlate_num(std::string plate_number, WaitVehicleInfo& info);
void DeleteWaitVehicleByPlate_num(std::string plate_number);
void DeleteWaitVehicleInfo();
//	add end
struct ManualGatewayOperationInfo {
    std::string pass_id;
    std::string device_id;
    int action_type;
    std::string user_id;
};

void InsertGatewayOperationLog(const ManualGatewayOperationInfo& info);

std::string InsertPassVehicleInfo(const PassVehicle& info);

struct ChargeRecord {
    std::string exit_id;
    std::string enter_id;
    float charge = 0.0;
    float actual_charge = 0.0;
    float discount = 1.0;
    int pay_type = 1;
    std::string user;
    int parking_type = 0;
    int exit_type = 1;
    std::string exit_comment;
    std::string charge_detail;
    boost::optional<float> remain_money;
};

std::string GenerateChargeRecordID();

void InsertChargeRecord(const std::string& nid,
                        const ChargeRecord& record);

void UpdateVoucherPictureURL(const std::string& nid,
                              const std::string& picture_url);

void ModifyPlateNumber(const string& pass_id,
                       const string& plate_number,
                       const string& user);

void DeleteTemporaryPassRecord(const string& pass_id);

void UpdateStoredValue(const string& vip_id, float remain_money);

/* void UpdateRemainParkingNumber(int remain_parkingnumber); */

/* void DecreaseRemainParkingNumber(); */

/* void IncreaseRemainParkingNumber(); */

struct PassVehiclePicture {
    std::string nid;
    std::string picture_url1_;
    std::string picture_url2_;
};

std::vector<PassVehiclePicture> QueryPassVehiclePicNeedClear(int expiry);
std::vector<PassVehiclePicture> QueryEarliestPassVehiclePicNeedClear();
std::vector<PassVehiclePicture> QueryPassVehiclePicByCounts(unsigned int counts);

void EmptyPassVehiclePicPath(const string & nid, int pic_id);

struct ChargeVoucherPicture {
    std::string nid;
    std::string picture_url;
};

std::vector<ChargeVoucherPicture> QueryVoucherPicNeedClear(int expiry);
std::vector<ChargeVoucherPicture> QueryEarliestVoucherPicNeedClear();
std::vector<ChargeVoucherPicture> QueryVoucherPicByCounts(unsigned int counts);

void EmptyVoucherPicPath(const string & nid);

unsigned int GetPassVehiclePictureCounts();

unsigned int GetVoucherPictureCounts();

struct SimpleChargeRecord {
    std::string charge_id;
    std::string enter_id;
	std::string record_time;	//	add by shenzixiao 2016/3/23
};

SimpleChargeRecord QueryLastestChargeByPlateNumber(const string& plate_number);

void DeleteChargeRecord(const std::string& charge_id);

void InsertInnerVehicle(const string& nid);

void DeleteInnerVehicle(const std::string& nid);

void DeleteInnerVehicleByPlateNumber(const std::string& plate_number);

void RecordUnusualRecord(const string& op, const string& op_time, const string& leave_nid, const string& match_nids, double cacl_fee);

template <typename T>
struct function_traits
        : public function_traits<decltype(&T::operator())>
{};

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const>
{
    typedef ReturnType result_type;
};

template<typename F>
typename function_traits<typename std::remove_reference<F>::type>::result_type
UniveralOperateDB(const std::string& sql, F&& f) {
    try {
        mysqlpp::ScopedConnection cp(*Singleton<MysqlConnectionPool>::GetInstance(),
                                     true);
        if (!cp) {
            log()->error("Failed to get a sql connection");
            throw std::system_error(error::database_connection_error,
                                    error::get_category());
        }
        else {
            log()->debug("SQL: {}", sql);
            mysqlpp::Query query(cp->query(sql));
            return f(query.store());
        }
    }
    catch (mysqlpp::Exception& e) {
        log()->error("SQL execute exception: {}", e.what());
        throw std::system_error(error::sql_execute_exception,
                                error::get_category());
    }
}

}
#endif
