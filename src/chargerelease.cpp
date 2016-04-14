#include "chargerelease.h"
#include "database.h"
#include "log.h"
#include "util.h"

namespace parkingserver {

ChargeRelease::ChargeRelease(device_pointer device,
                             Transaction* transaction)
    : ReleaseMode(device, transaction) {
}

void ChargeRelease::Release(const PassVehicle& passvehicle) {
    log()->info("Charge release");
    // 判断是入口过车还是出口过车
    if (passvehicle.pass_direction_ == ParkEntrance) { // 入口过车
        log()->error("The charge releasing mode should not be band to entrance");
    }
    else if (passvehicle.pass_direction_ == ParkExit) { // 出口过车
        auto notify_pass_vehicle_fun = [&]() {
            Json::Value json;
            json["CmdType"] = "ExitVehicleNotify";
            json["ExitVehicle"] = passvehicle.toJson();
            json["ParkingType"] = util::GetParkingType(passvehicle);
            auto notify_result = transaction_->notify_pass_vehicle(json,
                                                                   device_->device_id);
            if (!notify_result) {
                // 自动开闸
                transaction_->auto_open_exit_gateway(device_->ip);
                // 费用入库
                InsertCharge("", passvehicle.pass_id_,
                             TEMPORARY_VEHICLE_PARKING, 0.0f, OTHER_PAY,
                             AutoRelease, "客户端不在线");
            }
        };
        if (util::IsEmptyPlate(passvehicle.plate_number1_)) {
            notify_pass_vehicle_fun();
            return;
        }
        // 查询入口过车纪录
        auto enter_vehicles = QueryEnterVehicle(passvehicle.plate_number1_);
        if (enter_vehicles.empty()) { // 未找到入口纪录
			//	add by shenzixiao 2016/3/22
			log()->debug("Charge release enter_vehicles is empty");
			//	add end 
            notify_pass_vehicle_fun();
        }
        else {
            Json::Value json;
            json["CmdType"] = "ExitVehicleNotify";
            json["ExitVehicle"] = passvehicle.toJson();

            // 寻找时间最近的入口过车纪录
            auto enter_vehicle = std::max_element(enter_vehicles.begin(),
                                                  enter_vehicles.end(),
                                                  [](const PassVehicle& l,
                                                     const PassVehicle& r){
                                                      return l.pass_time_ < r.pass_time_;
                                                  });
            json["EnterVehicle"] = enter_vehicle->toJson();
            auto charge = transaction_->handle_charge(*enter_vehicle,
                                                      passvehicle,
                                                      json);
            json["OpenGate"] = charge == 0 ? 1 : 0;
            auto notify_result = transaction_->notify_pass_vehicle(json,
                                                                   device_->device_id);
            if (!notify_result) {
                if (charge == 0) {
                    // 自动开闸
                    transaction_->auto_open_exit_gateway(device_->ip);
                }
                else {
                    // 只显示费用
                    try {
                        transaction_->led_show_charge(device_->ip, charge, 30);
                    }
                    catch(std::exception& e) {
                        log()->warn("LED show charge error:{}", e.what());
                    }
                    // 费用入库
                    std::string bill;
                    if (!json["Bill"].isNull()) {
                        Json::FastWriter writer;
                        bill = writer.write(json["Bill"]);
                    }
                    InsertCharge(enter_vehicle->pass_id_, passvehicle.pass_id_,
                                 TEMPORARY_VEHICLE_PARKING, charge, CASH_PAY,
                                 ManualRelease, "客户端不在线", bill);
                }
            }
        }
    }
}

}
