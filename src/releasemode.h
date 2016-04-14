#ifndef _PARKINGSERVER_RELEASEMODE_
#define _PARKINGSERVER_RELEASEMODE_
#include <boost/optional.hpp>
#include <string>
#include "device.h"
#include "transaction.h"
#include "define.h"

namespace parkingserver {

class ReleaseMode {
  public:
    ReleaseMode(device_pointer device,
                Transaction* transaction);
    virtual ~ReleaseMode() = default;

  public:
    virtual void Release(const PassVehicle& passvehicle) = 0;

  protected:
    void InsertCharge(const string& enter_id,
                     const string& exit_id,
                      ParkingType parking_type,
                      float charge = 0.0f,
                      PayType pay_type = OTHER_PAY,
                      ExitType exit_type = AutoRelease,
                      const string& exit_comment = "",
                      const string& bill = "");
    void HandleExitPassVehicle(const PassVehicle& passvehicle,
                               ParkingType parking_type,
                               bool is_auto_release);
  protected:
    device_pointer device_;
    Transaction* transaction_;
};
}

#endif
