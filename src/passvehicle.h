#ifndef _PARKINGSERVER_PASSVEHICLE_
#define _PARKINGSERVER_PASSVEHICLE_
#include <string>
#include <json/json.h>
#include <boost/optional.hpp>
#include "device_message.h"

namespace parkingserver {

/* class PassVehicle { */
/*   public: */
/*     Json::Value& toJson(Json::Value& json) const; */
/*     Json::Value toJson() const; */

/*   public: */
/*     const device_message::device_data& data_; */
/*     std::string picture_path1_; */
/*     std::string picture_path2_; */
/*     std::string device_id_; */
/*     std::string pass_id_; */
/*     std::string server_ip_; */
/* }; */

class PassVehicle {
public:
    PassVehicle() = default;
    ~PassVehicle() = default;
    PassVehicle(const device_message::device_data& data);
    Json::Value& toJson(Json::Value& json) const;
    Json::Value toJson() const;

public:
    std::string pass_id_;
    int plate_type_ = 2;
    std::string plate_number1_;
    std::string plate_number2_;
    int plate_color_ = 2;
    int pass_direction_ = 9;
    std::string picture_url1_;
    std::string picture_url2_;
    std::string vehicle_color_;
    boost::optional<int> vehicle_type_;
    boost::optional<int> vehicle_subtype_;
    std::string device_id_;
    std::string plate_position_;
    time_t pass_time_ = 0;
};

}

#endif
