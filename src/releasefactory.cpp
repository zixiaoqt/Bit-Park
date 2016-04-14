#include "releasefactory.h"
#include "chargerelease.h"
#include "unconditionalrelease.h"
#include "whitelistrelease.h"
#include "taxirelease.h"
#include "log.h"
#include "util.h"
#include "define.h"

namespace parkingserver {

std::shared_ptr<ReleaseMode> ReleaseFactory::Create(device_pointer device,
                                                    const PassVehicle& passvehicle,
                                                    Transaction* transaction) {
    // 是否是出租车
    if (util::IsTaxiVehicle(passvehicle)) {
        return std::make_shared<TaxiRelease>(device, transaction);
    }
    if (device->type == ParkEntrance) {
        switch (device->release_mode) {
            case ChargeReleaseMode:
                log()->warn("The charge releasing mode should not be band to entrance");
                break;
            case UnconditionalReleaseMode:
                return std::make_shared<UnconditionalRelease>(device,
                                                              transaction);
            case WhiteListReleaseMode:
                return std::make_shared<WhiteListRelease>(device,
                                                              transaction);
                break;
            case BlackListReleaseMode:
                break;
            default:
                log()->warn("Default releasing mode at enter gate");
                return std::make_shared<UnconditionalRelease>(device,
                                                              transaction);
                break;
        }
    }
    else if (device->type == ParkExit) {
        switch (device->release_mode) {
            case ChargeReleaseMode:
                return std::make_shared<ChargeRelease>(device,
                                                       transaction);
            case UnconditionalReleaseMode:
                return std::make_shared<UnconditionalRelease>(device,
                                                       transaction);
            case WhiteListReleaseMode:
                return std::make_shared<WhiteListRelease>(device,
                                                          transaction);
                break;
            case BlackListReleaseMode:
                break;
            default:
                log()->warn("Default releasing mode at exit gate");
                return std::make_shared<ChargeRelease>(device,
                                                       transaction);
                break;
        }
    }
    else {
        log()->warn("Unknown direction of device");
    }
    return nullptr;
}

}
