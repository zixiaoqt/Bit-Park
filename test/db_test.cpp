#include "../src/database.h"
#include "../src/log.h"
#include "../src/options.h"

int main(int ac, char* av[])
{
  namespace pks = parkingserver;

  auto opts = pks::parse_options(ac, av, ".");
  if (!opts->help.empty()) {
    std::cout << opts->help << std::endl;
    return 0;
  }
  
  pks::InitLog(opts->log_level, opts->log_max_size, opts->log_max_files);
  pks::log()->info("Welcome to spdlog!");

  
  pks::Singleton<pks::MysqlConnectionPool>::Instance(opts->db_server,
                                                     opts->db_name,
                                                     opts->db_user,
                                                     opts->db_password,
                                                     opts->db_idle_time);


  {
    Json::Value query;
    Json::StyledWriter writer;
    pks::log()->info("query all parking vehicle");
    pks::log()->info(writer.write(pks::QueryParkingVehicle(query)));
  }
  {
    Json::Value query;
    query["StartTime"] = "2014-03-25 16:02:30";
    Json::StyledWriter writer;
    pks::log()->info("query parking vehicle since 2014-03-25 16:02:30");
    pks::log()->info(writer.write(pks::QueryParkingVehicle(query)));
  }
  {
    Json::Value query;
    query["EndTime"] = "2014-03-25 16:02:30";
    Json::StyledWriter writer;
    pks::log()->info("query parking vehicle before 2014-03-25 16:02:30");
    pks::log()->info(writer.write(pks::QueryParkingVehicle(query)));
  }
  {
    Json::Value query;
    query["VehicleColor"] = "A";
    Json::StyledWriter writer;
    pks::log()->info("query parking vehicle with color A");
    pks::log()->info(writer.write(pks::QueryParkingVehicle(query)));
  }
  {
    Json::Value query;
    query["StartTime"] = "2014-03-25 16:02:30";
    query["EndTime"] = "2014-03-25 16:02:30";
    Json::StyledWriter writer;
    pks::log()->info("query parking vehicle between 2014-03-25 16:02:30 and 2014-03-25 16:02:30");
    pks::log()->info(writer.write(pks::QueryParkingVehicle(query)));
  }
  {
    Json::Value query;
    query["StartTime"] = "2014-03-25 16:02:30";
    query["VehicleColor"] = "A";
    Json::StyledWriter writer;
    pks::log()->info("query parking vehicle with color A since 2014-03-25 16:02:30");
    pks::log()->info(writer.write(pks::QueryParkingVehicle(query)));
  }
  {
    Json::Value query;
    query["EndTime"] = "2014-03-25 16:02:30";
    query["VehicleColor"] = "A";
    Json::StyledWriter writer;
    pks::log()->info("query parking vehicle with color A before 2014-03-25 16:02:30");
    pks::log()->info(writer.write(pks::QueryParkingVehicle(query)));
  }
  {
    Json::Value query;
    query["StartTime"] = "2014-03-25 16:02:30";
    query["EndTime"] = "2014-03-25 16:02:30";
    query["VehicleColor"] = "A";
    Json::StyledWriter writer;
    pks::log()->info("query parking vehicle with color A between 2014-03-25 16:02:30 and 2014-03-25 16:02:30");
    pks::log()->info(writer.write(pks::QueryParkingVehicle(query)));
  }

  {
    Json::Value query;
    query["PlateNumber"] = "\351\262\201";  // 鲁的utf8编码
    Json::StyledWriter writer;
    pks::log()->info("query parking vehicle with '\351\262\201' in plate number");
    pks::log()->info(writer.write(pks::QueryParkingVehicle(query)));
  }

  {
    Json::Value query;
    query["ParkingType"] = 5;
    Json::StyledWriter writer;
    pks::log()->info("query parking vehicle with park type 5");
    pks::log()->info(writer.write(pks::QueryParkingVehicle(query)));
  }
  
  return 0;
}
