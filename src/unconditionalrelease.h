#ifndef _UNCONDITIONAL_RELEASE_
#define _UNCONDITIONAL_RELEASE_
#include "releasemode.h"

namespace parkingserver {

class UnconditionalRelease : public ReleaseMode {
  public:
    UnconditionalRelease(device_pointer device,
                         Transaction* transaction);

    virtual void Release(const PassVehicle& passvehicle);
};
}
#endif
