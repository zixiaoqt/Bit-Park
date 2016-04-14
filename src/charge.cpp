#include "charge.h"
#include "util.h"
#include "transaction.h"

namespace parkingserver {

Charge::Charge(const PassVehicle& enter_vehicle,
               const PassVehicle& exit_vehicle,
               const VIPVehicle& vip,
               Json::Value& json)
        : enter_vehicle_(enter_vehicle)
        , exit_vehicle_(exit_vehicle)
        , vip_(vip)
        , json_(json){
    parking_type_ = static_cast<ParkingType>(vip.type);
    park_position_ = QueryParkPosition(enter_vehicle.device_id_);

    bill_.enter_time_ = util::FormatLocalTime(enter_vehicle_.pass_time_);
    bill_.exit_time_ = util::FormatLocalTime(exit_vehicle_.pass_time_);
    bill_.duration_ = (exit_vehicle_.pass_time_ -
                       enter_vehicle_.pass_time_) / 60;
    bill_.discount_ = vip_.discount;

    charge_id_ = GenerateChargeRecordID();
    json_["ChargeID"] = charge_id_;
    json_["ParkingType"] = parking_type_;
    json_["TotalCharge"] = 0;
    json_["RequireCharge"] = 0;
}

void Charge::BuildChargeJson(double charge, double require_charge) {
    json_["TotalCharge"] = charge;
    json_["RequireCharge"] = require_charge;
    json_["Bill"] = bill_.toJson();
    if (charge == 0) {
        // 费用自动入库
        ChargeAutoInsertDB();
    }
}

void Charge::ChargeByTimeDuration(time_t enter_time, time_t exit_time) {
    auto charge = ChargeByTime(enter_time,
                               exit_time,
                               charge_pattern_,
                               freetime_mode_,
                               exit_vehicle_.plate_type_,
                               park_position_, vip_.discount,
                               bill_);
    json_["ParkingType"] = TEMPORARY_VEHICLE_PARKING;
    BuildChargeJson(charge, charge);
}

void Charge::ChargeByAccessTime() {
    ChargeByTimeDuration(enter_vehicle_.pass_time_,
                         exit_vehicle_.pass_time_);
}

void Charge::BuildAutoChargeBill() {
    bill_.free_duration_ = bill_.duration_;
    bill_.total_fee_ = 0;
    bill_.before_discounted_fee_ = 0;
    json_["Bill"] = bill_.toJson();
}

void Charge::FreeDurationBill(time_t begint_time,
                              time_t end_time) {
    Bill::Detail bill_detail;
    bill_detail.begin_time_ = util::FormatLocalTime(begint_time);
    bill_detail.end_time_ = util::FormatLocalTime(end_time);
    bill_detail.duration_ = (end_time - begint_time)/60;
    bill_detail.fee_ = 0;
    bill_.details_.push_back(bill_detail);
}

void Charge::RecordAutoCharge(double charge,
                              PayType pay_type,
                              double remain_money) {
    ChargeRecord record;
    record.exit_id = exit_vehicle_.pass_id_;
    record.enter_id = enter_vehicle_.pass_id_;
    record.charge = charge;
    record.actual_charge = 0;
    record.pay_type = pay_type;
    record.user = "0";
    record.parking_type = parking_type_;
    record.exit_type = 1;
    record.exit_comment = "";
    record.charge_detail = bill_.toString();
    record.remain_money = remain_money;
    record.discount = vip_.discount;

    log()->info("RecordAutoCharge auto insert database");	//	modefy by shenzixiao 2016/3/22 Charge==> RecordAutoCharge
    InsertChargeRecord(charge_id_, record);

    transaction_->delete_temporary_passrecord(enter_vehicle_.pass_id_);
}

void Charge::ChargeAutoInsertDB() {
    // 收费入库
    RecordAutoCharge(0, OTHER_PAY, 0);
}

void Charge::CalTimeBasedVIPCharge() {
    if (vip_.delete_status == "1") { // 未注销
        if (vip_.expiry_date == 0) {
            log()->info("Expiry date is invalid");
            ChargeByAccessTime();
        }
        else {
            if (exit_vehicle_.pass_time_ <= vip_.valid_begintime) {
                log()->info("Parking time is early than vip begin time");
                ChargeByAccessTime();
            }
            else if (exit_vehicle_.pass_time_ <= vip_.expiry_date) {
                log()->info("pass time {}; valid time {}",enter_vehicle_.pass_time_,
                            vip_.valid_begintime);
                if (enter_vehicle_.pass_time_ < vip_.valid_begintime) {
                    // 记录免费时段账单
                    FreeDurationBill(vip_.valid_begintime,
                                     exit_vehicle_.pass_time_);
                    ChargeByTimeDuration(enter_vehicle_.pass_time_,
                                         vip_.valid_begintime);
                }
                else {
                    log()->info("Parking time is in free duration");
                    // 自动放行账单
                    BuildAutoChargeBill();
                    // 费用自动入库
                    ChargeAutoInsertDB();
                }
            }
            else {
                if (enter_vehicle_.pass_time_ < vip_.valid_begintime) {
                    log()->info("Parking time involves vip time");
                    auto charge1 = ChargeByTime(enter_vehicle_.pass_time_,
                                                vip_.valid_begintime,
                                                charge_pattern_,
                                                freetime_mode_,
                                                exit_vehicle_.plate_type_,
                                                park_position_, vip_.discount,
                                                bill_);
                    // 记录免费时段账单
                    FreeDurationBill(vip_.valid_begintime, vip_.expiry_date);

                    auto charge2 = ChargeByTime(vip_.expiry_date,
                                                exit_vehicle_.pass_time_,
                                                charge_pattern_,
                                                freetime_mode_,
                                                exit_vehicle_.plate_type_,
                                                park_position_, vip_.discount,
                                                bill_);
                    auto charge = charge1 + charge2;
                    BuildChargeJson(charge, charge);
                }
                else if (enter_vehicle_.pass_time_ < vip_.expiry_date) {
                    log()->info("Exiting time is latter than vip expire time and "
                                "entering time is earlier than vip expire time");
                    // 记录免费时段账单
                    FreeDurationBill(enter_vehicle_.pass_time_, vip_.expiry_date);
                    ChargeByTimeDuration(vip_.expiry_date, exit_vehicle_.pass_time_);
                }
                else {
                    log()->info("Entering time is latter than vip expire time");
                    ChargeByAccessTime();
                }
            }
        }
    }
    else {  // 注销
        log()->info("VIP is logout");
        ChargeByAccessTime();
    }
}
}
