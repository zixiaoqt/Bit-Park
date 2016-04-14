#ifndef _PARKING_SERVER_LOG__
#define _PARKING_SERVER_LOG__
#include <spdlog/spdlog.h>

namespace parkingserver {

void InitLog(unsigned int log_level, unsigned int max_size, unsigned int max_files);
void InitLog(unsigned int log_level, unsigned int max_size, unsigned int max_files, const std::string &basename);
std::shared_ptr<spdlog::logger> log();
}

#endif
