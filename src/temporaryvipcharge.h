#ifndef _TEMPORARYVIP_CHARGE_H__
#define _TEMPORARYVIP_CHARGE_H__
#include "charge.h"

namespace parkingserver {

class TemporaryVIPCharge : public Charge {
  public:
    TemporaryVIPCharge(const PassVehicle& enter_vehicle,
                       const PassVehicle& exit_vehicle,
                       const VIPVehicle& vip,
                       Json::Value& json);

    virtual void CalCharge();
};
}
#endif
