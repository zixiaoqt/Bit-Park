#include "passvehicle.h"
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <mysql++.h>

namespace parkingserver {
using namespace boost::algorithm;

// Json::Value& PassVehicle::toJson(Json::Value& json) const {
//     json["PassID"] = pass_id_;
//     json["PlateType"] = data_.plate_type;
//     json["PlateNumber1"] = data_.plate_info1;
//     json["PlateNumber2"] = data_.plate_info2;
//     json["DeviceID"] = device_id_;
//     json["PassDirection"] = data_.direction;
//     boost::filesystem::path path1(picture_path1_);
//     json["PictureURL1"] = str(boost::format("http://%1%/%2%")
//                               % server_ip_ % path1.filename().string());
//     boost::filesystem::path path2(picture_path2_);
//     json["PictureURL2"] = str(boost::format("http://%1%/%2%")
//                               % server_ip_ % path2.filename().string());
//     if (!data_.vehicle_color.empty()) {
//         json["VehicleColor"] = data_.vehicle_color;
//     }
//     if (data_.vehicle_type) {
//         json["VehicleType"] = *data_.vehicle_type;
//     }
//     if (data_.vehicle_sub_type) {
//         json["VehicleSubType"] = *data_.vehicle_sub_type;
//     }
//     mysqlpp::DateTime t(data_.pass_time);
//     json["PassTime"] = t.str();
//     return json;
// }

// Json::Value PassVehicle::toJson() const {
//     Json::Value json;
//     return toJson(json);
// }

PassVehicle::PassVehicle(const device_message::device_data& data) {
    plate_type_ = data.plate_type;
    plate_number1_ = data.plate_info1;
    plate_number2_ = data.plate_info2;
    plate_color_ = data.plate_color;
    pass_direction_ = data.direction;
    vehicle_color_ = data.vehicle_color;
    if (data.position) {
        auto pos = *data.position;
        plate_position_ = boost::str(boost::format("%1%,%2%,%3%,%4%")
                                     % pos[0] % pos[1] % pos[2] % pos[3]);
    }
    if (data.vehicle_type) {
        vehicle_type_ = *data.vehicle_type;
    }
    if (data.vehicle_sub_type) {
        vehicle_subtype_ = *data.vehicle_sub_type;
    }
    pass_time_ = data.pass_time;
}

Json::Value& PassVehicle::toJson(Json::Value& json) const {
    json["PassID"] = pass_id_;
    json["PlateType"] = plate_type_;
    json["PlateNumber1"] = plate_number1_;
    json["PlateNumber2"] = plate_number2_;
    json["PlateColor"] = plate_color_;
    json["DeviceID"] = device_id_;
    json["PassDirection"] = pass_direction_;
    json["PictureURL1"] = picture_url1_;
    json["PictureURL2"] = picture_url2_;
    json["VehicleColor"] = vehicle_color_;
    if (!plate_position_.empty()) {
        std::vector<std::string> positions;
        split(positions, plate_position_, is_any_of(","));
        for (auto& pos : positions) {
            json["PlatePosition"].append(std::stoi(pos));
        }
    }
    if (vehicle_type_) {
        json["VehicleType"] = *vehicle_type_;
    }
    if (vehicle_subtype_) {
        json["VehicleSubType"] = *vehicle_subtype_;
    }
    mysqlpp::DateTime t(pass_time_);
    json["PassTime"] = t.str();
    return json;
}

Json::Value PassVehicle::toJson() const {
    Json::Value json;
    return toJson(json);
}

}
