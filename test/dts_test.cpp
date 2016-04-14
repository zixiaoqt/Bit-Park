#include "../src/device_tcp_server.h"
#include "../src/device_message.h"
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
// #include <boost/lock.hpp>
#include <iostream>
#include <boost/format.hpp>

namespace dts = parkingserver::device_tcp_server;
namespace dm = parkingserver::device_message;
using boost::mutex;
using boost::unique_lock;
using boost::format;
using std::string;

int main(int ac, int av) 
{
    
    auto svr = dts::make_server(2015);
    auto clients = dts::make_clients();
    mutex mtx;

    auto thd = boost::thread(&dts::server_run, svr,
                             [&](dts::client_type client){
                                 unique_lock<mutex> lock(mtx);
                                 clients = dts::clients_add_client(clients, client);
                                 std::cout <<  "new client from : " << dts::client_address(client)
                                 << ":" << dts::client_port(client) << std::endl;
                             });

    while(true) {
        std::string cmd;
        getline(std::cin, cmd);
        if (cmd.find("help") != string::npos) {
            std::cout << "road-gate" << std::endl;
            std::cout << "led" << std::endl;
            std::cout << "audio" << std::endl;
            std::cout << "capture" << std::endl;
            std::cout << "heartbeat" << std::endl;
            continue;
        }

        int c = dm::ict_control;
        Json::Value root;
        if (cmd.find("road-gate") != string::npos) {
            root["RoadGate"] = dm::json_encode_road_gate_control(dm::bgc_open, 5, dm::bgm_auto);
        }
        if (cmd.find("led") != string::npos) {
            root["Led"] = dm::json_encode_led_control(dm::la_display, "hello", dm::lc_red, 10, dm::lm_fixed, 5);
        }
        if (cmd.find("audio") != string::npos) {
            root["Audio"] = dm::json_encode_audio_control("test audio", 5);
        }
        if (cmd.find("capture") != string::npos) {
            root["Capture"] = dm::json_encode_capture_control(dm::cm_additional_capture, dm::cr_yes, 1);
        }
        if (cmd.find("unlock") != string::npos) {
            root["Lock"] = dm::json_encode_lock_action(dm::la_unlock);
        } else if (cmd.find("lock") != string::npos) {
            root["Lock"] = dm::json_encode_lock_action(dm::la_lock);
        }
        if (cmd.find("heartbeat") != string::npos) {
            c = dm::ict_query;
        }
        unique_lock<mutex> lock(mtx);
        for(auto iter =clients->begin(); iter!=clients->end(); ) {
            try {
                auto client = *iter;
                auto res = dts::client_send_recv(
                    client,
                    dm::json_encode_ipnc(dts::client_address(client), c, root));
                std::cout << res << std::endl;
                auto parsed_res = dm::json_parse_ipnc_response(res);
                std::cout << "ret: " << parsed_res.first << ", desc: " << parsed_res.second << std::endl;
                ++iter;
            }
            catch(dts::client_error& err) {
                std::cout << err.what() << std::endl;
                iter = clients->erase(iter);
            }
        }
    }
    
    return 0;
}
