#include "gateway.h"

namespace parkingserver {

Gateway::Gateway() {
}

Gateway::Gateway(const std::string& id,
        const std::string& name,
        bool is_entrance,
                 bool is_exit)
        : id_(id)
        , name_(name)
        , is_entrance_(is_entrance)
        , is_exit_(is_exit){
}

Gateway::Gateway(std::string&& id,
        std::string&& name,
        bool is_entrance,
                 bool is_exit)
        : id_(std::move(id))
        , name_(std::move(name))
        , is_entrance_(is_entrance)
        , is_exit_(is_exit){
}

Json::Value& Gateway::toJson(Json::Value& json) const{
    json["ID"] = id_;
    json["Name"] = name_;
    json["In"] = is_entrance_;
    json["Out"] = is_exit_;
    return json;
}

}
