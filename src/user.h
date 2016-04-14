#ifndef _PARKINGSERVER_USER_
#define _PARKINGSERVER_USER_
#include <string>
#include <json/json.h>
#include <boost/optional.hpp>

namespace parkingserver {

class User {
  public:
    /* User(); */
    /* ~User() = default; */
    /* User(const std::string& user, */
    /*      const std::string& password, */
    /*      const std::string& name, */
    /*      const std::string& phone, */
    /*      const std::string& authority, */
    /*      const std::string& description); */

    /* User(std::string&& user, */
    /*      std::string&& password, */
    /*      std::string&& name, */
    /*      std::string&& phone, */
    /*      std::string&& authority, */
    /*      std::string&& description); */

    Json::Value& toJson(Json::Value& json) const;
  public:
    std::string user_;
    std::string password_;
    std::string name_;
    boost::optional<std::string> phone_;
    boost::optional<std::string> authority_;
    boost::optional<std::string> description_;
};
}
#endif
