#include "whitelistcharge.h"

namespace parkingserver {

WhiteListCharge::WhiteListCharge(const PassVehicle& enter_vehicle,
                                 const PassVehicle& exit_vehicle,
                                 const VIPVehicle& vip,
                                 Json::Value& json)
        : Charge(enter_vehicle, exit_vehicle, vip, json){
}

void WhiteListCharge::CalCharge() {
    log()->info("Cal charge about white list");
    CalTimeBasedVIPCharge();
}
}
