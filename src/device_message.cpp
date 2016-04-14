#include "device_message.h"
#include <boost/format.hpp>
#include <boost/regex.hpp>
#include "log.h"
#include <boost/crc.hpp>

namespace parkingserver {

namespace device_message {

using boost::format;
void error(const string& err) {
    throw parse_error(err);
}
void error(boost::format& fmt) {
    error(str(fmt));
}
time_t parse_time(const string& timestr) {
    if (timestr.empty()) {
        error("time string null");
    }
	//	1、一个类型 basic_regex 的变量
    boost::regex e("(\\d{4})-(\\d{1,2})-(\\d{1,2}) (\\d{1,2}):(\\d{1,2}):(\\d{1,2}).*");
    boost::smatch sm;

	
	//  2、regex_math   匹配算法，测试一个字符串是否和一个正则式匹配，并通过match_results返回结果。
    boost::regex_match(timestr, sm, e);
    if (sm.size() != 7) {
        error("time parse error");
    }
	//	3、赋值
    struct tm timeinfo;
    timeinfo.tm_year = stoi(sm[1])-1900;
    timeinfo.tm_mon = stoi(sm[2])-1;
    timeinfo.tm_mday = stoi(sm[3]);
    timeinfo.tm_hour = stoi(sm[4]);
    timeinfo.tm_min = stoi(sm[5]);
    timeinfo.tm_sec = stoi(sm[6]);

    if (timeinfo.tm_mon < 0 || timeinfo.tm_mon > 11) {
        error("time parse month error");
    }
    if (timeinfo.tm_mday < 1 || timeinfo.tm_mday > 31) {
        error("time parse day error");
    }
    if (timeinfo.tm_hour < 0 || timeinfo.tm_hour > 23) {
        error("time parse hour error");
    }
    if (timeinfo.tm_min < 0 || timeinfo.tm_min > 59) {
        error("time parse min error");
    }
    if (timeinfo.tm_sec < 0 || timeinfo.tm_sec > 61) {
        error("time parse sec error");
    }
    return mktime(&timeinfo);
}
string encode_time(time_t t) {
    auto timeinfo = localtime(&t);
    return str(format("%04d-%02d-%02d %02d:%02d:%02d")
               % (timeinfo->tm_year + 1900)
               % (timeinfo->tm_mon + 1)
               % timeinfo->tm_mday
               % timeinfo->tm_hour
               % timeinfo->tm_min
               % timeinfo->tm_sec);
}
// optional<int> parse_color(const string& colorstr) {
//     if (colorstr.empty()) {
//         return boost::none;
//     }
//     int color = 0;
//     if (colorstr.find('A') != string::npos) {
//         color |= white;
//     }
//     if (colorstr.find('B') != string::npos) {
//         color |= grey;
//     }
//     if (colorstr.find('C') != string::npos) {
//         color |= yellow;
//     }
//     if (colorstr.find('D') != string::npos) {
//         color |= pink;
//     }
//     if (colorstr.find('E') != string::npos) {
//         color |= red;
//     }
//     if (colorstr.find('F') != string::npos) {
//         color |= purple;
//     }
//     if (colorstr.find('G') != string::npos) {
//         color |= green;
//     }
//     if (colorstr.find('H') != string::npos) {
//         color |= blue;
//     }
//     if (colorstr.find('I') != string::npos) {
//         color |= brown;
//     }
//     if (colorstr.find('J') != string::npos) {
//         color |= black;
//     }
//     if (colorstr.find('Z') != string::npos) {
//         color |= other;
//     }
//     if (color == 0) {
//         return boost::none;
//     }
//     return color;
// }

device_status json_parse_device_status(const string& jsonstr)
{
    // 表示整个 json 对象, Json::Value是一种很重要的类型，可以代表任意类型。如int, string, object, array
	Json::Value root;	
	// 用于读取的，将字符串转换为 Json::Value 对象
	Json::Reader reader;
	// 从字符串中读取数据
    if (!reader.parse(jsonstr, root)) {
        log()->info(format("failed to parse device status: %s") % jsonstr);
        error("parse error");
    }
	// 赋值 
	/*
	法1：int a=root.get("FaultState", 0).asInt();
	法2：int a=root[ "FaultState" ].asInt();
	*/
    device_status st;
    st.fault_state = root.get("FaultState", 0).asInt();
    st.sense_coil_state = root.get("SenseCoilState", 0).asInt();
    st.flash_light_state = root.get("FlashLightState", 0).asInt();
    st.indicato_light_state = root.get("IndicatoLightState", 0).asInt();
    return st;
}
string json_encode_device_status(const device_status& data)
{
    Json::Value root;
    root["FaultState"] = data.fault_state;
    root["SenseCoilState"] = data.sense_coil_state;
    root["FlashLightState"] = data.flash_light_state;
    root["IndicatoLightState"] = data.indicato_light_state;
	// 写入json字符串  
    Json::StyledWriter writer;
    return writer.write(root);
}

device_data json_parse_device_data(const string& jsonstr)
{
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(jsonstr, root)) {
        log()->info(format("failed to parse device data: %s") % jsonstr);
        error("parse device data error");
    }

    device_data data;
    data.longitude = root.get("Longitude", 0).asDouble();
    data.latitude = root.get("Latitude", 0).asDouble();
    data.vehicle_info_state = root.get("VehicleInfoState", -1).asInt();
    if (data.vehicle_info_state == -1) {
        error("parse VehicleInfoState error");
    }
    data.is_pic_url = root.get("IsPicUrl", -1).asInt();
    if (data.is_pic_url == -1) {
        error("parse IsPicUrl error");
    }
    data.lane_index = root.get("LaneIndex", 0).asInt();
    if (data.lane_index == 0) {
        error("parse LaneIndex error");
    }

    auto position = root.get("position", Json::Value());
    if (!position.empty() && position.isArray() && position.size() == 4) {
        vector<int> pos(4);
        pos[0] = position[0].asInt();
        pos[1] = position[1].asInt();
        pos[2] = position[2].asInt();
        pos[3] = position[3].asInt();
        data.position = pos;
    }

    auto direction = root.get("direction", 0).asInt();
    if (direction < 1 || direction > 14) {
        error("direction error");
    }
    data.direction = direction;

    data.plate_info1 = root.get("PlateInfo1", "").asString();
    if (data.plate_info1.empty()) {
        error("plate info 1 empty");
    }
    data.plate_info2 = root.get("PlateInfo2", "").asString();
    data.plate_color = root.get("PlateColor", -1).asInt();
    if (data.plate_color < 0 || data.plate_color > 9) {
        error("parse palte_color error");
    }
    data.plate_type = root.get("PlateType", 0).asInt();
    if (data.plate_type < 1 || data.plate_type > 99) {
        error("parse palte_type error");
    }
    data.pass_time = parse_time(root.get("PassTime", "").asString());

    data.vehicle_speed = root.get("VehicleSpeed", 0).asDouble();
    data.lane_min_speed = root.get("LaneMiniSpeed", 0).asDouble();
    data.lane_max_speed = root.get("LaneMaxSpeed", 0).asDouble();

    auto vehicle_type = root.get("VehicleType", -1).asInt();
    if (vehicle_type >= 0 && vehicle_type <= 15) {
        data.vehicle_type = vehicle_type;
    }
    data.vehicle_sub_type = root.get("VehicleSubType", 0).asInt();
    data.vehicle_color = root.get("VehicleColor", "").asString();
    auto vehicle_color_depth = root.get("VehicleColorDepth", -1).asInt();
    if (vehicle_color_depth >= 0 && vehicle_color_depth <= 2) {
        data.vehicle_color_depth = vehicle_color_depth;
    }
    data.vehicle_length = root.get("VehicleLength", 0).asInt();
    data.vehicle_state = root.get("VehicleState", 0).asInt();
    if (data.vehicle_state < 1 || data.vehicle_state > 4) {
        error("parse vehicle state error");
    }
    data.pic_count = root.get("PicCount", 0).asInt();
    if (data.pic_count < 0) {
        error("parse pic count error");
    }
    auto pic_type = root.get("PicType", Json::Value());
    if (data.pic_count > 0) {
        if (pic_type.empty() || !pic_type.isArray()) {
            error("parse pic type error");
        }
        if (pic_type.size() != data.pic_count) {
            error("parse pic type size not match");
        }
        for(int i=0; i<data.pic_count; i++) {
            data.pic_type.push_back(pic_type[i].asInt());
        }
    }

    data.plate_pic_url = root.get("PlatePicUrl", "").asString();
    data.vehicle_pic_1_url = root.get("VehiclePic1Url", "").asString();
    data.vehicle_pic_2_url = root.get("VehiclePic2Url", "").asString();
    data.vehicle_pic_3_url = root.get("VehiclePic3Url", "").asString();
    data.combine_pic_url = root.get("CombinedPicUrl", "").asString();
    data.alarm_action = root.get("AlarmAction", 0).asInt();
    if (data.alarm_action < 1 || data.alarm_action > 99) {
        error("parse alarm action error");
    }

    return data;
}
string json_encode_device_data(const device_data& data)
{
    Json::Value root;
    root["Longitude"] = data.longitude;
    root["Latitude"] = data.latitude;
    root["VehicleInfoState"] = data.vehicle_info_state;
    root["IsPicUrl"] = data.is_pic_url;
    root["LaneIndex"] = data.lane_index;
    if (data.position) {
        root["position"][0] = (*data.position)[0];
        root["position"][1] = (*data.position)[1];
        root["position"][2] = (*data.position)[2];
        root["position"][3] = (*data.position)[3];
    }

    root["direction"] = data.direction;
    root["PlateInfo1"] = data.plate_info1;
    root["PlateInfo2"] = data.plate_info2;
    root["PlateColor"] = data.plate_color;
    root["PlateType"] = data.plate_type;
    root["PassTime"] = encode_time(data.pass_time);
    root["VehicleSpeed"] = data.vehicle_speed;
    root["LaneMiniSpeed"] = data.lane_min_speed;
    root["LaneMaxSpeed"] = data.lane_max_speed;
    if (data.vehicle_type) {
        root["VehicleType"] = *data.vehicle_type;
    }
    if (data.vehicle_sub_type) {
        root["VehicleSubType"] = *data.vehicle_sub_type;
    }
    if (!data.vehicle_color.empty()) {
        root["VehicleColor"] = data.vehicle_color;
    }
    if (data.vehicle_color_depth) {
        root["VehicleColorDepth"] = *data.vehicle_color_depth;
    }
    root["vehicleLength"] = data.vehicle_length;
    root["VehicleState"] = data.vehicle_state;
    root["PicCount"] = data.pic_count;
    for(int i=0; i<data.pic_count; ++i) {
        root["PicType"][i] = data.pic_type[i];
    }
    root["PlatePicUrl"] = data.plate_pic_url;
    root["VehiclePic1Url"] = data.vehicle_pic_1_url;
    root["VehiclePic2Url"] = data.vehicle_pic_2_url;
    root["VehiclePic3Url"] = data.vehicle_pic_3_url;
    root["CombinePicUrl"] = data.combine_pic_url;
    root["AlarmAction"] = data.alarm_action;
    Json::StyledWriter writer;
    return writer.write(root);
}

time_t json_parse_keepalive(const string& jsonstr)
{
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(jsonstr, root)) {
        error("parse_keepalive error");
    }
    return parse_time(root.get("Time", "").asString());
}

Json::Value json_encode_road_gate_control(int cmd, optional<int> keep_sec, int mode)
{
    Json::Value root;
    root["BarrierGate"] = cmd;
    if (keep_sec) {
        root["BGKeepTime"] = *keep_sec;
    }
    root["BGMode"] = mode;

    return root;
}

Json::Value json_encode_led_control(int action, const string& content, int color,
                               int size, int mode, optional<int> duration)
{
    Json::Value root;
    root["LedAction"] = action;
    root["LedContent"] = content;
    root["LedColor"] = color;
    root["LedSize"] = size;
    root["LedMode"] = mode;
    if (duration) {
        root["LedDuration"] = *duration;
    }
    return root;
}

Json::Value json_encode_audio_control(const string& content, int volume)
{
    Json::Value root;
    root["VoiceContent"] = content;
    root["VoiceVolume"] = volume;
    return root;
}

Json::Value json_encode_capture_control(int mode, int recognize, int pic_num)
{
    Json::Value root;
    root["CaptureMode"] = mode;
    root["CaptureReg"] = recognize;
    root["CapturePicNum"] = pic_num;
    return root;
}

Json::Value json_encode_lock_action(int action)
{
    Json::Value root;
    root["LockAction"] = action;
    return root;
}

string json_encode_ipnc(const std::string& addr, int type, Json::Value message)
{
    Json::StyledWriter writer;
    auto msg = writer.write(message);
    return str(format("IPNC,%1%,%2%,%3%,%4%,%5%") % addr % type % check_sum(msg) % msg.length() % msg);
}

std::pair<int, string> json_parse_ipnc_response(const string& response)
{
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(response, root)) {
        error("parse ipnc response error");
    }

    int ret = root.get("ret", -1).asInt();
    if (ret == -1) {
        error("parse ipnc ret error");
    }
    string desc = root.get("desc", "").asString();
    return std::make_pair(ret, desc);
}

int check_sum(const string& msg)
{
    int sum = 0;
    for(int i=0; i<msg.size(); i++) {
        sum += msg[i];
    }
    return sum;
}


}
}
