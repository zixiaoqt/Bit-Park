#include "device.h"
#include <boost/make_shared.hpp>

namespace parkingserver {


device_pointer make_device(const string& deviceid,
                           const string& device_name,
                           const string& gateway_id,
                           const string& ip,
                           int port,
                           int type)
{
    auto devptr = std::make_shared<device>();
    devptr->device_id = deviceid;
    devptr->device_name = device_name;
    devptr->gateway_id = gateway_id;
    devptr->ip = ip;
    devptr->port = port;
    devptr->type = type;
    // devptr->location = location;
    devptr->is_alive = false;
    return devptr;
}

device_pointer device_keepalive(device_pointer dev, time_t t)
{
    dev->is_alive = true;
    dev->alive_time = t;
    return dev;
}

device_pointer device_update_status(device_pointer dev, const device_status& status)
{
    dev->status = status;
    return dev;
}

}
