#ifndef _PARKINGSERVER_CHARGERULE_
#define _PARKINGSERVER_CHARGERULE_
#include <ctime>
#include <boost/date_time.hpp>
#include <boost/optional.hpp>
#include "bill.h"

namespace parkingserver {

struct TimePeriod {
    boost::posix_time::time_duration begin;
    boost::posix_time::time_duration end;
};
struct ChargeRule {
    std::string id;
    std::string name;
    int priority;
    /* boost::optional<std::time_t> festival_begin_time; */
    /* boost::optional<std::time_t> festival_end_time; */
    /* boost::optional<int> week_day; */
    boost::posix_time::time_duration daylight_begin;
    boost::posix_time::time_duration night_begin;
    int free_time_length = 0;
    int day_fee_unit = 1;
    double day_fee_rate = 0;
    int day_fee_unit_big = 1;
    double day_fee_rate_big = 0;
    int night_fee_unit = 1;
    double night_fee_rate = 0;
    int night_fee_unit_big = 1;
    double night_fee_rate_big = 0;
    double max_fee = 0;
    double max_fee_big = 0;
    std::vector<TimePeriod> free_time_sections;
    int park_position = 0;
    std::string create_user;
    time_t create_time = 0;
    boost::optional<int> invalid_flag;
    time_t invalid_time = 0;
};

double ChargeByWhitelist();

double ChargeByMonthlyPayment();

double ChargeByTime(time_t enter_time, time_t exit_time,
                    int charge_pattern, int free_time_mode,
                    int vehicle_type, int park_position,
                    double discount, Bill& bill);
}
#endif
