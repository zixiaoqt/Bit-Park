#include "temporaryvehiclecharge.h"

namespace parkingserver {

TemporaryVehicleCharge::TemporaryVehicleCharge(const PassVehicle& enter_vehicle,
                                 const PassVehicle& exit_vehicle,
                                 const VIPVehicle& vip,
                                 Json::Value& json)
        : Charge(enter_vehicle, exit_vehicle, vip, json){
    parking_type_ = TEMPORARY_VEHICLE_PARKING;
}

void TemporaryVehicleCharge::CalCharge() {
    log()->info("Cal charge about Temporary vehicle");
    ChargeByAccessTime();
}

}
