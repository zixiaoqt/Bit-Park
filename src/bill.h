#ifndef _PARKINGSEVER_BILL_
#define _PARKINGSEVER_BILL_
#include <string>
#include <vector>
#include <json/json.h>
#include <boost/optional.hpp>

namespace parkingserver {
class Bill{
  public:
    Json::Value& toJson(Json::Value& json) const;
    Json::Value toJson() const;
    std::string toString() const;

    class Detail {
      public:
        std::string begin_time_;
        std::string end_time_;
        std::string rule_id_;
        int duration_;
        boost::optional<int> unbilled_duration_;
        double fee_;
    };
    std::vector<Detail> details_;
    std::string enter_time_;
    std::string exit_time_;
    int duration_ = 0;
    int free_duration_ = 0;
    double total_fee_ = 0;
    double before_discounted_fee_ = 0;
    double discount_ = 1;
    double fee_exemption_ = 0;
};

}
#endif
