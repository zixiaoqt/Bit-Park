#ifndef _CHARGE_FACTORY_H__
#define _CHARGE_FACTORY_H__
#include <memory>
#include "charge.h"

namespace parkingserver {

class ChargeFactory {
public:
    ChargeFactory() = default;
    ~ChargeFactory() = default;

public:
    static std::shared_ptr<Charge> Create(const PassVehicle& enter_vehicle,
                                          const PassVehicle& exit_vehicle,
                                          Json::Value& json);
};
}
#endif
