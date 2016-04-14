#include "taxicharge.h"

namespace parkingserver {

TaxiCharge::TaxiCharge(const PassVehicle& enter_vehicle,
                       const PassVehicle& exit_vehicle,
                       Json::Value& json)
        : Charge(enter_vehicle, exit_vehicle, VIPVehicle(), json){
    parking_type_ = TAXI_PARKING;
    json_["ParkingType"] = parking_type_;
}

void TaxiCharge::CalCharge() {
    log()->info("Cal charge about taxi");
    // 创建免费账单
    BuildAutoChargeBill();
    // 费用自动入库
    ChargeAutoInsertDB();
}
}
