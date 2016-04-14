#include "user.h"

namespace parkingserver {
// User::User() {
// }
// User::User(const std::string& user,
//            const std::string& password,
//            const std::string& name,
//            const std::string& phone,
//            const std::string& authority,
//            const std::string& description)
//         : user_(user)
//         , password_(password)
//         , name_(name)
//         , phone_(phone)
//         , authority_(authority)
//         , description_(description){
// }

// User::User(std::string&& user,
//            std::string&& password,
//            std::string&& name,
//            std::string&& phone,
//            std::string&& authority,
//            std::string&& description)
//         : user_(std::move(user))
//         , password_(std::move(password))
//         , name_(std::move(name))
//         , phone_(std::move(phone))
//         , authority_(std::move(authority))
//         , description_(std::move(description)){
// }

Json::Value& User::toJson(Json::Value& json) const{
    json["User"] = user_;
    json["Name"] = name_;
    if (phone_) {
        json["Phone"] = *phone_;
    }
    if (description_) {
        json["Description"] = *description_;
    }
    if (authority_) {
        json["Authority"] = *authority_;
    }
    return json;
}
}
