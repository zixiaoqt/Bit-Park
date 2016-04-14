#include "releasemode.h"
#include "database.h"
#include "log.h"
#include "util.h"

namespace parkingserver {

ReleaseMode::ReleaseMode(device_pointer device,
                         Transaction* transaction)
        : device_(device)
        , transaction_(transaction) {
}

void ReleaseMode::InsertCharge(const string& enter_id,
                              const string& exit_id,
                               ParkingType parking_type,
                               float charge,
                               PayType pay_type,
                               ExitType exit_type,
                               const string& exit_comment,
                               const string& bill) {
    ChargeRecord record;
    record.exit_id = exit_id;
    record.enter_id = enter_id;
    record.charge = charge;
    record.actual_charge = 0;
    record.pay_type = pay_type;
    record.user = "0";
    record.parking_type = parking_type;
    record.exit_type = exit_type;
    record.exit_comment = exit_comment;
    record.discount = 1;
    record.charge_detail = bill;

    log()->info("ReleaseMode-InsertCharge auto insert database");//	modefy by shenzixiao 2016/3/22 Charge==> ReleaseMode-InsertCharge
    InsertChargeRecord(GenerateChargeRecordID(), record);

    transaction_->delete_temporary_passrecord(enter_id);
}

void ReleaseMode::HandleExitPassVehicle(const PassVehicle& passvehicle,
                                        ParkingType parking_type,
                                        bool is_auto_release) {
    auto handle_vehicle_fun = [&](const std::string& enter_id,
                                  boost::optional<Json::Value> entervehicle) {
        Json::Value json;
        json["CmdType"] = "ExitVehicleNotify";
        json["ExitVehicle"] = passvehicle.toJson();
        json["ParkingType"] = parking_type;
        json["TotalCharge"] = 0;
        json["RequireCharge"] = 0;
        json["OpenGate"] = is_auto_release ? 1 : 0;
        if (entervehicle != boost::none) {
            json["EnterVehicle"] = *entervehicle;
        }
        InsertCharge(enter_id, passvehicle.pass_id_, parking_type);
        auto notify_result = transaction_->notify_pass_vehicle(json,
                                                               device_->device_id);
        if (!notify_result && is_auto_release) {
            // 自动开闸
            transaction_->auto_open_exit_gateway(device_->ip);
        }
    };
    if(util::IsEmptyPlate(passvehicle.plate_number1_)) {
        handle_vehicle_fun("", boost::none);
        return;
    }
    // 查询入口过车纪录
    auto enter_vehicles = QueryEnterVehicle(passvehicle.plate_number1_);
    if (enter_vehicles.empty()) { // 未找到入口纪录
        handle_vehicle_fun("", boost::none);
    }
    else {
        // 寻找时间最近的入口过车纪录
        auto enter_vehicle = std::max_element(enter_vehicles.begin(),
                                              enter_vehicles.end(),
                                              [](const PassVehicle& l,
                                                 const PassVehicle& r){
                                                  return l.pass_time_ < r.pass_time_;
                                              });
        handle_vehicle_fun(enter_vehicle->pass_id_, enter_vehicle->toJson());
    }
}

}
