#include "session.h"
#include "define.h"

namespace parkingserver {

Session::Session(const std::shared_ptr<User> user)
        : user_(user){
}

void Session::bind_device(int direction, const std::string& device_id) {
    if (direction == PassIn) {
        bind_device_[0] = device_id;
    }
    else if (direction == PassOut) {
        bind_device_[1] = device_id;
    }
}

bool Session::has_bind_device(const std::string& device_id) {
    for(auto& id : bind_device_) {
        if (id == device_id) {
            return true;
        }
    }
    return false;
}

}
