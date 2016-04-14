#ifndef _CHARGE_RELEASE_
#define _CHARGE_RELEASE_
#include "releasemode.h"

namespace parkingserver {

class ChargeRelease : public ReleaseMode {
  public:
    ChargeRelease(device_pointer device,
                  Transaction* transaction);

    virtual void Release(const PassVehicle& passvehicle);

};
}
#endif
