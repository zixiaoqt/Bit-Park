#include "database.h"
#include <boost/format.hpp>
#include <boost/date_time.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string.hpp>
#include "passvehicle.h"
#include <vector>
#include <list>
#include <memory>
#include <algorithm>
// #include <locale>
#include <cwctype>
#include <boost/locale/encoding_utf.hpp>
#include "util.h"
namespace parkingserver {
using namespace boost::algorithm;
// using namespace boost::posix_time;

using boost::posix_time::duration_from_string;
using boost::str;
using boost::format;

inline std::string GenerateUUID() {
    boost::uuids::random_generator generator;
    boost::uuids::uuid uuid = generator();
    auto suuid = boost::uuids::to_string(uuid);
    boost::erase_all(suuid, "-");
    return suuid;
}

std::map<std::string, std::string> QuerySysConfig() {
    auto sql = "select * from sys_config";
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
            std::map<std::string, std::string> config;
            if (res) {
                for (auto& row : res) {
                    if (row["config_state"].is_null() ||
                        int(row["config_state"]) == 1) {
                        config[row["config_key"].c_str()] = row["config_value"].c_str();
                    }
                }
            }
            return config;
        });
}

int QueryChargePattern() {
    auto sql = "select * from sys_config where config_key='charge_pattern'";
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
            int mode = 0;
            if (res && res.num_rows() == 1) {
                if (res[0]["config_state"].is_null() ||
                    int(res[0]["config_state"]) == 1) {
                    if (!res[0]["config_value"].is_null() &&
                        !res[0]["config_value"].empty()) {
                        mode = int(res[0]["config_value"]);
                    }
                }
            }
            return mode;
        });
}

int QueryFreeTimeMode() {
    auto sql = "select * from sys_config where config_key='freetime_mode'";
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
            int mode = 0;
            if (res && res.num_rows() == 1) {
                if (res[0]["config_state"].is_null() ||
                    int(res[0]["config_state"]) == 1) {
                    if (!res[0]["config_value"].is_null() &&
                        !res[0]["config_value"].empty()) {
                        mode = int(res[0]["config_value"]);
                    }
                }
            }
            return mode;
        });
}

auto query_intvalue_fun = [](const mysqlpp::StoreQueryResult& res)->int{
    if (res && res.num_rows() == 1) {
        if (res[0]["config_state"].is_null() ||
            int(res[0]["config_state"]) == 1) {
            if (!res[0]["config_value"].is_null()) {
                return int(res[0]["config_value"]);
            }
        }
    }
    return 0;
};

int QueryPictureExpiry() {
    auto sql = "select * from sys_config where config_key='picture_expiry'";
    return UniveralOperateDB(sql, query_intvalue_fun);
}

int QueryStoreCapacity() {
    auto sql = "select * from sys_config where config_key='store_capacity'";
    return UniveralOperateDB(sql, query_intvalue_fun);
}

auto query_greeting_fun = [](const mysqlpp::StoreQueryResult& res)->std::string{
    if (res && res.num_rows() == 1) {
        if (res[0]["config_state"].is_null() ||
            int(res[0]["config_state"]) == 1) {
            if (!res[0]["config_value"].is_null()) {
                return res[0]["config_value"].c_str();
            }
        }
    }
    return "";
};

std::string QueryEntranceGreeting() {
    auto sql = "select * from sys_config where config_key='enter_greeting'";
    return UniveralOperateDB(sql, query_greeting_fun);
}

std::string QueryExitGreeting() {
    auto sql = "select * from sys_config where config_key='exit_greeting'";
    return UniveralOperateDB(sql, query_greeting_fun);
}
// + add by shenzixiao 2016/3/28  查询是否启用余位判断
int QueryJudgeRemainParkingNumber() {
	auto sql = "select * from sys_config where config_key='judge_remain_parking_number'";
	return UniveralOperateDB(sql, query_intvalue_fun);
}
// + add end

// + add by shenzixiao 2016/4/14  查询限制车牌类型
std::string QueryLimitPlateType() {
	auto sql = "select * from sys_config where config_key='limit_platetype'";
	return UniveralOperateDB(sql, query_greeting_fun);
}
// + add end


// + add by zhengweixue 20160118 查询白名单匹配模式
int QueryWhitelistMatchMode() {
	auto sql = "select * from sys_config where config_key='whitelist_match_mode'";
	return UniveralOperateDB(sql, query_intvalue_fun);
}
// + add end
std::shared_ptr<User> QueryUser(const std::string& userid) {
    auto sql = str(format("select * from park_user_info where userid='%1%'") % userid);
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
            std::shared_ptr<User> user;
            if (res && res.num_rows() == 1) {
                user = std::make_shared<User>();
                user->user_ = res[0]["userid"].c_str();
                user->password_ = res[0]["password"].c_str();
                user->name_ = res[0]["username"].c_str();
                if (!res[0]["phone"].is_null()) {
                    user->phone_ = res[0]["phone"].c_str();
                }
                if (!res[0]["roles"].is_null()) {
                    user->authority_ = res[0]["roles"].c_str();
                }
                if (!res[0]["description"].is_null()) {
                    user->description_ = res[0]["description"].c_str();
                }
            }
            return user;
        });
}

Json::Value QueryGateway() {
    return UniveralOperateDB("select * from gate_info",
                             [](const mysqlpp::StoreQueryResult& res){
                                 Json::Value root;
                                 if (res) {
                                     for (auto& row : res) {
                                         Json::Value gateway;
                                         gateway["ID"] = row["pointcode"].c_str();
                                         gateway["Name"] = row["pointname"].c_str();
                                         gateway["Type"] = int(row["point_func"]);
                                         gateway["Park"] = row["parkcode"].c_str();
                                         root.append(gateway);
                                     }
                                 }
                                 return root;
                             });
}

auto handle_query_device_json_func = [](const mysqlpp::StoreQueryResult& res){
    Json::Value root;
    if (res) {
        for (auto& row : res) {
            Json::Value dev;
            dev["ID"] = row["devicecode"].c_str();
            dev["Name"] = row["devicename"].c_str();
            dev["Gateway"] = row["pointcode"].c_str();
            dev["Type"] = int(row["videofunc"]);
            dev["IP"] = row["ip"].c_str();
            dev["Port"] = int(row["port"]);
            root.append(dev);
        }
    }
    return root;
};

Json::Value QueryDevice() {
    return UniveralOperateDB("select * from equip_info",
                             handle_query_device_json_func);
}

Json::Value QueryDeviceByBindIP(const std::string& bind_ip) {
    auto sql = str(format("select * from equip_info where bind_ip='%1%'")
                   % bind_ip);
    return UniveralOperateDB(sql, handle_query_device_json_func);
}
auto handle_device_func = [](const mysqlpp::Row& row) {
    auto dev = std::make_shared<device>();
    dev->device_id = row["devicecode"].c_str();
    dev->device_name = row["devicename"].c_str();
    dev->gateway_id = row["pointcode"].c_str();
    if (!row["ip"].is_null()) {
        dev->ip = row["ip"].c_str();
    }
    dev->port = int(row["port"]);
    if (!row["videofunc"].is_null()) {
        dev->type = int(row["videofunc"]);
    }
    if (!row["run_mode"].is_null()) {
        dev->release_mode = int(row["run_mode"]);
    }
    if (!row["bind_ip"].is_null()) {
        dev->bind_ip = row["bind_ip"].c_str();
    }
    return dev;
};

auto handle_query_device_func = [](const mysqlpp::StoreQueryResult& res){
    if (res && res.num_rows() == 1) {
        return handle_device_func(res[0]);
    }
    return std::make_shared<device>();
};

auto handle_query_all_device_func = [](const mysqlpp::StoreQueryResult& res){
    std::list<device_pointer> device_list;
    if (res) {
        for (auto& row : res) {
            device_list.push_back(handle_device_func(row));
        }
    }
    return device_list;
};

// + add by zhengweixue 20160118
auto handle_query_vip_vehicle_func = [](const mysqlpp::Row& row){

	VIPVehicle vip;
	vip.id = row["vip_id"].c_str();
	vip.plate_number = row["carno"].c_str();
	vip.vehicle_type = row["carno_type"].c_str();
	vip.type = int(row["vip_type"]);
	if (!row["expiry_date"].is_null()) {
		mysqlpp::DateTime dt(row["expiry_date"].c_str());
		vip.expiry_date = time_t(dt);
	}

	vip.remain_money = float(row["remain_money"]);
	vip.delete_status = row["delete_status"].c_str();
	vip.freeze_status = row["freeze_status"].c_str();
	if (!row["freeze_until"].is_null()) {
		mysqlpp::DateTime dt(row["freeze_until"].c_str());
		vip.freeze_date = time_t(dt);
	}

	if (!row["vip_discount"].is_null()) {
		vip.discount = double(row["vip_discount"]);
	}

	if (!row["valid_begintime"].is_null()) {
		mysqlpp::DateTime dt(row["valid_begintime"].c_str());
		vip.valid_begintime = time_t(dt);
	}
	return vip;
};
// + add end


std::list<device_pointer> QueryDeviceList() {
    return UniveralOperateDB("select * from equip_info",
                             handle_query_all_device_func);
}

std::list<device_pointer> QueryEntranceDeviceList() {
    return UniveralOperateDB("select * from equip_info where videofunc=9",
                             handle_query_all_device_func);
}

std::list<device_pointer> QueryDeviceListByBindIP(const std::string& bind_ip) {
    auto sql = str(format("select * from equip_info where bind_ip='%1%'")
                   % bind_ip);
    return UniveralOperateDB(sql, handle_query_all_device_func);
}

device_pointer QueryDeviceByID(const std::string& id) {
    auto sql = str(format("select * from equip_info where devicecode='%1%'") % id);
    return UniveralOperateDB(sql, handle_query_device_func);
}

device_pointer QueryDeviceByIP(const std::string& ip) {
    auto sql = str(format("select * from equip_info where ip='%1%'") % ip);
    return UniveralOperateDB(sql, handle_query_device_func);
}

device_pointer QueryDeviceByPassID(const std::string& pass_id) {
    auto sql = str(format("select e.* from equip_info e join park_detect_info d "
                          "where d.devicecode=e.devicecode and d.nid='%1%'")
                   % pass_id);
    return UniveralOperateDB(sql, handle_query_device_func);
}

std::string QueryDeviceIDByIP(const std::string& ip) {
    auto sql = str(format("select devicecode from equip_info where ip='%1%'") % ip);
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
            std::string device_id;
            if (res && res.num_rows() > 0) {
                device_id = res[0]["devicecode"].c_str();
            }
            return device_id;
        });
}

std::string QueryDeviceIPByID(const std::string& id) {
    auto sql = str(format("select ip from equip_info where devicecode='%1%'") % id);
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
            std::string device_ip;
            if (res && res.num_rows() > 0) {
                device_ip = res[0]["ip"].c_str();
            }
            return device_ip;
        });
}

using boost::locale::conv::utf_to_utf;

std::wstring utf8_to_wstring(const std::string& str)
{
    return utf_to_utf<wchar_t>(str.c_str(), str.c_str() + str.size());
}

std::string wstring_to_utf8(const std::wstring& str)
{
    return utf_to_utf<char>(str.c_str(), str.c_str() + str.size());
}

//	模糊匹配算法
int matchscore(const string& s8, const string& t8)
{
    std::wstring s = utf8_to_wstring(s8);
    std::wstring t = utf8_to_wstring(t8);

    std::vector<std::vector<int>> d;

    int m = s.length();
    int n = t.length();

    d.resize(m+1);
    for(int i=0; i<=m; i++) {
        d[i].resize(n+1, 0);
    }

    for(int i=1; i<=m; i++) {
        d[i][0] = i;
    }
    for(int j=1; j<=n; j++) {
        d[0][j] = j;
    }
    for(int j=1; j<=n; j++) {
        for(int i=1; i<=m; i++) {
            if (std::towupper(s[i-1]) == std::towupper(t[j-1])) {
                d[i][j]=d[i-1][j-1];
            }
            else {
                d[i][j]=std::min(d[i-1][j]+1, std::min(d[i][j-1]+1,d[i-1][j-1]+1));
            }
        }
    }
    return d[m][n];
}


Json::Value QueryParkingVehicle(const Json::Value& json)
{
    auto start_time = json["StartTime"];
    auto end_time = json["EndTime"];
    auto vehicle_color = json["VehicleColor"];
    auto plate_number = json["PlateNumber"];
    // auto park_type = json["ParkingType"];

    string sql = str(format("select d.nid, d.licensetype, d.carno, d.carnosecond, d.devicecode"
                            ", d.direction, d.cpic1path, d.cpic2path, d.platecolor, d.plate_position"
                            ", d.carcolor, d.carbrand, d.carversion, d.collectiondate"
                            " from park_detect_info as d"
                            " right join park_inner_cars as i on d.nid = i.nid where true"));
    if (!start_time.isNull()) {
        sql += str(format(" and d.collectiondate >= '%s'") % start_time.asString());
    }
    if (!end_time.isNull()) {
        sql += str(format(" and d.collectiondate <= '%s'") % end_time.asString());
    }
    if (!vehicle_color.isNull()) {
        sql += str(format(" and d.carcolor = '%s'") % vehicle_color.asString());
    }

    log()->info("query parking vehicle sql: {}", sql);

    return UniveralOperateDB(sql, [plate_number](const mysqlpp::StoreQueryResult& res){
            Json::Value root;
            if (res) {
                std::list<std::pair<int, Json::Value>> records;
                for (auto& row : res) {
                    Json::Value record;

                    record["PassID"] = row["nid"].c_str();
					record["PlateType"] = std::stoi(row["licensetype"].c_str());
                    record["PlateNumber1"] = row["carno"].c_str();
                    record["PlateNumber2"] = row["carnosecond"].c_str();
                    record["DeviceID"] = row["devicecode"].c_str();
                    record["PassDirection"] = std::stoi(row["direction"].c_str());
                    record["PictureURL1"] = row["cpic1path"].c_str();
                    record["PictureURL2"] = row["cpic2path"].c_str();
                    record["PlateColor"] = std::stoi(row["platecolor"].c_str());

                    if (!row["plate_position"].is_null()) {
                        string val = row["plate_position"].c_str();
                        std::vector<std::string> positions;
                        split(positions, val, is_any_of(","));
                        for (auto& pos : positions) {
                            record["PlatePosition"].append(std::stoi(pos));
                        }
                    }

                    if (!row["carcolor"].is_null()) {
                        record["VehicleColor"] = row["carcolor"].c_str();
                    }

                    if (!row["carbrand"].is_null()) {
                        record["VehicleType"] = std::stoi(row["carbrand"].c_str());
                    }

                    if (!row["carversion"].is_null()) {
                        record["VehicleSubType"] = std::stoi(row["carversion"].c_str());
                    }

                    record["PassTime"] = row["collectiondate"].c_str();

                    if (!plate_number.isNull()) {
                        records.push_back(
                            std::make_pair(matchscore(plate_number.asString(), row["carno"].c_str()),
                                           record));
                    } else {
                        records.push_back(std::make_pair(0, record));
                    }
                }
                auto end = records.end();

                records.sort(
                    [](const std::pair<int, Json::Value>& a, const std::pair<int, Json::Value>& b) {
                        mysqlpp::DateTime dt1(a.second["PassTime"].asString());
                        mysqlpp::DateTime dt2(b.second["PassTime"].asString());
                        return dt1 > dt2;
                    });

                if (!plate_number.isNull()) {
                    records.sort(
                        [](const std::pair<int, Json::Value>& a, const std::pair<int, Json::Value>& b) {
                            return a.first < b.first;
                        });
                    end = std::remove_if(records.begin(), records.end(),
                                         [](const std::pair<int, Json::Value>& v) {
                                             return v.first > 2;
                                         });
                }
                for(auto iter=records.begin(); iter!=end; ++iter) {
                    root.append(iter->second);
                }
            }

            return root;
        });
}


auto handle_query_enter_device_func = [](const mysqlpp::Row& row){
    PassVehicle vehicle;
    vehicle.pass_id_ = row["nid"].c_str();
    vehicle.plate_type_ = int(row["licensetype"]);
    vehicle.plate_number1_ = row["carno"].c_str();
    vehicle.plate_number2_ = row["carnosecond"].c_str();
    vehicle.plate_color_ = int(row["platecolor"]);
    vehicle.device_id_ = row["devicecode"].c_str();
    vehicle.pass_direction_ = int(row["direction"]);
    vehicle.picture_url1_ = row["cpic1path"].c_str();
    vehicle.picture_url2_ = row["cpic2path"].c_str();
    vehicle.vehicle_color_ = row["carcolor"].c_str();
    vehicle.plate_position_ = row["plate_position"].c_str();
    auto carbrand = row["carbrand"];
    if (!carbrand.empty()) {
        vehicle.vehicle_type_ = int(carbrand);
    }
    auto carversion = row["carversion"];
    if (!carversion.empty()) {
        vehicle.vehicle_subtype_ = int(carversion);
    }
    mysqlpp::DateTime dt(row["collectiondate"].c_str());
    vehicle.pass_time_= time_t(dt);
    return vehicle;
};

std::vector<PassVehicle> QueryEnterVehicle(const std::string& exit_vehicle_plate) {
    auto sql1 = str(format("select * from park_detect_info t join "
                           "park_inner_cars tt where t.nid = tt.nid and t.carno = '%1%'")
                    % exit_vehicle_plate);
    return UniveralOperateDB(sql1, [](const mysqlpp::StoreQueryResult& res){
            std::vector<PassVehicle> vehicles;
            if (res) {
                for (auto& row : res) {
                    if (int(row["direction"]) == 9) {
                        vehicles.push_back(handle_query_enter_device_func(row));
                    }
                }
            }
            return vehicles;
        });
}

// + add by zhengweixue 20160118
std::vector<VIPVehicle> QueryVIPVehicleRecord_IgnoreProvince(const std::string& plate_number)
{
	std::string plate_number_matched(plate_number);
	plate_number_matched = plate_number_matched.substr(3, plate_number_matched.length() - 3);
	plate_number_matched = "_" + plate_number_matched;

	log()->debug("plate_number_matched: {}", plate_number_matched);
	auto sql = str(format("select * from park_vip_info "
		" where carno like '%1%'") % plate_number_matched);

	return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
		std::vector<VIPVehicle> vehicles;
		if (res) 
		{
			for (auto& row : res) 
			{
				vehicles.push_back(handle_query_vip_vehicle_func(row));
			}
		}
		return vehicles;
	});
}

std::vector<VIPVehicle> QueryVIPVehicleRecord_FuzzyMatch(const std::string& plate_number)
{
	std::string plate_number_matched(plate_number);
	std::string plate_number_matched1(plate_number_matched.substr(3, plate_number_matched.length() - 3));
	plate_number_matched1 = "_" + plate_number_matched1;
	plate_number_matched = plate_number;
	std::string plate_number_matched2(plate_number_matched.replace(3, 1, "_"));
	plate_number_matched = plate_number;
	std::string plate_number_matched3(plate_number_matched.replace(4, 1, "_"));
	plate_number_matched = plate_number;
	std::string plate_number_matched4(plate_number_matched.replace(5, 1, "_"));
	plate_number_matched = plate_number;
	std::string plate_number_matched5(plate_number_matched.replace(6, 1, "_"));
	plate_number_matched = plate_number;
	std::string plate_number_matched6(plate_number_matched.replace(7, 1, "_"));
	plate_number_matched = plate_number;
	std::string plate_number_matched7(plate_number_matched.replace(8, 1, "_"));

	auto sql = str(format("select * from park_vip_info "
		" where carno like '%1%'"
		"or carno like '%2%'"
		"or carno like '%3%'"
		"or carno like '%4%'"
		"or carno like '%5%'"
		"or carno like '%6%'"
		"or carno like '%7%'") % plate_number_matched1 %plate_number_matched2
		%plate_number_matched3 %plate_number_matched4 %plate_number_matched5
		%plate_number_matched6 %plate_number_matched7);

	return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
		std::vector<VIPVehicle> vehicles;
		if (res)
		{
			for (auto& row : res)
			{
				vehicles.push_back(handle_query_vip_vehicle_func(row));
			}
		}
		return vehicles;
	});
}
// + add end

PassVehicle QueryPassVehicleByID(const std::string& pass_id) {
    auto sql1 = str(format("select * from park_detect_info where nid='%1%'")
                    % pass_id);

    return UniveralOperateDB(sql1, [](const mysqlpp::StoreQueryResult& res){
            PassVehicle vehicle;
            if (res && res.num_rows() == 1) {
                vehicle = handle_query_enter_device_func(res[0]);
            }
            return vehicle;
        });
}
// + add by zhengweixue  20160118  白名单车辆的模糊匹配规则

bool QueryVIPVehicle_IgnoreProvinceMatch(const std::string& plate_number, std::vector<VIPVehicle>& vip_vec) {

	vip_vec = QueryVIPVehicleRecord_IgnoreProvince(plate_number);
	if (vip_vec.empty())
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool QueryVIPVehicle_FuzzyMatch(const std::string& plate_number, std::vector<VIPVehicle>& vip_vec)
{
	vip_vec = QueryVIPVehicleRecord_FuzzyMatch(plate_number);
	if (vip_vec.empty())
	{
		return false;
	}
	else
	{
		return true;
	}
}
// + add end


bool QueryVIPVehicle(const std::string& plate_number, VIPVehicle& vip) {
    auto sql = str(format("select * from park_vip_info "
                          " where carno='%1%'") % plate_number);
    return UniveralOperateDB(sql, [&vip](const mysqlpp::StoreQueryResult& res){
            if (res && res.num_rows() > 0) {
                vip.id = res[0]["vip_id"].c_str();
                vip.plate_number = res[0]["carno"].c_str();
                vip.vehicle_type = res[0]["carno_type"].c_str();
                vip.type = int(res[0]["vip_type"]);
                if (!res[0]["expiry_date"].is_null()) {
                    mysqlpp::DateTime dt(res[0]["expiry_date"].c_str());
                    vip.expiry_date= time_t(dt);
                }

                vip.remain_money = float(res[0]["remain_money"]);
                vip.delete_status = res[0]["delete_status"].c_str();
                vip.freeze_status = res[0]["freeze_status"].c_str();
                if (!res[0]["freeze_until"].is_null()) {
                    mysqlpp::DateTime dt(res[0]["freeze_until"].c_str());
                    vip.freeze_date= time_t(dt);
                }

                if (!res[0]["vip_discount"].is_null()) {
                    vip.discount = double(res[0]["vip_discount"]);
                }

                if (!res[0]["valid_begintime"].is_null()) {
                    mysqlpp::DateTime dt(res[0]["valid_begintime"].c_str());
                    vip.valid_begintime= time_t(dt);
                }
                return true;
            }
            return false;
        });
}

std::string QueryPlateNumber(const std::string& pass_id) {
    auto sql = str(format("select carno from park_detect_info where nid='%1%'") % pass_id);
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
            std::string plate_number;
            if (res && res.num_rows() == 1) {
                plate_number = res[0]["carno"].c_str();
            }
            return plate_number;
        });
}

int QueryParkPosition(const std::string& device_id) {
    auto sql = str(format("select g.position from equip_info t join "
                          "gate_info g where t.devicecode = '%1%' "
                          "and t.pointcode = g.pointcode") % device_id);
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
            int pos = 0;
            if (res && res.num_rows() == 1) {
                auto position = res[0]["position"];
                if (!position.is_null()) {
                    std::string strposition = position.c_str();
                    if (!strposition.empty()) {
                        pos = int(position);
                    }
                }
            }
            return pos;
        });
}

auto handle_query_rule_func = [](const mysqlpp::StoreQueryResult& res,
                                 ChargeRule& rule){
    if (res && res.num_rows() == 1) {
        rule.id = res[0]["nid"].c_str();
        rule.name = res[0]["name"].c_str();
        rule.priority = int(res[0]["rule_priority"]);
        rule.daylight_begin = duration_from_string(res[0]["day_begin"].c_str());
        rule.night_begin = duration_from_string(res[0]["night_begin"].c_str());
        if (!res[0]["free_time_length"].is_null()) {
            rule.free_time_length = int(res[0]["free_time_length"]);
        }
        else {
            rule.free_time_length = 0;
        }
        rule.day_fee_unit = int(res[0]["dayfeespan"]);
        rule.day_fee_unit = rule.day_fee_unit == 0 ?
                            1 : rule.day_fee_unit;
        rule.day_fee_rate = double(res[0]["dayfeerate"]);

        rule.day_fee_unit_big = int(res[0]["dayfeespanbig"]);
        rule.day_fee_unit_big = rule.day_fee_unit_big == 0 ?
                                1 : rule.day_fee_unit_big;
        rule.day_fee_rate_big = double(res[0]["dayfeeratebig"]);

        rule.night_fee_unit = int(res[0]["nightfeespan"]);
        rule.night_fee_unit = rule.night_fee_unit == 0 ?
                              1 : rule.night_fee_unit;
        rule.night_fee_rate = double(res[0]["nightfeerate"]);

        rule.night_fee_unit_big = int(res[0]["nightfeespanbig"]);
        rule.night_fee_unit_big = rule.night_fee_unit_big == 0 ?
                                  1 : rule.night_fee_unit_big;
        rule.night_fee_rate_big = double(res[0]["nightfeeratebig"]);

        rule.max_fee = double(res[0]["maxfee"]);
        rule.max_fee_big = double(res[0]["maxfeebig"]);
        rule.park_position = int(res[0]["park_position"]);
        if (!res[0]["free_time_sections"].is_null()) {
            std::string strsection = res[0]["free_time_sections"].c_str();
            if (!strsection.empty()) {
                std::vector<std::string> sections;
                split(sections, strsection, is_any_of(";"));
                for (auto& s : sections) {
                    std::vector<std::string> durations;
                    split(durations, s, is_any_of("-"));
                    if (durations.size() >= 2) {
                        TimePeriod td{duration_from_string(durations[0]),
                                    duration_from_string(durations[1])};
                        rule.free_time_sections.push_back(td);
                    }
                    else {
                        log()->warn("Time duration format in charge rule table is invalid");
                    }
                }
            }
        }
        if (!res[0]["create_user"].is_null()) {
            rule.create_user = res[0]["create_user"].c_str();
        }
        if (!res[0]["create_time"].is_null()) {
            mysqlpp::DateTime dt(res[0]["create_time"].c_str());
            rule.create_time= time_t(dt);
        }
        if (!res[0]["invalid_flag"].is_null()) {
            rule.invalid_flag = int(res[0]["invalid_flag"]);
        }
        if (!res[0]["invalid_time"].is_null()) {
            mysqlpp::DateTime dt(res[0]["invalid_time"].c_str());
            rule.invalid_time= time_t(dt);
        }
        return true;
    }
    return false;
};

bool QueryChargeRule(const std::string& date, int park_position, ChargeRule& rule) {
    auto sql = str(format("select * from park_rule_config a, "
                          "(select min(rule_priority) min_priority from park_rule_config) b "
                          "where (a.rule_priority = b.min_priority or a.week_day = dayofweek('%1%') "
                          "or dayofweek('%1%') in (1,7) "
                          "or '%1%' between festival_begin_time and festival_end_time) and "
                          "park_position = %2% and (invalid_flag is null or invalid_flag = 1) "
                          "order by rule_priority desc limit 1")
                   % date % park_position);
    return UniveralOperateDB(sql, [&rule](const mysqlpp::StoreQueryResult& res){
            return handle_query_rule_func(res, rule);
        });
}

bool QueryMinPriorityRule(int park_position, ChargeRule& rule) {
    auto sql = str(format("select * from park_rule_config t, "
                          "(select min(rule_priority) rule_priority "
                          "from park_rule_config) tt where "
                          "t.rule_priority  = tt.rule_priority and "
                          "t.park_position = %1% and "
                          "(t.invalid_flag is null or t.invalid_flag = 1)")
                   % park_position);
    return UniveralOperateDB(sql, [&rule](const mysqlpp::StoreQueryResult& res){
            return handle_query_rule_func(res, rule);
        });
}

ParkingNumber QueryParkingNumber() {
	
    ParkingNumber number;
    auto sql1 = "select berth_count from park_info";
    UniveralOperateDB(sql1, [&number](const mysqlpp::StoreQueryResult& res){
            if (res && res.num_rows() == 1) {
                number.total_parking = int(res[0]["berth_count"]);
            }
        });
	auto sql5 = "select rectify_count from park_info";
	UniveralOperateDB(sql5, [&number](const mysqlpp::StoreQueryResult& res){
		if (res && res.num_rows() == 1) {
			number.rectify_count = int(res[0]["rectify_count"]);
		}
	});
	auto sql3 = "select count(*) from park_vip_info";
	UniveralOperateDB(sql3, [&number](const mysqlpp::StoreQueryResult& res){
		if (res && res.num_rows() == 1) {
			number.real_temporary_parking = number.total_parking - int(res[0]["count(*)"]) + number.rectify_count;
		}
	});
	auto sql4 = str(format("select count(*) from park_inner_cars where parking_type=  %1% ")%"9");
	UniveralOperateDB(sql4, [&number](const mysqlpp::StoreQueryResult& res){
		if (res && res.num_rows() == 1) {
			number.real_temporary_parking = number.real_temporary_parking - int(res[0]["count(*)"]) + number.rectify_count;
		}
	});
    auto sql2 = "select count(*) from park_inner_cars";
    return UniveralOperateDB(sql2, [&number](const mysqlpp::StoreQueryResult& res){
            if (res && res.num_rows() == 1) {
				auto judge_ok = QueryJudgeRemainParkingNumber();
				if (judge_ok==2)
				{
					number.real_remain_parking = number.total_parking - int(res[0]["count(*)"]) + number.rectify_count;
					if (number.real_temporary_parking < 0) {
						number.remain_parking = 0;
					}
				}
				else
				{
					number.remain_parking = number.total_parking - int(res[0]["count(*)"]) + number.rectify_count;
					number.real_remain_parking = number.remain_parking;
					if (number.remain_parking < 0) {
						number.remain_parking = 0;
					}
				}  
            }
            return number;
        });
}

std::vector<TaxiInfo> QueryTaxiInfo() {
    auto sql = "select * from taxi_list";
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
            std::vector<TaxiInfo> taxis;
            if (res) {
                for (auto& row : res) {
                    TaxiInfo taxi;
                    taxi.plate_regex_pattern = row["carno_Reg"].c_str();
                    taxis.push_back(taxi);
                }
            }
            return taxis;
        });
}

void InsertShiftLog(const Json::Value& shift) {
    auto sql = str(format("insert into park_duty_info "
                          "(nid, duty_give_userid, duty_receive_userid, duty_pass_time) "
                          "values "
                          "('%1%', '%2%', '%3%', NOW())")
                   % GenerateUUID()
                   % shift["CurrentUser"].asString()
                   % shift["NextUser"].asString());
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
        });
}

void InsertGatewayOperationLog(const ManualGatewayOperationInfo& info) {
    auto sql = str(format("insert into park_gate_operate_info "
                          "(nid, ref_nid, devicecode, actiontype, operator, actiondate) "
                          "values "
                          "('%1%', '%2%', '%3%', '%4%', '%5%', NOW())")
                   % GenerateUUID()
                   % info.pass_id
                   % info.device_id
                   % info.action_type
                   % info.user_id);
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
        });
}
//	add by shenzixiao 2016/4/6
void InsertWaitVehicleInfo(const WaitVehicleInfo& info)
{
	auto sql = str(format("insert into park_wait_info "
		"(nid, device_ip, wait_vehicle_plate_number,wait_vehicle_plate_type, actiondate) "
		"values "
		"('%1%', '%2%', '%3%', '%4%',NOW())")
		% GenerateUUID()
		% info.device_ip
		% info.wait_vehicle_plate_number
		% info.wait_vehicle_plate_type
		);
	return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
	});
}
std::vector<WaitVehicleInfo> QueryWaitVehicleInfo()
{
	auto sql = str(format("select * from park_wait_info"));
	return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
		std::vector<WaitVehicleInfo> WaitVehicles;
		if (res) {
			for (auto& row : res) {
				WaitVehicleInfo WaitVehicle;
				WaitVehicle.device_ip = row["device_ip"].c_str();
				WaitVehicle.wait_vehicle_plate_number = row["wait_vehicle_plate_number"].c_str();
				WaitVehicle.wait_vehicle_plate_type = std::stoi(row["wait_vehicle_plate_type"].c_str());
				WaitVehicles.push_back(WaitVehicle);
			}
		}
		return WaitVehicles;
	});
}
bool QueryWaitVehicleInfoRes(std::vector<WaitVehicleInfo>& wait_vec)
{
	wait_vec = QueryWaitVehicleInfo();
	if (wait_vec.empty())
	{
		return false;
	}
	else
	{
		return true;
	}
}
bool QueryWaitVehicleByDevice_ip(std::string device_ip, WaitVehicleInfo& info)
{
	auto sql = str(format("select * from park_wait_info "
		" where device_ip='%1%'") % device_ip);
	return UniveralOperateDB(sql, [&info](const mysqlpp::StoreQueryResult& res){
		if (res && res.num_rows() > 0) {
			info.device_ip = res[0]["device_ip"].c_str();
			info.wait_vehicle_plate_number = res[0]["wait_vehicle_plate_number"].c_str();
			info.wait_vehicle_plate_type = std::stoi(res[0]["wait_vehicle_plate_type"].c_str());
			return true;
		}
		return false;
	});
}
void DeleteWaitVehicleByDevice_ip(std::string device_ip)
{
	auto sql = str(format("delete from park_wait_info where device_ip = '%1%'")
		% device_ip);
	return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
	});
}
bool QueryWaitVehicleByPlate_num(std::string plate_number, WaitVehicleInfo& info)
{
	auto sql = str(format("select * from park_wait_info "
		" where wait_vehicle_plate_number='%1%'") % plate_number);
	return UniveralOperateDB(sql, [&info](const mysqlpp::StoreQueryResult& res){
		if (res && res.num_rows() > 0) {
			info.device_ip = res[0]["device_ip"].c_str();
			info.wait_vehicle_plate_number = res[0]["wait_vehicle_plate_number"].c_str();
			info.wait_vehicle_plate_type = std::stoi(res[0]["wait_vehicle_plate_type"].c_str());
			return true;
		}
		return false;
	});
}

void DeleteWaitVehicleByPlate_num(std::string plate_number)
{
	auto sql = str(format("delete from park_wait_info where wait_vehicle_plate_number = '%1%'")
		% plate_number);
	return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
	});
}
void DeleteWaitVehicleInfo()
{
	auto sql = str(format("delete park_wait_info"));
	return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
	});
}
//	add end 
std::string InsertPassVehicleInfo(const PassVehicle& info) {
    auto nid = GenerateUUID();
    mysqlpp::DateTime t(info.pass_time_);
	//	匹配白名单
	auto parking_type = util::GetParkingType(info);

    auto sql = str(format("insert into park_detect_info "
                          "(nid, licensetype, carno, carnosecond, "
                          "carno_original, direction, devicecode, "
                          "cpic1path, cpic2path, carcolor, platecolor, carbrand, "
                          "carversion, collectiondate, savedate,parking_type, pointcode, plate_position) "
                          "values "
                          "('%1%', '%2%', '%3%', '%4%', "
                          "'%5%', '%6%', '%7%', "
                          "'%8%', '%9%', '%10%', '%11%', '%12%', "
                          "'%13%', '%14%', NOW(), '%15%', '%16%', '%17%')")
                   % nid
                   % info.plate_type_
                   % info.plate_number1_
                   % info.plate_number2_
                   % info.plate_number1_
                   % info.pass_direction_
                   % info.device_id_
                   % info.picture_url1_
                   % info.picture_url2_
                   % info.vehicle_color_
                   % info.plate_color_
                   % (info.vehicle_type_ ? std::to_string(*info.vehicle_type_) : "")
                   % (info.vehicle_subtype_ ? std::to_string(*info.vehicle_subtype_) : "")
                   % t.str()
				   % parking_type
                   % "123456789012"
                   % info.plate_position_);
    UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
        });
    return nid;
}

std::string GenerateChargeRecordID() {
    return GenerateUUID();
}

void InsertChargeRecord(const std::string& nid,
                        const ChargeRecord& record) {
    auto sql = str(format("insert into park_charge_info "
                          "(nid, enter_nid, exit_nid, charge, realcharge, "
                          "pay_type, operator, parking_type, exit_type, "
                          "exit_comment, remain_money, charge_detail, discount, "
                          "record_time) "
                          "values "
                          "('%1%', '%2%', '%3%', %4%, %5%, "
                          "'%6%', '%7%', '%8%', '%9%', "
                          "'%10%', %11%, '%12%', %13%, NOW())")
                   % nid
                   % record.enter_id
                   % record.exit_id
                   % record.charge
                   % record.actual_charge
                   % record.pay_type
                   % record.user
                   % record.parking_type
                   % record.exit_type
                   % record.exit_comment
                   % (record.remain_money ? *record.remain_money : 0)
                   % record.charge_detail
                   % record.discount);
    UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
        });
}

void UpdateVoucherPictureURL(const std::string& nid,
                              const std::string& picture_url) {
    auto sql = str(format("update park_charge_info set proof_url='%1%' where nid='%2%'")
                   % picture_url % nid);
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
        });
}

void ModifyPlateNumber(const string& pass_id,
                       const string& plate_number,
                       const string& user) {
    auto sql = str(format("update park_detect_info set "
                          "carno='%1%', editor='%2%', editdate=NOW() where nid='%3%'")
                   % plate_number % user % pass_id);
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
        });
}

void DeleteTemporaryPassRecord(const string& pass_id) {
    auto sql = str(format("delete from park_inner_cars where nid='%1%'")
                          % pass_id);
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
        });
}

void UpdateStoredValue(const string& vip_id, float remain_money) {
    auto sql = str(format("update park_vip_info set remain_money=%1% where vip_id='%2%'")
                   % remain_money % vip_id);
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
        });
}

// void UpdateRemainParkingNumber(int remain_parkingnumber) {
//     auto sql = str(format("update park_info set remain_berth_count='%1%'")
//                    % remain_parkingnumber);
//     return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
//         });
// }

// void DecreaseRemainParkingNumber() {
//     auto sql = str(format("update park_info set remain_berth_count = if(remain_berth_count<1, 0, remain_berth_count-1)"));
//     return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
//         });
// }

// void IncreaseRemainParkingNumber() {
//     auto total_parking_sql = "select berth_count from park_info";
//     auto total_parking =  UniveralOperateDB(total_parking_sql,
//                              [](const mysqlpp::StoreQueryResult& res){
//             int number = 0;
//             if (res && res.num_rows() == 1) {
//                 number = int(res[0]["berth_count"]);
//             }
//             return number;
//         });
//     auto sql = str(format("update park_info set remain_berth_count = if(remain_berth_count>'%1%', '%2%', remain_berth_count+1)")
//                    % (total_parking - 1) % total_parking);
//     return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
//         });
// }

auto get_vehicle_picture = [](const mysqlpp::StoreQueryResult& res) {
    std::vector<PassVehiclePicture> vehicles;
    if (res) {
        for (auto& row : res) {
            PassVehiclePicture vehicle;
            vehicle.nid = row["nid"].c_str();
            vehicle.picture_url1_ = row["cpic1path"].c_str();
            vehicle.picture_url2_ = row["cpic2path"].c_str();
            vehicles.push_back(vehicle);
        }
    }
    return vehicles;
};

//	Modify by shenzixiao  2016/3/8
/*
	error: Operand should contain 1 column(s)

	solution: in (select * from ...)  ==>in (select nid ...)
*/
std::vector<PassVehiclePicture> QueryPassVehiclePicNeedClear(int expiry) {
    auto sql = str(format("select nid, cpic1path, cpic2path from park_detect_info"
                           " where nid not in (select nid from park_inner_cars)"
                           " and collectiondate < SUBDATE(NOW(), INTERVAL %1% DAY)"
                           " and (cpic1path <> '' or cpic2path <> '')")
                    % expiry);
	//auto sql = str(format("select nid, cpic1path, cpic2path from park_detect_info"
	//	" where nid not in (select * from park_inner_cars)"
	//	" and collectiondate < SUBDATE(NOW(), INTERVAL %1% DAY)"
	//	" and (cpic1path <> '' or cpic2path <> '')")
	//	% expiry);
    return UniveralOperateDB(sql, get_vehicle_picture);
}

std::vector<PassVehiclePicture> QueryEarliestPassVehiclePicNeedClear() {
    auto sql = "select nid, cpic1path, cpic2path from park_detect_info t, "
               "(select DATE_FORMAT(min(t.collectiondate), '%Y-%m-%d') min_time "
               "from park_detect_info t "
               "where "
               "nid not in (select nid from park_inner_cars) and "
               "(t.cpic1path <> '' or t.cpic2path <> '')) mintime "
               "where "
               "DATE_FORMAT(t.collectiondate, '%Y-%m-%d') = mintime.min_time "
               "and nid not in (select * from park_inner_cars)"
               "and (t.cpic1path <> '' or t.cpic2path <> '')";
    return UniveralOperateDB(sql, get_vehicle_picture);
}

std::vector<PassVehiclePicture> QueryPassVehiclePicByCounts(unsigned int counts) {
    auto sql = str(format("select nid, cpic1path, cpic2path from park_detect_info "
                          "where nid not in (select nid from park_inner_cars) "
                          "and (cpic1path <> '' or cpic2path <> '') "
                          "order by collectiondate limit %1%") % counts);
    return UniveralOperateDB(sql, get_vehicle_picture);
}
//	Modify end

void EmptyPassVehiclePicPath(const string & nid, int pic_id) {
    string sql;
    if (pic_id == 1) {
        sql = str(format("update park_detect_info set cpic1path = '' where nid = '%s'") % nid);
    }
    else if (pic_id == 2) {
        sql = str(format("update park_detect_info set cpic2path = '' where nid = '%s'") % nid);
    }
    else {
        sql = str(format("update park_detect_info set cpic1path = '', cpic2path = '' where nid = '%s'") % nid);
    }
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
        });
}

auto get_voucher_picture = [](const mysqlpp::StoreQueryResult& res) {
    std::vector<ChargeVoucherPicture> vouchers;
    if (res) {
        for (auto& row : res) {
            ChargeVoucherPicture voucher;
            voucher.nid = row["nid"].c_str();
            voucher.picture_url = row["proof_url"].c_str();
            vouchers.push_back(voucher);
        }
    }
    return vouchers;
};

std::vector<ChargeVoucherPicture> QueryVoucherPicNeedClear(int expiry) {
    auto sql = str(format("select nid, proof_url from park_charge_info "
                           "where record_time < SUBDATE(NOW(), INTERVAL %1% DAY)"
                    " and proof_url <> ''")
                    % expiry);

    return UniveralOperateDB(sql, get_voucher_picture);
}

std::vector<ChargeVoucherPicture> QueryEarliestVoucherPicNeedClear() {
    auto sql = "select nid, proof_url from park_charge_info t, "
               "(select DATE_FORMAT(min(t.record_time), '%Y-%m-%d') min_time "
               "from park_charge_info t "
               "where "
               "proof_url <> '') mintime "
               "where "
               "DATE_FORMAT(t.record_time, '%Y-%m-%d') = mintime.min_time "
               "and proof_url <> ''";
    return UniveralOperateDB(sql, get_voucher_picture);
}

std::vector<ChargeVoucherPicture> QueryVoucherPicByCounts(unsigned int counts) {
    auto sql = str(format("select nid, proof_url from park_charge_info "
                          "where proof_url <> '' "
                          "order by record_time limit %1%") % counts);
    return UniveralOperateDB(sql, get_voucher_picture);
}


void EmptyVoucherPicPath(const string & nid) {
    auto sql = str(format("update park_charge_info set proof_url = '' where nid = '%1%'")
                   % nid);
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
        });
}

unsigned int GetPassVehiclePictureCounts() {
    return UniveralOperateDB("select count(*) as ct from park_detect_info "
                             "where cpic1path <>'' or cpic2path <>''",
                             [](const mysqlpp::StoreQueryResult& res)->unsigned int{
                                 if (res && res.num_rows() == 1) {
                                     return static_cast<unsigned int>(res[0]["ct"]);
                                 }
                                 return 0;
                             });
}

unsigned int GetVoucherPictureCounts() {
    return UniveralOperateDB("select count(proof_url) as ct from park_charge_info where proof_url <>''",
                             [](const mysqlpp::StoreQueryResult& res)->unsigned int{
                                 if (res && res.num_rows() == 1) {
                                     return static_cast<unsigned int>(res[0]["ct"]);
                                 }
                                 return 0;
                             });
}

SimpleChargeRecord QueryLastestChargeByPlateNumber(const string& plate_number) {
    auto sql = str(format("select charge.nid, charge.enter_nid from park_detect_info detect, "
                          "(select nid, enter_nid, exit_nid from park_charge_info order by record_time desc) charge "
                          "where charge.exit_nid = detect.nid and detect.carno = '%1%'")
                   % plate_number);
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
            SimpleChargeRecord record;
            if (res && res.num_rows() >= 1) {
                record.charge_id = res[0]["nid"].c_str();
                record.enter_id = res[0]["enter_nid"].c_str();
				record.record_time = res[0]["record_time"].c_str();//	add by shenzixiao 2016/3/23
            }
            return record;
        });
}

void DeleteChargeRecord(const std::string& charge_id) {
    auto sql = str(format("delete from park_charge_info where nid = '%1%'")
                   % charge_id);
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
        });
}

void InsertInnerVehicle(const string& nid) {
    auto sql = str(format("insert into park_inner_cars set nid = '%1%'")
                   % nid);
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
        });
}

void DeleteInnerVehicleByPlateNumber(const std::string& plate_number) {
    auto sql = str(format("delete incar from park_inner_cars incar, "
                          "(select nid, carno from park_detect_info order by savedate desc) "
                          "detect where detect.carno='%1%' and detect.nid=incar.nid")
                   % plate_number);
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
        });
}

void DeleteInnerVehicle(const std::string& nid) {
    auto sql = str(format("delete from park_inner_cars where nid = '%1%'")
                   % nid);
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
        });
}

void RecordUnusualRecord(const string& op, const string& op_time, const string& leave_nid,
                         const string& match_nids, double cacl_fee)
{
    auto sql = str(format("insert into park_unusual_record set "
                          "nid = '%s', "
                          "operator = '%s', "
                          "op_time = '%s', "
                          "leave_nid = '%s', "
                          "match_nids = '%s', "
                          "cacl_fee = '%f'")
                   % GenerateUUID()
                   % op
                   % op_time
                   % leave_nid
                   % match_nids
                   % cacl_fee);
    return UniveralOperateDB(sql, [](const mysqlpp::StoreQueryResult& res){
        });
}

}
