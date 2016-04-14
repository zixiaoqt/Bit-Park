#ifndef _PARKINGSERVER_PARKINGVEHICLE_
#define _PARKINGSERVER_PARKINGVEHICLE_
#include <string>
#include <json/json.h>

namespace parkingserver {

class ParkingVehicle {
  public:
    ParkingVehicle();
    ~ParkingVehicle() = default;

    ParkingVehicle(const std::string& vehicle_id,
                   const std::string& plate_type,
                   const std::string& plate_info,
                   const std::string& gateway_name,
                   const std::string& photo);

    ParkingVehicle(std::string&& vehicle_id,
                   std::string&& plate_type,
                   std::string&& plate_info,
                   std::string&& gateway_name,
                   std::string&& photo);

    Json::Value& toJson(Json::Value& json) const;
  public:
    std::string vehicle_id_;
    std::string plate_type_;
    std::string plate_info_;
    std::string gateway_name_;
    std::string photo_;
};
}
#endif
