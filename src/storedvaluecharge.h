#ifndef _STOREDVALUE_CHARGE_H__
#define _STOREDVALUE_CHARGE_H__
#include "charge.h"

namespace parkingserver {

class StoredValueCharge : public Charge{
  public:
    StoredValueCharge(const PassVehicle& enter_vehicle,
                      const PassVehicle& exit_vehicle,
                      const VIPVehicle& vip,
                      Json::Value& json);

    virtual void CalCharge();
};

}
#endif
