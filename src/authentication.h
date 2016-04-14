#ifndef _AUTHENTICATION_H_
#define _AUTHENTICATION_H_
#include <boost/asio.hpp>
#include <thread>
#include <memory>

namespace parkingserver {
class Authentication {
  public:
    Authentication();
    ~Authentication();

    void authenticate();
    void detect_dongle();
    void handle_timer(const boost::system::error_code& e,
                      const std::string& loginfo);
  private:
    boost::asio::io_service ios_;
    boost::asio::io_service::work work_;
    std::thread thread_;
    std::shared_ptr<boost::asio::deadline_timer> timer_;
};

}
#endif
