#ifndef _PARKINGSERVER_RELEASEFACTORY_
#define _PARKINGSERVER_RELEASEFACTORY_
#include <memory>
#include "releasemode.h"

namespace parkingserver {

class ReleaseFactory {
  public:
    static std::shared_ptr<ReleaseMode> Create(device_pointer device,
                                               const PassVehicle& passvehicle,
                                               Transaction* transaction);
};
}
#endif
