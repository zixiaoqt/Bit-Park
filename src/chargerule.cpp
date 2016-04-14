#include "chargerule.h"
#include <boost/date_time/c_local_time_adjustor.hpp>
#include "database.h"
#include "sysconfig.h"
#include "util.h"
#include "define.h"

namespace parkingserver {
using namespace boost::gregorian;
using namespace boost::posix_time;

typedef boost::date_time::c_local_adjustor<ptime> local_adj;

struct ParkingTime {
    time_duration daylight_time;
    time_duration night_time;
};

double ChargeByWhitelist() {
    return 0.0f;
}

double ChargeByMonthlyPayment() {
    return 0.0f;
}

inline std::vector<time_period> DivideTimeByDay0(ptime begin_pt, ptime end_pt) {
    std::vector<time_period> periods;
    time_period period(begin_pt, end_pt);
    ptime day_begin = begin_pt;
    while (day_begin < end_pt) {
        ptime day_end = day_begin + days(1);
        time_period day_period(day_begin, day_end);
        if (period.intersects(day_period)) {
            periods.push_back(period.intersection(day_period));
        }
        day_begin = day_end;
    }
    return periods;
}

inline std::list<time_period> DivideTimeByDay1(ptime begin_pt, ptime end_pt) {
    std::list<time_period> periods;
    auto begin_date = begin_pt.date();
    auto end_date = end_pt.date();
    time_period period(begin_pt, end_pt);
    for (day_iterator iter = begin_date; iter <= end_date; ++iter) {
        time_period day_period(ptime(*iter, seconds(0)),
                               ptime(*iter, hours(24)));
        if (period.intersects(day_period)) {
            periods.push_back(period.intersection(day_period));
        }
    }
    return periods;
}

inline long CompleteTime(long time, long uint) {
    auto mod = time % uint;
    if (mod == 0) {
        return 0;
    }
    else {
        return uint - mod;
    }
}


inline double CalChargeTooDays(time_t enter_time, time_t exit_time, int days,
                               int vehicle_type, int park_position, Bill& bill) {
    double total_fee = 0;
    ChargeRule rule;
    if (!QueryMinPriorityRule(park_position, rule)) {
        log()->warn("Do not find charge rule");
    }
    else {
        log()->debug("Choose rule {}", rule.id);
        if (1 == vehicle_type) {
            log()->trace("Max fee of large vehicle is {}", rule.max_fee_big);
            total_fee = rule.max_fee_big * days;
        }
        else {
            log()->trace("Max fee of small vehicle is {}", rule.max_fee);
            total_fee = rule.max_fee * days;
        }
    }
    Bill::Detail bill_detail;
    bill_detail.begin_time_ = util::FormatLocalTime(enter_time);
    bill_detail.end_time_ = util::FormatLocalTime(exit_time);
    bill_detail.duration_ = (exit_time - enter_time) / 60;
    bill_detail.fee_ = total_fee;
    bill_detail.rule_id_ = rule.id;
    bill.details_.push_back(bill_detail);
    bill.discount_ = 1;
    bill.before_discounted_fee_ = total_fee;
    bill.total_fee_ = total_fee;
    return total_fee;
}

inline ParkingTime SubstrctFreeTimeSection(ParkingTime& parking_time,
                        const date& dt,
                        const std::list<time_period>& daylight_park_periods,
                        const std::list<time_period>& night_park_periods,
                        const ChargeRule& rule) {
    for (auto& section : rule.free_time_sections) {
        time_period free_period(ptime(dt, section.begin),
                                ptime(dt, section.end));
        for (auto& daylight_park_period : daylight_park_periods) {
            if (daylight_park_period.intersects(free_period)) {
                auto free_parking = daylight_park_period.intersection(free_period);
                log()->trace("Free daylight parking period: {}", free_parking);
                parking_time.daylight_time = parking_time.daylight_time -
                                             free_parking.length();
                parking_time.daylight_time.is_negative() ? minutes(0)
                        : parking_time.daylight_time;
            }
        }
        for (auto& night_park_period : night_park_periods) {
            if (night_park_period.intersects(free_period)) {
                auto free_parking = night_park_period.intersection(free_period);
                log()->trace("Free night parking period: {}", free_parking);
                parking_time.night_time = parking_time.night_time -
                                          free_parking.length();
                parking_time.night_time.is_negative() ? minutes(0)
                        : parking_time.night_time;
            }
        }
    }
    return parking_time;
}

inline ParkingTime CalInvalidParkingTime(const time_period& park_period,
                      const date& today,
                      const date& tomorrow,
                                         const ChargeRule& rule) {
    std::vector<time_period> night_periods =
            {time_period(ptime(today, seconds(0)),
                         ptime(today, rule.daylight_begin)),
             time_period(ptime(today, rule.night_begin),
                         ptime(tomorrow, rule.daylight_begin)),
             time_period(ptime(tomorrow, rule.night_begin),
                         ptime(tomorrow, hours(24)))};

    std::vector<time_period> daylight_periods =
            {time_period(ptime(today, rule.daylight_begin),
                         ptime(today, rule.night_begin)),
             time_period(ptime(tomorrow, rule.daylight_begin),
                         ptime(tomorrow, rule.night_begin))};

    std::list<time_period> night_park_periods;
    std::list<time_period> daylight_park_periods;
    ParkingTime parking_time{minutes(0), minutes(0)};

    for (auto& night_period : night_periods) {
        if (park_period.intersects(night_period)) {
            auto section = park_period.intersection(night_period);
            night_park_periods.push_back(section);
            parking_time.night_time += section.length();
        }
    }
    for (auto& daylight_period : daylight_periods) {
        if (park_period.intersects(daylight_period)) {
            auto section = park_period.intersection(daylight_period);
            daylight_park_periods.push_back(section);
            parking_time.daylight_time += section.length();
        }
    }
    log()->trace("Daylight parking time has ");
    for (auto& daylight_park_period : daylight_park_periods) {
        log()->trace("{}", daylight_park_period);
    }
    log()->trace("Night parking time has ");
    for (auto& night_park_period : night_park_periods) {
        log()->trace("{}", night_park_period);
    }

    log()->trace("Daylight parking time: {} and Night parking time: {}",
                 parking_time.daylight_time, parking_time.night_time);

    // 有效停车时间=停车时间-免费时段
    SubstrctFreeTimeSection(parking_time, today, daylight_park_periods,
                            night_park_periods, rule);
    SubstrctFreeTimeSection(parking_time, tomorrow, daylight_park_periods,
                            night_park_periods, rule);
    log()->trace("After substrcting free time segment, "
                 "Daylight parking time: {} and Night parking time: {}",
                 parking_time.daylight_time, parking_time.night_time);
    return parking_time;
}

inline ParkingTime BorrowTime(ParkingTime& parking_time,
                       int vehicle_type,
                       const time_duration& time_of_day,
                       const ChargeRule& rule) {
    auto daylight_park_minutes = parking_time.daylight_time.total_seconds() / 60;
    auto night_park_minutes = parking_time.night_time.total_seconds() / 60;
    long borrow_time = 0;
    if (time_of_day < rule.daylight_begin || time_of_day >= rule.night_begin) { //晚上
        int fee_unit;
        if (1 == vehicle_type) { // 大车
            fee_unit = rule.night_fee_unit_big;
        }
        else { // 小车
            fee_unit = rule.night_fee_unit;
        }
        borrow_time = CompleteTime(night_park_minutes, fee_unit);
        log()->trace("Night borrow daylight time: {}", borrow_time);
        parking_time.night_time += minutes(borrow_time);
        parking_time.daylight_time -= minutes(borrow_time);
        if (parking_time.daylight_time.is_negative()) {
            parking_time.daylight_time = minutes(0);
        }
    }
    else {  // 白天
        int fee_unit;
        if (1 == vehicle_type) { // 大车
            fee_unit = rule.day_fee_unit_big;
        }
        else { // 小车
            fee_unit = rule.day_fee_unit;
        }
        borrow_time = CompleteTime(daylight_park_minutes, fee_unit);
        log()->trace("Daylight borrow night time: {}", borrow_time);
        parking_time.daylight_time += minutes(borrow_time);
        parking_time.night_time -= minutes(borrow_time);
        if (parking_time.night_time.is_negative()) {
            parking_time.night_time = minutes(0);
        }
    }
    log()->trace("After borrowing time, Daylight parking time: {} and "
                 "Night parking time: {}",
                 parking_time.daylight_time, parking_time.night_time);
    return parking_time;
}

inline double CalOneDayCharge(double daylight_park_time, double night_park_time,
                              int day_fee_unit, double day_fee_rate,
                              int night_fee_unit, double night_fee_rate,
                              double max_fee) {
    double total_fee = 0;
    auto daylight_fee = std::ceil(daylight_park_time / day_fee_unit) * day_fee_rate;
    auto night_fee = std::ceil(night_park_time / night_fee_unit) * night_fee_rate;
    auto cal_today_fee = daylight_fee + night_fee;
    total_fee = cal_today_fee > max_fee ? max_fee : cal_today_fee;
    log()->trace("Daylight fee unit: {}; "
                 "Daylight fee rate: {}; Daylight fee: {}; "
                 "Night fee unit: {}; Night fee rate: {}; Night fee: {}; "
                 "Max fee: {}; Calculate today fee: {}; Today fee: {}",
                 day_fee_unit, day_fee_rate,
                 daylight_fee,
                 night_fee_unit, night_fee_rate,
                 night_fee,
                 max_fee, cal_today_fee, total_fee);
    return total_fee;
}

inline double CalOneDayChargeByVehicleType(double daylight_park_mintues,
                                           double night_park_mintues,
                                           int vehicle_type,
                                           const ChargeRule& rule) {
    double total_fee = 0;
    if (1 == vehicle_type) { // 大车
        total_fee = CalOneDayCharge(daylight_park_mintues, night_park_mintues,
                                    rule.day_fee_unit_big, rule.day_fee_rate_big,
                                    rule.night_fee_unit_big, rule.night_fee_rate_big,
                                    rule.max_fee_big);
    }
    else { // 小车
        total_fee = CalOneDayCharge(daylight_park_mintues, night_park_mintues,
                                    rule.day_fee_unit, rule.day_fee_rate,
                                    rule.night_fee_unit, rule.night_fee_rate,
                                    rule.max_fee);
    }
    return total_fee;
}

double CalChargeByTime(const ChargeRule& rule, const time_period& period,
                       int free_time_mode, int vehicle_type,
                       bool first_day, Bill& bill) {
    auto period_begin = period.begin();
    auto period_end = period.end();
    Bill::Detail bill_detail;
    bill_detail.begin_time_ = util::FormatTime(period_begin);
    bill_detail.end_time_ = util::FormatTime(period_end);
    bill_detail.duration_ = (period_end - period_begin).total_seconds() / 60;
    bill_detail.rule_id_ = rule.id;
    bill_detail.fee_ = 0;

    // 第一天减去免费时间
    if (first_day) {
        log()->trace("The period subtract free time {}", rule.free_time_length);
        bill.free_duration_ = rule.free_time_length;
        if (free_time_mode == FreeTimeNoChargeMode) {
            period_begin = period_begin + minutes(rule.free_time_length);
            log()->info("Mode of free time is no charging, so substract free time");
            if (period_begin >= period_end) {
                bill.details_.push_back(bill_detail);
                return 0;
            }
        }
    }

    auto new_period = time_period(period_begin, period_end);
    auto date = period_begin.date();
    auto next_date = date + date_duration(1);

    //计算白天和晚上的有效停车时间
    auto parking_time = CalInvalidParkingTime(new_period, date, next_date, rule);

    // 借时间
    BorrowTime(parking_time, vehicle_type, period_begin.time_of_day(), rule);

    auto daylight_park_minutes = parking_time.daylight_time.total_seconds() / 60;
    auto night_park_minutes = parking_time.night_time.total_seconds() / 60;

    // 根据车类型计算一天费用
    auto fee = CalOneDayChargeByVehicleType(daylight_park_minutes,
                                            night_park_minutes,
                                            vehicle_type,
                                            rule);

    bill_detail.fee_ = fee;
    bill.details_.push_back(bill_detail);
    return fee;
}

double ChargeByTime0(time_t enter_time, time_t exit_time,
                    int free_time_mode,
                    int vehicle_type, int park_position,
                    double discount, Bill& bill) {
    auto NoChargeBill = [&]() {
        Bill::Detail bill_detail;
        bill_detail.begin_time_ = util::FormatLocalTime(enter_time);
        bill_detail.end_time_ = util::FormatLocalTime(exit_time);
        bill_detail.duration_ = (exit_time - enter_time) / 60;
        bill_detail.fee_ = 0;
        bill.details_.push_back(bill_detail);
        bill.discount_ = 1;
        bill.before_discounted_fee_ = 0;
        bill.total_fee_ = 0;
    };

    auto enter_time_pt = local_adj::utc_to_local(from_time_t(enter_time));
    auto exit_time_pt = local_adj::utc_to_local(from_time_t(exit_time));
    auto periods = DivideTimeByDay0(enter_time_pt, exit_time_pt);
    auto periods_size = periods.size();
    if (periods.empty()) {
        NoChargeBill();
        return 0;
    }
    if (periods_size > 30) {
        log()->info("Parking time [{} days] is over 30 days", periods_size);
        return CalChargeTooDays(enter_time, exit_time, periods_size,
                                vehicle_type, park_position, bill);
    }

    ChargeRule rule;
    if (!QueryChargeRule(util::FormatLocalTime(enter_time),
                         park_position, rule)) {
        log()->error("Do not find charge rule");
        NoChargeBill();
        return 0;
    }
    bill.free_duration_ = rule.free_time_length;
    auto first_day_fee = CalChargeByTime(rule, periods[0], free_time_mode,
                                         vehicle_type, true, bill);
    log()->trace("Calculate fee of the first day: {}", first_day_fee);
    double total_fee = first_day_fee;
    if (periods_size > 1) {
        for (int i = 1; i < periods_size - 1; ++i) {
            Bill::Detail bill_detail;
            bill_detail.begin_time_ = util::FormatTime(periods[i].begin());
            bill_detail.end_time_ = util::FormatTime(periods[i].end());
            bill_detail.duration_ = (periods[i].end() - periods[i].begin()).total_seconds() / 60;
            bill_detail.fee_ = first_day_fee;
            bill_detail.rule_id_ = rule.id;
            bill.details_.push_back(bill_detail);
            total_fee += first_day_fee;
        }
        auto last_day_fee = CalChargeByTime(rule, periods[periods_size - 1],
                                            free_time_mode, vehicle_type,
                                            false, bill);
        log()->trace("Calculate fee of the last day: {}", last_day_fee);
        total_fee += last_day_fee;
    }
    auto actual_fee = total_fee * discount;
    bill.before_discounted_fee_ = total_fee;
    bill.total_fee_ = actual_fee;
    return actual_fee;
}

double ChargeByTime1(time_t enter_time, time_t exit_time,
                    int free_time_mode,
                    int vehicle_type, int park_position,
                    double discount, Bill& bill) {
    double total_fee = 0;
    auto enter_time_pt = local_adj::utc_to_local(from_time_t(enter_time));
    auto exit_time_pt = local_adj::utc_to_local(from_time_t(exit_time));
    auto periods = DivideTimeByDay1(enter_time_pt, exit_time_pt);
    auto periods_size = periods.size();
    if (periods_size > 30) {
        log()->info("Parking time [{} days] is over 30 days", periods_size);
        return CalChargeTooDays(enter_time, exit_time, periods_size,
                                vehicle_type, park_position, bill);
    }
    time_duration borrowed_time = minutes(0);
    bool first_day = true;
    for (auto& period : periods) {
        Bill::Detail bill_detail;
        bill_detail.begin_time_ = util::FormatTime(period.begin());
        bill_detail.end_time_ = util::FormatTime(period.end());
        bill_detail.duration_ = (period.end() - period.begin()).total_seconds() / 60;
        bill_detail.unbilled_duration_ = borrowed_time.total_seconds() / 60;
        bill_detail.fee_ = 0;

        log()->trace("The period is from {} to {}", period.begin(), period.end());

        auto period_begin = period.begin() + borrowed_time;
        log()->trace("The period subtract borrowed time {}", borrowed_time);

        auto period_end = period.end();
        if (period_begin >= period_end) {
            bill.details_.push_back(bill_detail);
            continue;
        }
        auto date = period_begin.date();
        ChargeRule rule;
        if (!QueryChargeRule(to_iso_extended_string(date),
                             park_position, rule)) {
            log()->warn("Do not find charge rule");
            bill.details_.push_back(bill_detail);
            continue;
        }
        bill_detail.rule_id_ = rule.id;
        log()->trace("Choose rule {}", rule.id);
        if (first_day) {
            first_day = false;
            log()->trace("The period subtract free time {}", rule.free_time_length);
            bill.free_duration_ = rule.free_time_length;
            if (free_time_mode == FreeTimeNoChargeMode) {
                period_begin = period_begin + minutes(rule.free_time_length);
                log()->info("Mode of free time is no charging, so substract free time");
                if (period_begin >= period_end) {
                    bill.details_.push_back(bill_detail);
                    continue;
                }
            }
        }

        auto new_period = time_period(period_begin, period_end);
        time_period after_midnight_period(ptime(date, seconds(0)),
                                          ptime(date, rule.daylight_begin));
        time_period before_midnight_period(ptime(date, rule.night_begin),
                                           ptime(date, hours(24)));
        time_period daylight_period(ptime(date, rule.daylight_begin),
                                    ptime(date, rule.night_begin));

        // 计算白天和黑天停车时长
        time_duration night_park_time = minutes(0);
        time_duration daylight_park_time = minutes(0);
        if (new_period.intersects(after_midnight_period)) {
            night_park_time = new_period.intersection(after_midnight_period).length();
        }
        if (new_period.intersects(before_midnight_period)) {
            night_park_time = night_park_time +
                              new_period.intersection(before_midnight_period).length();
        }
        if (new_period.intersects(daylight_period)) {
            daylight_park_time = new_period.intersection(daylight_period).length();
        }

        log()->trace("Daylight parking time: {}; Night parking time: {}",
                     daylight_park_time, night_park_time);

        // 计算停车时间与免费时段的白天交集、夜间交集时间
        time_duration night_free_time = minutes(0);
        time_duration daylight_free_time = minutes(0);
        for (auto& section : rule.free_time_sections) {
            time_period free_period(ptime(date, section.begin),
                                    ptime(date, section.end));
            log()->trace("Free section: {}", free_period);
            auto cal_free_time = [&](const time_period& tp) {
                if (new_period.intersects(tp)) {
                    return new_period.intersection(tp).length();
                }
                return time_duration(minutes(0));
            };
            if (section.begin < rule.daylight_begin) {
                if (section.end <= rule.daylight_begin) {
                    night_free_time = night_free_time +
                                      cal_free_time(free_period);
                    log()->trace("Inersection night, night free time: {}",
                                 night_free_time);
                }
                else if (section.end > rule.night_begin) {
                    night_free_time = night_free_time +
                                      cal_free_time(time_period(
                                          ptime(date, section.begin),
                                          ptime(date, rule.daylight_begin)));

                    daylight_free_time = daylight_free_time +
                                         cal_free_time(daylight_period);

                    night_free_time = night_free_time +
                                      cal_free_time(time_period(
                                          ptime(date, rule.night_begin),
                                          ptime(date, section.end)));
                    log()->trace("Inersection night->daylight->night, "
                                 "daylight free time: {}; night free time: {}",
                                 daylight_free_time, night_free_time);
                }
                else {
                    night_free_time = night_free_time +
                                      cal_free_time(time_period(
                                          ptime(date, section.begin),
                                          ptime(date, rule.daylight_begin)));

                    daylight_free_time = daylight_free_time +
                                         cal_free_time(time_period(
                                             ptime(date, rule.daylight_begin),
                                             ptime(date, section.end)));
                    log()->trace("Inersection night->daylight, "
                                 "daylight free time: {}; night free time: {}",
                                 daylight_free_time, night_free_time);
                }
            }
            else if (section.begin < rule.night_begin) {
                if (section.end <= rule.night_begin) {
                    daylight_free_time = daylight_free_time +
                                         cal_free_time(free_period);
                    log()->trace("Inersection daylight, daylight free time: {}",
                                 daylight_free_time);
                }
                else {
                    daylight_free_time = daylight_free_time +
                                         cal_free_time(time_period(
                                             ptime(date, section.begin),
                                             ptime(date, rule.night_begin)));

                    night_free_time = night_free_time +
                                      cal_free_time(time_period(
                                          ptime(date, rule.night_begin),
                                          ptime(date, section.end)));
                    log()->trace("Inersection daylight->night, "
                                 "daylight free time: {}; night free time: {}",
                                 daylight_free_time, night_free_time);
                }
            }
            else {
                night_free_time = night_free_time +
                                  cal_free_time(free_period);
                log()->trace("Inersection night, night free time: {}",
                             night_free_time);
            }
        }
        log()->trace("Daylight free time: {}; Night free time: {}",
                     daylight_free_time, night_free_time);

        daylight_park_time = daylight_park_time - daylight_free_time;
        night_park_time = night_park_time - night_free_time;

        log()->trace("After subtracting free time sections, "
                     "Daylight parking time: {}; Night parking time: {}",
                     daylight_park_time, night_park_time);

        auto daylight_park_minutes = daylight_park_time.total_seconds() / 60;
        long daylight_borrow_night_time = 0;
        if (1 == vehicle_type) {
            daylight_borrow_night_time = CompleteTime(daylight_park_minutes,
                                                      rule.day_fee_unit_big);
        }
        else {
            daylight_borrow_night_time = CompleteTime(daylight_park_minutes,
                                                      rule.day_fee_unit);
        }
        log()->trace("Daylight borrow night time: {}",
                     daylight_borrow_night_time);

        daylight_park_time = daylight_park_time + minutes(daylight_borrow_night_time);
        night_park_time = night_park_time - minutes(daylight_borrow_night_time);

        log()->trace("After daylight completing time, "
                     "Daylight parking time: {}; Night parking time: {}",
                     daylight_park_time, night_park_time);

        if (night_park_time.is_negative()) {
            night_park_time = minutes(0);
        }
        else {
            auto night_park_minutes = night_park_time.total_seconds() / 60;
            long night_borrow_tomorrow_time = 0;
            if (1 == vehicle_type) {
                night_borrow_tomorrow_time = CompleteTime(night_park_minutes,
                                                          rule.night_fee_unit_big);
            }
            else {
                night_borrow_tomorrow_time = CompleteTime(night_park_minutes,
                                                          rule.night_fee_unit);
            }
            log()->trace("Night borrow tomorrow time: {}", night_borrow_tomorrow_time);

            borrowed_time = minutes(night_borrow_tomorrow_time);
            night_park_time = night_park_time + borrowed_time;

            log()->trace("After night completing time, Night parking time: {}",
                         night_park_time);
        }

        auto daylight_total_minutes = daylight_park_time.total_seconds() / 60;
        auto night_total_minutes = night_park_time.total_seconds() / 60;

        log()->trace("After completing time, Daylight parking time: {}; "
                     "Night parking time: {}",
                     daylight_total_minutes, night_total_minutes);

        // 根据车类型计算一天费用
        auto today_fee = CalOneDayChargeByVehicleType(daylight_total_minutes,
                                                      night_total_minutes,
                                                      vehicle_type, rule);

        total_fee += today_fee;
        bill_detail.fee_ = today_fee;
        bill.details_.push_back(bill_detail);
    }

    auto actual_fee = total_fee * discount;
    bill.before_discounted_fee_ = total_fee;
    bill.total_fee_ = actual_fee;
    return actual_fee;
}

double ChargeByTime(time_t enter_time, time_t exit_time,
                    int charge_pattern, int free_time_mode,
                    int vehicle_type, int park_position,
                    double discount, Bill& bill) {
    // 小于免费时间
    ChargeRule rule;
    if (!QueryChargeRule(util::FormatLocalTime(enter_time),
                         park_position, rule)) {
        log()->warn("Do not find charge rule");
    }
    else {
		// + add by zhengweixue 20160201
		log()->info("charge rule name :{}", rule.name);
		// + add end
        if (exit_time - enter_time < (rule.free_time_length * 60)) {
            log()->info("Parking time in the free time");
            Bill::Detail bill_detail;
            bill_detail.begin_time_ = util::FormatLocalTime(enter_time);
            bill_detail.end_time_ = util::FormatLocalTime(exit_time);
            bill_detail.duration_ = (exit_time - enter_time) / 60;
            bill_detail.fee_ = 0;
            bill_detail.rule_id_ = rule.id;
            bill.details_.push_back(bill_detail);
            bill.free_duration_ = rule.free_time_length;
            bill.discount_ = 1;
            bill.before_discounted_fee_ = 0;
            bill.total_fee_ = 0;
            return 0;
        }
        log()->info("Parking time is longer than the free time");
    }
    switch (charge_pattern) {
        case 0:
            return ChargeByTime0(enter_time, exit_time, free_time_mode,
                                 vehicle_type, park_position, discount, bill);
        case 1:
            return ChargeByTime1(enter_time, exit_time, free_time_mode,
                                 vehicle_type, park_position, discount, bill);
        default:
            log()->warn("Unknown charge pattern {}, default using pattern 0",
                        charge_pattern);
            return ChargeByTime0(enter_time, exit_time,
                                 free_time_mode, vehicle_type, park_position,
                                 discount, bill);
    }
}

}
