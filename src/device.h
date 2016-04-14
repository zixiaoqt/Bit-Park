#ifndef DEVICE_H
#define DEVICE_H

#include <string>
#include <memory>
#include <boost/chrono.hpp>
#include "device_status.h"

namespace parkingserver {

using std::string;

typedef boost::chrono::steady_clock clock;

struct device
{
public:
    string device_id;
    string device_name;
    string gateway_id;
    string ip;
    int port;
    int type = 0;
    string location;

    int release_mode = 0;  // 放行模式
    string bind_ip; // 绑定ip

    /* status */
    device_status status;
    bool is_alive;
    time_t alive_time;
};

typedef std::shared_ptr<device> device_pointer;

device_pointer make_device(const string& deviceid,
                           const string& device_name,
                           const string& gateway_id,
                           const string& ip,
                           int port,
                           int type);
device_pointer device_keepalive(device_pointer, time_t t);
device_pointer device_update_status(device_pointer, const device_status& status);

}

#endif /* DEVICE_H */
