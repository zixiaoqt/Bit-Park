#include "storedvaluecharge.h"

namespace parkingserver {

StoredValueCharge::StoredValueCharge(const PassVehicle& enter_vehicle,
                                 const PassVehicle& exit_vehicle,
                                 const VIPVehicle& vip,
                                 Json::Value& json)
        : Charge(enter_vehicle, exit_vehicle, vip, json){
}

void StoredValueCharge::CalCharge() {
    log()->info("Cal charge about stored value user");
    if (vip_.delete_status == "1") { // 未注销
        // if (vip.freeze_status == "0") { // 未冻结
        auto charge = ChargeByTime(enter_vehicle_.pass_time_,
                                   exit_vehicle_.pass_time_,
                                   charge_pattern_,
                                   freetime_mode_,
                                   enter_vehicle_.plate_type_,
                                   park_position_, vip_.discount,
                                   bill_);
        auto left_money = vip_.remain_money - charge;
        if (left_money >= 0) { // 判断余额
            log()->trace("Stored value is enough, left money {}",
                         left_money);
            bill_.fee_exemption_ = charge;
            BuildChargeJson(charge, 0);
            // 收费入库
            RecordAutoCharge(charge, STORED_VALUE_PAY, left_money);
            // 更新储值余额
            UpdateStoredValue(vip_.id, left_money);
        }
        else {
            log()->trace("Stored value is not enough, left money {}",
                         left_money);
            bill_.fee_exemption_ = vip_.remain_money;
            BuildChargeJson(charge, -left_money);
            json_["ParkingType"] = TEMPORARY_VEHICLE_PARKING;
            // 更新储值余额
            UpdateStoredValue(vip_.id, 0);
        }
        //}
        // else {  // 冻结
        //     chargebytime_fun();
        // }
    }
    else {  // 注销
        ChargeByAccessTime();
    }
}

}
