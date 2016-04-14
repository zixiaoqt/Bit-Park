#ifndef _WHITELIST_CHARGE_H__
#define _WHITELIST_CHARGE_H__
#include "charge.h"

namespace parkingserver {

class WhiteListCharge : public Charge {
  public:
    WhiteListCharge(const PassVehicle& enter_vehicle,
                    const PassVehicle& exit_vehicle,
                    const VIPVehicle& vip,
                    Json::Value& json);

    virtual void CalCharge();
};

}
#endif
