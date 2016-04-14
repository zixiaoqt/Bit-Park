#include "temporaryvipcharge.h"

namespace parkingserver {

TemporaryVIPCharge::TemporaryVIPCharge(const PassVehicle& enter_vehicle,
                             const PassVehicle& exit_vehicle,
                             const VIPVehicle& vip,
                             Json::Value& json)
        : Charge(enter_vehicle, exit_vehicle, vip, json){

}

void TemporaryVIPCharge::CalCharge() {
    log()->info("Cal charge about Temporary VIP");
    CalTimeBasedVIPCharge();
}
}
