#include "log.h"

namespace parkingserver {

static std::shared_ptr<spdlog::logger> g_logger;

void InitLog(unsigned int log_level, unsigned int max_size, unsigned int max_files) {
    // spdlog::set_async_mode(1048576);
    // std::vector<spdlog::sink_ptr> sinks;
    // sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>("server",
    //                                                                        "log",
    //                                                                        10 * 1024 * 1024,
    //                                                                        3,
    //                                                                        false));
    // auto combined_logger = std::make_shared<spdlog::logger>("server", begin(sinks), end(sinks));
    // spdlog::register_logger(combined_logger);
    // spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%f] [thread %t] [%l] %v ");

    // g_logger = spdlog::get("server");

    // g_logger->set_level(static_cast<spdlog::level::level_enum>(log_level));
    InitLog(log_level, max_size, max_files, "server");
}

void InitLog(unsigned int log_level, unsigned int max_size, unsigned int max_files, const std::string &basename)
{
    spdlog::set_async_mode(1048576);
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(basename,
                                                                           "log",
                                                                           max_size,
                                                                           max_files,
                                                                           false));
	//	日志名称
	auto combined_logger = std::make_shared<spdlog::logger>("server", begin(sinks), end(sinks));
   
	spdlog::register_logger(combined_logger);
   
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%f] [thread %t] [%l] %v ");
	
    g_logger = spdlog::get("server");
	//	设置全局日志等级
    g_logger->set_level(static_cast<spdlog::level::level_enum>(log_level));
}

std::shared_ptr<spdlog::logger> log()
{
    return g_logger;
}

}
