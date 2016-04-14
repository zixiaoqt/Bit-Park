#ifndef _PARKINGSERVER_DEFINE_
#define _PARKINGSERVER_DEFINE_

namespace parkingserver {
    enum ParkingType {
        SPECIAL_VEHICLE_PARKING = 0,
        WHITE_LIST_PARKING = 1,
        STORED_VALUE_PARKING = 2,
        MOTHLY_PARKING = 3,
        TEMPORARY_VIP_PARKING = 4,
        TAXI_PARKING = 8,
        TEMPORARY_VEHICLE_PARKING = 9
    };

    enum PayType {
        CASH_PAY = 1,
        ALIPAY_PAY = 2,
        WECHAT_PAY = 3,
        UNIONPAY_PAY = 4,
        STORED_VALUE_PAY = 5,
        OTHER_PAY = 6
    };

    enum ExitType {
        AutoRelease = 1,
        ManualRelease = 2,
        SpecialVehicleRelease = 3,
        FreeRelease = 4,
        ProofRelease = 5
    };

    enum ParkReleaseMode {
        ChargeReleaseMode = 1,
        UnconditionalReleaseMode,
        WhiteListReleaseMode,
        BlackListReleaseMode
    };

    enum ParkType {
        ParkEntrance = 9,
        ParkExit = 10
    };

    enum PassDirection {
        PassIn = 9,
        PassOut = 10
    };

    enum VIPType {
        WhiteListVIP = 1,
        StoredValueVIP = 2,
        MonthlyVIP = 3,
        TemporaryVIP = 4
    };

    enum FreeTimeMode {
        FreeTimeChargeMode = 0,
        FreeTimeNoChargeMode = 1
    };
	
	// + add by zhengweixue 20160118
	enum WhitelistMatchMode{
		ExactMatch = 0,
		IgnoreProvinceMatch = 1,
		FuzzyMatch = 2
	};
	// + add end
}
#endif
