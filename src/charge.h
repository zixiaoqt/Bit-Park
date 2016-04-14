#ifndef _VIP_CHARGE_H__
#define _VIP_CHARGE_H__
#include <functional>
#include "passvehicle.h"
#include "database.h"
#include "define.h"
#include "transaction.h"

namespace parkingserver {

class Charge {
public:
    Charge(const PassVehicle& enter_vehicle,
           const PassVehicle& exit_vehicle,
           const VIPVehicle& vip,
           Json::Value& json);

    virtual ~Charge() = default;

    virtual void CalCharge() = 0;

    void SetFreeTimeMode(int mode) {
        freetime_mode_ = mode;
    }

    void SetChargePattern(int pattern) {
        charge_pattern_ = pattern;
    }

    void SetTranscation(Transaction* transaction) {
        transaction_ = transaction;
    }

    ParkingType GetParkingType() const {return parking_type_;}
protected:
    void BuildChargeJson(double charge, double require_charge);
    // 按照进场和出场实际计费
    void ChargeByTimeDuration(time_t enter_time, time_t exit_time);
    void ChargeByAccessTime();

    // 自动放行详单
    void BuildAutoChargeBill();
    // 免费时段账单
    void FreeDurationBill(time_t begint_time, time_t end_time);
    // 自动放行的收费入库
    void RecordAutoCharge(double charge,
                          PayType pay_type,
                          double remain_money);
    void ChargeAutoInsertDB();

    // 计算具有时效性的VIP费用，依据车辆进场时间和出场时间与VIP有效时间的关系分为6种情况
    //        vip生效时间    vip失效时间
    //              o---------o
    // 1. o----o
    // 2.      o------o
    // 3. o-------------------------------o
    // 4.             o-----o
    // 5.                o--------o
    // 6.                         o-------o
    void CalTimeBasedVIPCharge();
protected:
    const PassVehicle& enter_vehicle_;
    const PassVehicle& exit_vehicle_;
    VIPVehicle vip_;
    Json::Value& json_;

    Bill bill_;
    std::string charge_id_;
    int park_position_ = 0;
    ParkingType parking_type_ = TEMPORARY_VEHICLE_PARKING;
    double remain_money_ = 0;

    int freetime_mode_;
    int charge_pattern_;

    Transaction* transaction_;
};
}
#endif
