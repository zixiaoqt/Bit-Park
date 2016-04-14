#include <iostream>
#include <chrono>
#include "../src/chargerule.h"
#include "../src/bill.h"
#include "../src/singleton.h"
#include "../src/mysqlconnectionpool.h"
#include "../src/options.h"
#include "../src/log.h"

using namespace std;
using namespace std::chrono;

int main(int ac, char* av[])
{
    auto opts = parkingserver::parse_options(ac, av, "../build/");
    if (!opts->help.empty()) {
        std::cout << opts->help << std::endl;
        return 0;
    }

    int charge_pattern = 1;
    int free_mode = 0;
    int vechile_type = 2;
    parkingserver::InitLog(opts->log_level, opts->log_max_size, opts->log_max_files);
    parkingserver::log()->info("Welcome to spdlog!");

    parkingserver::log()->info(dump_options(opts));

    parkingserver::Singleton<parkingserver::MysqlConnectionPool>::Instance(opts->db_server,
                                                       opts->db_name,
                                                       opts->db_user,
                                                       opts->db_password,
                                                       opts->db_idle_time);
    std::vector<std::tuple<int, int>> parking_times = {
        // parking time < free time
        std::make_tuple(
            1440028800,  // 2015-8-20 08:00:00
            1440030000  // 2015-8-20 08:20:00
                        ),

        // parking time > free time
        std::make_tuple(
            1440028800,  // 2015-8-20 8:00:00
            1440031200  // 2015-8-20 8:40:00
                        ),

        std::make_tuple(
            1440072000,  // 2015-8-20 20:00:00
            1440074400  // 2015-8-20 20:40:00
                        ),
        // parking time > 1 and parking time < 2
        // daylight
        std::make_tuple(
            1440028800,  // 2015-8-20 8:00:00
            1440033300  // 2015-8-20 9:15:00
                        ),

        // night
        std::make_tuple(
            1440072000,  // 2015-8-20 20:00:00
            1440078300  // 2015-8-20 21:45:00
                        ),
        std::make_tuple(
            1440079200,  // 2015-8-20 23:00:00
            1440092700  // 2015-8-21 01:45:00
                        ),

        // night-daylight
        std::make_tuple(
            1440018000,  // 2015-8-20 05:00:00
            1440024300  // 2015-8-20 6:45:00
                        ),
        std::make_tuple(
            1440020700,  // 2015-8-20 05:45:00
            1440022200  // 2015-8-20 6:10:00
                        ),
        std::make_tuple(
            1440020700,  // 2015-8-20 05:45:00
            1440024300  // 2015-8-20 6:45:00
                        ),

        // daylight-night
         std::make_tuple(
            1440061200,  // 2015-8-20 17:00:00
            1440065700  // 2015-8-20 18:15:00
                        ),
        std::make_tuple(
            1440063900,  // 2015-8-20 17:45:00
            1440066000  // 2015-8-20 18:20:00
                        ),

        // parking time > a half day
        // night-daylight-night
        std::make_tuple(
            1440013500,  // 2015-8-20 03:45:00
            1440090000  // 2015-8-21 01:00:00
                        ),
        // daylight-night-daylight
        std::make_tuple(
            1440036600,  // 2015-8-20 10:10:00
            1440117900   // 2015-8-21 8:45:00
                        ),
        // daylight-night
        std::make_tuple(
            1440022200,  // 2015-8-20 6:10:00
            1440090000  // 2015-8-21 01:00:00
                        ),
        // night-daylight
        std::make_tuple(
            1440072900,  // 2015-8-20 20:15:00
            1440115860  // 2015-8-21 08:11:00
                        ),

        // parking time > one day
        // night-daylight-night
        std::make_tuple(
            1440013500,  // 2015-8-20 03:45:00
            1440115860  // 2015-8-21 08:11:00
                        ),
        // daylight-night-daylight
        std::make_tuple(
            1440022200,  // 2015-8-20 6:10:00
            1440117900   // 2015-8-21 8:45:00
                        ),

        // parking time > two day
        std::make_tuple(
            1440013500,  // 2015-8-20 03:45:00
            1440202260  // 2015-8-22 08:11:00
                        ),
        std::make_tuple(
            1440022200,  // 2015-8-20 6:10:00
            1440241860  // 2015-8-22 19:11:00
                        ),

        // parking time > one week
        std::make_tuple(
            1440013500,  // 2015-8-20 03:45:00
            1440720660  // 2015-8-28 08:11:00
                        ),

        // parking time = one month
        std::make_tuple(
            1438358400,  // 2015-8-1 00:00:00
            1441036800  // 2015-9-1 00:00:00
                        ),

        // parking time = three months
        std::make_tuple(
            1435680000,  // 2015-6-1 00:00:00
            1441036800  // 2015-9-1 00:00:00
                        ),

        // parking time = one half year
        std::make_tuple(
            1425139200,  // 2015-3-1 00:00:00
            1441036800  // 2015-9-1 00:00:00
                        ),

        // parking time = one year
        std::make_tuple(
            1409500800,  // 2014-9-1 00:00:00
            1441036800  // 2015-9-1 00:00:00
                        ),

        // free time segment
        std::make_tuple(
            1440040200,  // 2014-8-20 11:10:00
            1440042300  // 2015-8-20 11:45:00
                        ),
        std::make_tuple(
            1440040200,  // 2014-8-20 11:10:00
            1440045000  // 2015-8-20 12:30:00
                        ),

        // seconds
        std::make_tuple(
            1440028800,  // 2015-8-20 08:00:00
            1440030601  // 2015-8-20 08:30:01
                        ),
    };

    for (auto& parking_time : parking_times) {
        parkingserver::log()->trace("Start to calculate charge: {}-{}",
                                    std::get<0>(parking_time),
                                    std::get<1>(parking_time));
        parkingserver::Bill bill;
        auto start = system_clock::now();
        auto charge = parkingserver::ChargeByTime(std::get<0>(parking_time),
                                                  std::get<1>(parking_time),
                                                  charge_pattern,
                                                  free_mode,
                                                  vechile_type,
                                                  0,
                                                  1,
                                                  bill);
        auto delta = system_clock::now() - start;
        auto delta_d = duration_cast<duration<double>>(delta).count();
        cout << "Query time: " << delta_d * 1000 << "ms"<< endl;

        cout << "Charge: " << charge << endl;
        cout << "Bill:" << endl << bill.toString() << endl;
        bill.details_.clear();
    }

    return 0;
}
