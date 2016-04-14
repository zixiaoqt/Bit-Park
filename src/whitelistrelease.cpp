#include "whitelistrelease.h"
#include "log.h"
#include "util.h"
#include "taxirelease.h"

namespace parkingserver {

WhiteListRelease::WhiteListRelease(device_pointer device,
                                           Transaction* transaction)
        : ReleaseMode(device, transaction) {
}

void WhiteListRelease::Release(const PassVehicle& passvehicle) {
    log()->info("White list release");
    auto parking_type = util::GetParkingType(passvehicle);
    // 判断是入口过车还是出口过车
    if (passvehicle.pass_direction_ == ParkEntrance) { // 入口过车
        auto msg = passvehicle.toJson();
        msg["CmdType"] = "EnterVehicleNotify";
        // 是否是有效的白名单用户
        if (parking_type == WHITE_LIST_PARKING) {
            transaction_->notify_pass_vehicle(msg,
                                              device_->device_id);

            // 自动开闸
            transaction_->auto_open_entrance_gateway(device_->ip,
                                                     passvehicle.plate_number1_,
													 passvehicle.plate_type_);
        }
        else {
            log()->warn("Not a valid user of white list, can not open gateway");
            msg["OpenGate"] = 0;
            auto notify_result = transaction_->notify_pass_vehicle(msg,
                                                                   device_->device_id);
            if (!notify_result) {
                transaction_->led_show_plate_number(device_->ip,
                                                    passvehicle.plate_number1_);
            }
        }
    }
    else if (passvehicle.pass_direction_ == ParkExit) { // 出口过车
        bool is_auto_release = true;
        // 不是有效的白名单用户
        if (parking_type != WHITE_LIST_PARKING) {
            log()->info("Not a valid user of white list, not open gateway");
            is_auto_release = false;
        }
        HandleExitPassVehicle(passvehicle, parking_type, is_auto_release);
    }
}

}
