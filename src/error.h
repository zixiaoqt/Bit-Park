#ifndef _PARKINGSERVER_ERROR_H_
#define _PARKINGSERVER_ERROR_H_
#include <system_error>
#include <websocketpp/common/cpp11.hpp>
#include <string>

namespace parkingserver {
namespace error {
enum value {
    general = 1,
    invalid_json = 2,
    invalid_user_password = 3,
    invalid_cmdtype = 4,
    match_platenumber = 5,
    sql_execute_exception = 6,
    database_connection_error = 7,
    invalid_passvehicle_record = 8,
    not_find_gateway_device = 9,
    gateway_device_not_connected = 10,
    invalid_operate_gateway = 11,
    no_space_to_save_pic = 12,
    operate_gateway_error = 13,
    invalid_direction = 14,
};

class category : public std::error_category {
  public:
    category() {}

    char const * name() const _WEBSOCKETPP_NOEXCEPT_TOKEN_ {
        return "parkingserver";
    }

    std::string message(int value) const {
        switch(value) {
            case error::general:
                return "Generic Error";
            case error::invalid_json:
                return "Invalid JSON";
            case error::invalid_user_password:
                return "Invalid Name OR Password";
            case error::invalid_cmdtype:
                return "Invalid Command Type";
            case error::match_platenumber:
                return "Not Match PlateNumber";
            case error::sql_execute_exception:
                return "Execute SQL Exception";
            case error::database_connection_error:
                return "Database Connection Error";
            case error::invalid_passvehicle_record:
                return "Invalid Pass Vehicle Record";
            case error::not_find_gateway_device:
                return "Not Find Gateway Device";
            case error::gateway_device_not_connected:
                return "Not Connect Gateway Device";
            case error::invalid_operate_gateway:
                return "Invalid operate gateway";
            case error::no_space_to_save_pic:
                return "No Space To Save Picture";
            case error::operate_gateway_error:
                return "Operate Gateway Error";
            case error::invalid_direction:
                return "Invalid Pass Vehicle Direction";
            default:
                return "Unknown";
        }
    }
};

inline const std::error_category& get_category() {
    static category instance;
    return instance;
}

}}

#endif
