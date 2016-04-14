#ifndef DEVICE_STATUS_H
#define DEVICE_STATUS_H

namespace parkingserver {

enum FAULT_STATE
{
    ok = 1
    , coil_fault = 2
    , temperature_exception = 3
    , graphics_exception = 4
    , recognition_exception = 5
    , network_exception = 6
    , time_exception = 7
    , config_exception = 8
    , coil_exception = 9
    , flash_light_exception = 10
    , indicator_light_exception = 11
    , pan_tilt_exception = 12
    , focus_blur = 13
    , glass_defilement = 14
};

enum SENSE_COIL_STATE
{
    no_signal = 0
    , signaled = 1
};

enum FLASH_LIGHT_STATE
{
    light_off = 0
    , light_on = 1
};

enum INDICATO_LIGHT_STATE
{
    indicato_light_off = 0
    , indicato_red_on = 1
    , indicato_green_on = 2
};


struct device_status {
    int fault_state;
    int sense_coil_state;
    int flash_light_state;
    int indicato_light_state;
};

}

#endif /* DEVICE_STATUS_H */
