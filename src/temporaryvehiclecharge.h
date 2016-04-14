#ifndef _TEMPORARYVEHICLE_CHARGE_H__
#define _TEMPORARYVEHICLE_CHARGE_H__
#include "charge.h"

namespace parkingserver {

class TemporaryVehicleCharge : public Charge{
  public:
    TemporaryVehicleCharge(const PassVehicle& enter_vehicle,
                           const PassVehicle& exit_vehicle,
                           const VIPVehicle& vip,
                           Json::Value& json);

    virtual void CalCharge();
};

}
#endif
