#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>
#include <boost/date_time.hpp>
#include "passvehicle.h"
#include "database.h"
#include "define.h"

namespace parkingserver {
namespace util {

std::vector<std::string> split(const std::string &s, char delim);
std::string debug_strings(const std::vector<std::string> & strings);
std::string debug_string_length(const std::vector<std::string> & strings);

std::time_t GetCurrentTime();

std::string FormatTime(const boost::posix_time::ptime& pt,
                       const char* time_format);
std::string FormatLocalTime(std::time_t t, const char* time_format);

std::string FormatTime(const boost::posix_time::ptime& pt);
std::string FormatLocalTime(std::time_t t);

long SecondsToNextDay();

bool IsTaxiVehicle(const PassVehicle& exit_vehicle);

bool IsEmptyPlate(const std::string& plate_number);
bool IsNoMotorVehicle(const std::string& plate_number);

std::string GetPictureName(const std::string& url,
                           std::string* parent_directory);
std::string GetPictureDirectoryName(const std::string& directory);

std::string PictureName(const std::string& devid, std::time_t t, int index);
std::string PictureDirectory(const std::string& parent_directory, std::time_t t);

void WriteFile(const std::string& filename, const std::string& data);
void RemoveEmptyDirectory(const std::string& directory);

bool IsValidVIP(const VIPVehicle& vip, const PassVehicle& passvehicle);
ParkingType GetParkingType(const PassVehicle& passvehicle);

}}

#endif /* UTIL_H */
