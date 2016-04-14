#include "util.h"
#include <boost/date_time/local_time_adjustor.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <sstream>
#include <locale>
#include <chrono>
#include <iomanip>

namespace parkingserver {
namespace util {

using namespace boost::posix_time;
typedef boost::date_time::c_local_adjustor<ptime> local_adj;

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems)
{
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        if (!item.empty()) {
            elems.push_back(item);
        }
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim)
{
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

std::string debug_strings(const std::vector<std::string> & strings)
{
    std::string ret = "[";
    for(const auto& s : strings) {
        ret = ret + "'" + s + "'"+ " ";
    }
    ret += "]";
    return ret;
}

std::string debug_string_length(const std::vector<std::string> & strings)
{
    std::string ret = "[";
    for(const auto& s : strings) {
        ret = ret + " " + std::to_string(s.length()) + " "+ " ";
    }
    ret += "]";
    return ret;
}

std::time_t GetCurrentTime() {
    return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

std::string FormatTime(const ptime& pt, const char* time_format) {
    std::stringstream stream;
    std::locale currlocale(std::locale::classic(),
                                        new time_facet(time_format));
    stream.imbue(currlocale);
    stream << pt;
    return stream.str();
}

std::string FormatLocalTime(std::time_t t, const char* time_format) {
    return FormatTime(local_adj::utc_to_local(from_time_t(t)), time_format);
}

std::string FormatTime(const ptime& pt) {
    return FormatTime(pt, "%Y-%m-%d %H:%M:%S");
}

std::string FormatLocalTime(std::time_t t) {
    return FormatLocalTime(t, "%Y-%m-%d %H:%M:%S");
}

long SecondsToNextDay() {
    auto now_pt = boost::posix_time::second_clock::local_time();
    boost::posix_time::ptime next_day_pt(now_pt.date() +
                                         boost::gregorian::date_duration(1),
                                         boost::posix_time::time_duration(2, 0, 0));
    return (next_day_pt - now_pt).total_seconds();
}

bool IsTaxiVehicle(const PassVehicle& exit_vehicle) {
    // 查询出租车信息
    auto taxis = QueryTaxiInfo();
    for (auto& taxi : taxis) {
        boost::regex pattern(boost::to_upper_copy(taxi.plate_regex_pattern));
        if (boost::regex_search(boost::to_upper_copy(exit_vehicle.plate_number1_),
                                pattern) &&
            exit_vehicle.plate_color_ == 2) { //蓝色号牌
            return true;
        }
    }
    return false;
}

bool IsEmptyPlate(const std::string& plate_number) {
    return plate_number.empty() ||
           plate_number == "\346\227\240\350\275\246\347\211\214";
}

bool IsNoMotorVehicle(const std::string& plate_number) {
    return plate_number == "非机动车";
}

std::string GetPictureName(const std::string& url,
                           std::string* parent_directory) {
    std::vector<std::string> segments;
    boost::split(segments, url, boost::is_any_of("/"));
    auto size = segments.size();
    if (size >= 2) {
        if (segments[size - 2] == "PassVehicle" ||
            segments[size - 2] == "Voucher") {
            return segments[size - 1];
        }
        else {
            if (parent_directory != nullptr) {
                *parent_directory = segments[size - 2];
            }
            return segments[size - 2] + "/" + segments[size - 1];
        }
    }
    return "";
}

std::string GetPictureDirectoryName(const std::string& directory) {
    auto pos = directory.find_last_of("/\\");
    return directory.substr(pos + 1);
}

std::string PictureName(const std::string& devid, std::time_t t, int index)
{
    static std::string name[] = { "vehiclepic1" , "vehiclepic2" , "vehiclepic3" , "platepic", "combinedpic" };
    auto timestr = FormatLocalTime(t, "%Y%m%d%H%M%S");
    return devid+"_"+timestr+"_"+name[index-1];
}

std::string PictureDirectory(const std::string& parent_directory, std::time_t t) {
    auto directory = parent_directory + FormatLocalTime(t, "%Y-%m-%d");
    try {
        auto file_exist = boost::filesystem::exists(directory);
        if (!file_exist) {
            boost::filesystem::create_directory(directory);
        }
        return directory;
    }
    catch (boost::filesystem::filesystem_error& err) {
        log()->error("Create picture directory[{}] error:{}", directory,
                    err.what());
        return parent_directory;
    }
}

void WriteFile(const std::string& filename, const std::string& data)
{
    log()->info("write file {} {}", filename, data.length());
    std::ofstream ostr(filename, std::ios_base::out|std::ios_base::binary|std::ios_base::trunc);
    ostr.write(data.c_str(), data.length());
}

void RemoveEmptyDirectory(const std::string& directory) {
    try {
        if (boost::filesystem::is_empty(directory)) {
            log()->info("remove empty directory {}", directory);
            boost::filesystem::remove(directory);
        }
    }
    catch (boost::filesystem::filesystem_error& err) {
        log()->error("Remove empty directory[{}] error:{}", directory,
                   err.what());
    }

}

bool IsValidVIP(const VIPVehicle& vip,
                      const PassVehicle& passvehicle) {
    if (vip.delete_status == "1") { // 未注销
        if (passvehicle.pass_time_ >= vip.valid_begintime &&
            passvehicle.pass_time_ <= vip.expiry_date) {
            return true;
        }
    }
    return false;
}

ParkingType GetParkingType(const PassVehicle& passvehicle) {
    ParkingType parking_type = TEMPORARY_VEHICLE_PARKING;

	// + add by zhengweixue 20160118
	int mode = QueryWhitelistMatchMode();
	log()->debug("get white list match mode: {}", mode);

	switch (mode)
	{
		// 1.精确匹配
		case ExactMatch:
		{
			VIPVehicle vip;
			auto findVIP = QueryVIPVehicle(passvehicle.plate_number1_, vip);

			if (findVIP) { // 是VIP
				switch (vip.type) {
				case WhiteListVIP: // 白名单
					if (IsValidVIP(vip, passvehicle)) {
						parking_type = WHITE_LIST_PARKING;
					}
					break;
				case StoredValueVIP: // 储值车
					if (IsValidVIP(vip, passvehicle)) {
						parking_type = STORED_VALUE_PARKING;
					}
					break;
				case MonthlyVIP: // 包月车
					if (IsValidVIP(vip, passvehicle)) {
						parking_type = MOTHLY_PARKING;
					}
					break;
				case TemporaryVIP: // 临时会员
					if (IsValidVIP(vip, passvehicle)) {
						parking_type = TEMPORARY_VIP_PARKING;
					}
					break;
				default: // 临时车
					parking_type = TEMPORARY_VEHICLE_PARKING;
					break;
				}
			}
		}
			break;
		// 2.忽略省份汉字的精确匹配
		case IgnoreProvinceMatch:
		{
			std::vector<VIPVehicle> vec_vip;
			// 1.忽略省份字符的精确匹配
			auto findVIP = QueryVIPVehicle_IgnoreProvinceMatch(passvehicle.plate_number1_, vec_vip);

			if (findVIP)
			{
				for (size_t i = 0; i < vec_vip.size(); i++)
				{
					switch (vec_vip[i].type)
					{
					case WhiteListVIP: // 白名单
						if (IsValidVIP(vec_vip[i], passvehicle)) {
							parking_type = WHITE_LIST_PARKING;
						}
						break;
					case StoredValueVIP: // 储值车
						if (IsValidVIP(vec_vip[i], passvehicle)) {
							parking_type = STORED_VALUE_PARKING;
						}
						break;
					case MonthlyVIP: // 包月车
						if (IsValidVIP(vec_vip[i], passvehicle)) {
							parking_type = MOTHLY_PARKING;
						}
						break;
					case TemporaryVIP: // 临时会员
						if (IsValidVIP(vec_vip[i], passvehicle)) {
							parking_type = TEMPORARY_VIP_PARKING;
						}
						break;
					default: // 临时车
						parking_type = TEMPORARY_VEHICLE_PARKING;
						break;
					}

					if (parking_type == WHITE_LIST_PARKING || parking_type == STORED_VALUE_PARKING ||
						parking_type == MOTHLY_PARKING || parking_type == TEMPORARY_VIP_PARKING)
					{
						break;
					}
				}
			}
		}
			break;
		// 3.一个字符不一致的全字匹配
		case FuzzyMatch:
		{
			std::vector<VIPVehicle> vec_vip;
			// 1.忽略省份字符的精确匹配
			auto findVIP = QueryVIPVehicle_FuzzyMatch(passvehicle.plate_number1_, vec_vip);

			if (findVIP)
			{
				for (size_t i = 0; i < vec_vip.size(); i++)
				{
					switch (vec_vip[i].type)
					{
					case WhiteListVIP: // 白名单
						if (IsValidVIP(vec_vip[i], passvehicle)) {
							parking_type = WHITE_LIST_PARKING;
						}
						break;
					case StoredValueVIP: // 储值车
						if (IsValidVIP(vec_vip[i], passvehicle)) {
							parking_type = STORED_VALUE_PARKING;
						}
						break;
					case MonthlyVIP: // 包月车
						if (IsValidVIP(vec_vip[i], passvehicle)) {
							parking_type = MOTHLY_PARKING;
						}
						break;
					case TemporaryVIP: // 临时会员
						if (IsValidVIP(vec_vip[i], passvehicle)) {
							parking_type = TEMPORARY_VIP_PARKING;
						}
						break;
					default: // 临时车
						parking_type = TEMPORARY_VEHICLE_PARKING;
						break;
					}

					if (parking_type == WHITE_LIST_PARKING || parking_type == STORED_VALUE_PARKING ||
						parking_type == MOTHLY_PARKING || parking_type == TEMPORARY_VIP_PARKING)
					{
						break;
					}
				}
			}
		}
			break;
	}

	/*comment by zhengweixue 20160118
    VIPVehicle vip;
    auto findVIP = QueryVIPVehicle(passvehicle.plate_number1_, vip);

    if (findVIP) { // 是VIP
        switch (vip.type) {
            case WhiteListVIP: // 白名单
                if (IsValidVIP(vip, passvehicle)) {
                    parking_type = WHITE_LIST_PARKING;
                }
                break;
            case StoredValueVIP: // 储值车
                if (IsValidVIP(vip, passvehicle)) {
                    parking_type = STORED_VALUE_PARKING;
                }
                break;
            case MonthlyVIP: // 包月车
                if (IsValidVIP(vip, passvehicle)) {
                    parking_type = MOTHLY_PARKING;
                }
                break;
            case TemporaryVIP: // 临时会员
                if (IsValidVIP(vip, passvehicle)) {
                    parking_type = TEMPORARY_VIP_PARKING;
                }
                break;
            default: // 临时车
                parking_type = TEMPORARY_VEHICLE_PARKING;
                break;
        }
    }
	*/ 
	// + add by zhengweixue 20160118 模糊匹配


	// + add end
    log()->debug("Get Parking type: {}", parking_type);
    return parking_type;
}
}}
