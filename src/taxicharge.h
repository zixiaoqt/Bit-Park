#ifndef _TAXI_CHARGE_H__
#define _TAXI_CHARGE_H__
#include "charge.h"

namespace parkingserver {

class TaxiCharge : public Charge {
  public:
    TaxiCharge(const PassVehicle& enter_vehicle,
               const PassVehicle& exit_vehicle,
               Json::Value& json);

    virtual void CalCharge();
};

}
#endif
