#include "chargefactory.h"
#include <boost/algorithm/string.hpp>
#include "whitelistcharge.h"
#include "storedvaluecharge.h"
#include "monthlycharge.h"
#include "temporaryvipcharge.h"
#include "temporaryvehiclecharge.h"
#include "database.h"
#include "util.h"
#include "define.h"

namespace parkingserver {

	std::shared_ptr<Charge> ChargeFactory::Create(const PassVehicle& enter_vehicle,
		const PassVehicle& exit_vehicle,
		Json::Value& json) {
		// + add by zhengweixue  20160218
		int mode = QueryWhitelistMatchMode();
		log()->debug("charge release, get white list match mode: {}", mode);

		VIPVehicle vip;
		std::vector<VIPVehicle> vec_vip;

		switch (mode)
		{
			// 1.精确匹配
		case ExactMatch:
		{
			// 查询车辆是否是VIP
			auto findVIP = QueryVIPVehicle(exit_vehicle.plate_number1_, vip);
			if (findVIP) {			// 是VIP
				switch (vip.type) {
				case WhiteListVIP:	 // 白名单 自动放行
					return std::make_shared<WhiteListCharge>(enter_vehicle,
						exit_vehicle,
						vip,
						json);
				case StoredValueVIP: // 储值车
					return std::make_shared<StoredValueCharge>(enter_vehicle,
						exit_vehicle,
						vip,
						json);
				case MonthlyVIP:	 // 包月车
					return std::make_shared<MonthlyCharge>(enter_vehicle,
						exit_vehicle,
						vip,
						json);
				case TemporaryVIP:	 // 临时会员
					return std::make_shared<TemporaryVIPCharge>(enter_vehicle,
						exit_vehicle,
						vip,
						json);

				default:			// 临时车
					return std::make_shared<TemporaryVehicleCharge>(enter_vehicle,
						exit_vehicle,
						vip,
						json);
				}
			}
			else {					// 临时车
					return std::make_shared<TemporaryVehicleCharge>(enter_vehicle,
					exit_vehicle,
					vip,
					json);
			}
		}
		break;
		// 2.忽略省份汉字的精确匹配
		case IgnoreProvinceMatch:
		{
			auto findVIP = QueryVIPVehicle_IgnoreProvinceMatch(exit_vehicle.plate_number1_, vec_vip);

			if (findVIP)
			{
				// a)确认是否白名单、包月车、临时会员
				for (size_t i = 0; i < vec_vip.size(); i++)
				{
					switch (vec_vip[i].type)
					{
					case WhiteListVIP:	// 白名单
						return std::make_shared<WhiteListCharge>(enter_vehicle,
							exit_vehicle,
							vec_vip[i],
							json);
					case MonthlyVIP:	// 包月车
						return std::make_shared<MonthlyCharge>(enter_vehicle,
							exit_vehicle,
							vec_vip[i],
							json);
					case TemporaryVIP:	// 临时会员
						return std::make_shared<TemporaryVIPCharge>(enter_vehicle,
							exit_vehicle,
							vec_vip[i],
							json);
					default:			 // 临时车、储值会员
						break;
					}
				}
				// b)寻找储值车
				for (size_t i = 0; i < vec_vip.size(); i++)
				{
					switch (vec_vip[i].type)
					{
					case StoredValueVIP: // 储值车
						return std::make_shared<StoredValueCharge>(enter_vehicle,
							exit_vehicle,
							vec_vip[i],
							json);
					default: // 临时车
						return std::make_shared<TemporaryVehicleCharge>(enter_vehicle,
							exit_vehicle,
							vec_vip[i],
							json);
					}
				}
			}
			else { // 临时车
					return std::make_shared<TemporaryVehicleCharge>(enter_vehicle,
					exit_vehicle,
					vip,
					json);
			}
		}
		break;
		// 3.一个字符不一致的全字匹配
		case FuzzyMatch:
		{
			auto findVIP = QueryVIPVehicle_FuzzyMatch(exit_vehicle.plate_number1_, vec_vip);

			if (findVIP)
			{
				// a)确认是否白名单、包月车、临时会员
				for (size_t i = 0; i < vec_vip.size(); i++)
				{
					switch (vec_vip[i].type)
					{
					case WhiteListVIP: // 白名单
						return std::make_shared<WhiteListCharge>(enter_vehicle,
							exit_vehicle,
							vec_vip[i],
							json);
					case MonthlyVIP: // 包月车
						return std::make_shared<MonthlyCharge>(enter_vehicle,
							exit_vehicle,
							vec_vip[i],
							json);
					case TemporaryVIP: // 临时会员
						return std::make_shared<TemporaryVIPCharge>(enter_vehicle,
							exit_vehicle,
							vec_vip[i],
							json);
					default: // 临时车、储值会员
						break;
					}
				}
				// b)寻找储值车
				for (size_t i = 0; i < vec_vip.size(); i++)
				{
					switch (vec_vip[i].type)
					{
					case StoredValueVIP: // 储值车
						return std::make_shared<StoredValueCharge>(enter_vehicle,
							exit_vehicle,
							vec_vip[i],
							json);
					default: // 临时车
						return std::make_shared<TemporaryVehicleCharge>(enter_vehicle,
							exit_vehicle,
							vec_vip[i],
							json);
					}
				}
			}
			else { // 临时车
					return std::make_shared<TemporaryVehicleCharge>(enter_vehicle,
					exit_vehicle,
					vip,
					json);
			}
		}
		break;
		default:
		{
			// 查询车辆是否是VIP
			VIPVehicle vip;
			auto findVIP = QueryVIPVehicle(exit_vehicle.plate_number1_, vip);
			if (findVIP) { // 是VIP
				switch (vip.type) {
				case WhiteListVIP: // 白名单 自动放行
					return std::make_shared<WhiteListCharge>(enter_vehicle,
						exit_vehicle,
						vip,
						json);
				case StoredValueVIP: // 储值车
					return std::make_shared<StoredValueCharge>(enter_vehicle,
						exit_vehicle,
						vip,
						json);
				case MonthlyVIP: // 包月车
					return std::make_shared<MonthlyCharge>(enter_vehicle,
						exit_vehicle,
						vip,
						json);
				case TemporaryVIP: // 临时会员
					return std::make_shared<TemporaryVIPCharge>(enter_vehicle,
						exit_vehicle,
						vip,
						json);

				default: // 临时车
					return std::make_shared<TemporaryVehicleCharge>(enter_vehicle,
						exit_vehicle,
						vip,
						json);
				}
			}
			else { // 临时车
					return std::make_shared<TemporaryVehicleCharge>(enter_vehicle,
					exit_vehicle,
					vip,
					json);
			}
		}
		break;
	}
}
	// + add end


    // 查询车辆是否是VIP
 /*   VIPVehicle vip;
    auto findVIP = QueryVIPVehicle(exit_vehicle.plate_number1_, vip);
    if (findVIP) { // 是VIP
        switch (vip.type) {
            case WhiteListVIP: // 白名单 自动放行
                return std::make_shared<WhiteListCharge>(enter_vehicle,
                                                         exit_vehicle,
                                                         vip,
                                                         json);
            case StoredValueVIP: // 储值车
                return std::make_shared<StoredValueCharge>(enter_vehicle,
                                                           exit_vehicle,
                                                           vip,
                                                           json);
            case MonthlyVIP: // 包月车
                return std::make_shared<MonthlyCharge>(enter_vehicle,
                                                       exit_vehicle,
                                                       vip,
                                                       json);
            case TemporaryVIP: // 临时会员
                return std::make_shared<TemporaryVIPCharge>(enter_vehicle,
                                                            exit_vehicle,
                                                            vip,
                                                            json);

            default: // 临时车
                return std::make_shared<TemporaryVehicleCharge>(enter_vehicle,
                                                                exit_vehicle,
                                                                vip,
                                                                json);
        }
    }
    else { // 临时车
        return std::make_shared<TemporaryVehicleCharge>(enter_vehicle,
                                                        exit_vehicle,
                                                        vip,
                                                        json);
    }
	*/
}