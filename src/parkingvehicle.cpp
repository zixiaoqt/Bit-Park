#include "parkingvehicle.h"

namespace parkingserver {

ParkingVehicle::ParkingVehicle() {

}

ParkingVehicle::ParkingVehicle(const std::string& vehicle_id,
                               const std::string& plate_type,
                               const std::string& plate_info,
                               const std::string& gateway_name,
                               const std::string& photo)
        : vehicle_id_(vehicle_id)
        , plate_type_(plate_type)
        , plate_info_(plate_info)
        , gateway_name_(gateway_name)
        , photo_(photo){
}

ParkingVehicle::ParkingVehicle(std::string&& vehicle_id,
                               std::string&& plate_type,
                               std::string&& plate_info,
                               std::string&& gateway_name,
                               std::string&& photo)
        : vehicle_id_(std::move(vehicle_id))
        , plate_type_(std::move(plate_type))
        , plate_info_(std::move(plate_info))
        , gateway_name_(std::move(gateway_name))
        , photo_(std::move(photo)){
}

Json::Value& ParkingVehicle::toJson(Json::Value& json) const{
    json["ID"] = vehicle_id_;
    json["PlateType"] = plate_type_;
    json["PlateInfo"] = plate_info_;
    json["GatewayName"] = gateway_name_;
    json["Photo"] = photo_;
    return json;
}

}
