#include "taxirelease.h"
#include "database.h"
#include "log.h"
#include "util.h"

namespace parkingserver {

TaxiRelease::TaxiRelease(device_pointer device,
                                   Transaction* transaction)
        : ReleaseMode(device, transaction) {
}

void TaxiRelease::Release(const PassVehicle& passvehicle) {
    log()->info("Taxi release");
    // 判断是入口过车还是出口过车
    if (passvehicle.pass_direction_ == ParkEntrance) { // 入口过车
        auto msg = passvehicle.toJson();
        msg["CmdType"] = "EnterVehicleNotify";
        msg["OpenGate"] = 1;
        transaction_->notify_pass_vehicle(msg,
                                          device_->device_id);
        // 自动开闸
        transaction_->auto_open_entrance_gateway(device_->ip,
												 passvehicle.plate_number1_,
												 passvehicle.plate_type_);
    }
    else if (passvehicle.pass_direction_ == ParkExit) { // 出口过车
        HandleExitPassVehicle(passvehicle, TAXI_PARKING, true);
    }
}

}
