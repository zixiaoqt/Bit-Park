#ifndef _TAXI_RELEASE_
#define _TAXI_RELEASE_
#include "releasemode.h"

namespace parkingserver {

class TaxiRelease : public ReleaseMode {
  public:
    TaxiRelease(device_pointer device,
                     Transaction* transaction);

    virtual void Release(const PassVehicle& passvehicle);
};
}
#endif
