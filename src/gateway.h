#ifndef _PARKINGSERVER_GATEWAY_
#define _PARKINGSERVER_GATEWAY_
#include <string>
#include <json/json.h>

namespace parkingserver {

class Gateway {
  public:
    Gateway();
    ~Gateway() = default;

    Gateway(const std::string& id,
            const std::string& name,
            bool is_entrance,
            bool is_exit);

    Gateway(std::string&& id,
            std::string&& name,
            bool is_entrance,
            bool is_exit);

    Json::Value& toJson(Json::Value& json) const;

  public:
    std::string id_;
    std::string name_;
    bool is_entrance_ = false;
    bool is_exit_ = false;
};

}
#endif
