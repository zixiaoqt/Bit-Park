#include "bill.h"

namespace parkingserver {

Json::Value& Bill::toJson(Json::Value& json) const {
    json["EnterTime"] = enter_time_;
    json["ExitTime"] = exit_time_;
    json["Duration"] = duration_;
    json["FreeDuration"] = free_duration_;
    json["TotalFee"] = total_fee_;
    json["Discount"] = discount_;
    json["BeforeDiscountedFee"] = before_discounted_fee_;
    if (fee_exemption_ > 0) {
        json["FeeExemption"] = fee_exemption_;
    }
    if (!details_.empty()) {
        Json::Value detail_list;
        for (auto& d : details_) {
            Json::Value detail;
            detail["BeginTime"] = d.begin_time_;
            detail["EndTime"] = d.end_time_;
            detail["Duration"] = d.duration_;
            if (d.unbilled_duration_) {
                detail["UnbilledDuration"] = *d.unbilled_duration_;
            }
            detail["Fee"] = d.fee_;
            detail["RuleID"] = d.rule_id_;
            detail_list.append(detail);
        }
        json["List"] = detail_list;
    }
    return json;
}

Json::Value Bill::toJson() const {
    Json::Value json;
    return toJson(json);
}

std::string Bill::toString() const {
    auto json = toJson();
    Json::FastWriter writer;
    return writer.write(json);
}

}
