#ifndef _MONTHLY_CHARGE_H__
#define _MONTHLY_CHARGE_H__
#include "charge.h"

namespace parkingserver {

class MonthlyCharge : public Charge {
  public:
    MonthlyCharge(const PassVehicle& enter_vehicle,
                  const PassVehicle& exit_vehicle,
                  const VIPVehicle& vip,
                  Json::Value& json);

    virtual void CalCharge();
};
}
#endif
