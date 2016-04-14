#ifndef _PARKINGSERVER_SESSION_
#define _PARKINGSERVER_SESSION_
#include <string>
#include <memory>
#include "user.h"

namespace parkingserver {

class Session {
  public:
    Session(const std::shared_ptr<User> user);
    ~Session() = default;
    void bind_device(int direction, const std::string& device_id);
    bool has_bind_device(const std::string& device_id);
  public:
    std::shared_ptr<User> user_;
    std::string ip_;
    time_t heart_time_ = 0;
    std::string bind_device_[2];
};
}
#endif
