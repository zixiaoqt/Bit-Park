#include "monthlycharge.h"

namespace parkingserver {

MonthlyCharge::MonthlyCharge(const PassVehicle& enter_vehicle,
                                     const PassVehicle& exit_vehicle,
                                     const VIPVehicle& vip,
                                     Json::Value& json)
        : Charge(enter_vehicle, exit_vehicle, vip, json){

}

void MonthlyCharge::CalCharge() {
    log()->info("Cal charge about monthly user");
    CalTimeBasedVIPCharge();
}
}
