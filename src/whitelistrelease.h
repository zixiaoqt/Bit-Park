#ifndef _WHITELIST_RELEASE_
#define _WHITELIST_RELEASE_
#include "releasemode.h"
#include "database.h"

namespace parkingserver {

class WhiteListRelease : public ReleaseMode {
  public:
    WhiteListRelease(device_pointer device,
                     Transaction* transaction);

    virtual void Release(const PassVehicle& passvehicle);
};
}
#endif
