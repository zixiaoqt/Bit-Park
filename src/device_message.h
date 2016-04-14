#ifndef DEVICE_MESSAGE_H
#define DEVICE_MESSAGE_H

#include <string>
#include <boost/optional.hpp>
#include <json/json.h>
#include <json/writer.h>
#include "device_status.h"

namespace parkingserver {

namespace device_message {

using std::string;
using boost::optional;
using std::vector;

enum VEHICLE_INFO_STATE {
    realtime_vehicle_info = 0
    , history_vehicle_info = 1
};

enum VEHICLE_COLOR {
    white = 0x1
    , grey = 0x2
    , yellow = 0x4
    , pink = 0x8
    , red = 0x10
    , purple = 0x20
    , green = 0x40
    , blue = 0x80
    , brown = 0x100
    , black = 0x200
    , other = 0xFFFFFFFF
};


enum BARRIER_GATE_CMD
{
    bgc_null = 0              // 0  无操作
    , bgc_open = 1            // 1  道闸升起的命令，需要 KeepTime 字段
    , bgc_close = 2           // 2  道闸降下的命令，无需 KeepTime 字段
    , bgc_stop = 3            // 3  道闸停止的命令，无需 KeepTime 字段
    , bgc_keep_open = 4       // 4  道闸常开的命令，无需 KeepTime 字段
    , bgc_keep_close = 5      // 5  道闸常闭的命令，无需 KeepTime 字段
	, bgc_cancle_close = 6    // 6  道闸结束常闭的命令，无需 KeepTime 字段	// + add by shenzixiao 2016/3/28
};

enum BARRIER_GATE_MODE
{
    bgm_auto = 1              // 服务自动控制
    , bgm_manual = 2          // 人工控制
};

enum LED_ACTION
{
    la_display = 1
    , la_clear = 2
};

enum LED_COLOR
{
    lc_red = 1
    , lc_green = 2
    , lc_blue = 3
};

enum LED_MODE
{
    lm_fixed = 1
    , lm_scrolling = 2
};


enum CAPTURE_MODE
{
    cm_additional_capture = 1
    , cm_manual_open_capture = 2
};

enum CAPTURE_REG
{
    cr_yes = 0
    , cr_no = 1
};

enum LOCK_ACTION
{
    la_lock = 1
    , la_unlock = 2
};

enum IPNC_CMD_TYPE
{
    ict_control = 1
    , ict_query = 2
    , ict_response = 100
};

class parse_error : public std::exception
{
  public:
    parse_error(const string& err) {
        this->err = err;
    }
    string err;
};

struct device_data {
    double longitude;         // 抓拍点经度
    double latitude;          // 抓拍点纬度
    int vehicle_info_state;   // 过车信息状态. 0：实时过车信息。 1：历史过车信息。
    int is_pic_url;           // 0：是图片数据，图片数据负载在该 HTTP 请求的 ContentBody 中。
							  // 1：是图片Url。Url 在过车信息的文本消息中指明。
    int lane_index;           // 该值不可以为空。抓拍的车道编号。不可为空。从 1 开始编号。
    optional<vector<int>> position;     // 车牌在第一张图片中的位置信息
    int direction;                      // 行车方向
    string plate_info1;       // 主车牌号。未识别填"未知"
    string plate_info2;       // 辅车牌号。未识别填"未知"
    int plate_color;          // 车牌颜色. 0: 白色, 1: 黄色, 2: 蓝色, 3: 黑色, 4: 绿色, 9: 其他
    int plate_type;           // 车牌类型
    time_t pass_time;         // 过车时间。
    double vehicle_speed;     // 车速。
    double lane_min_speed;    // 车道低限速。
    double lane_max_speed;    // 车道高限速。
    optional<int> vehicle_type;         // 车辆类型。
    optional<int> vehicle_sub_type;     // 车辆子类型。
    string vehicle_color;     // 车辆颜色。
    optional<int> vehicle_color_depth;  // 车辆颜色深浅
    int vehicle_length;       // 车辆长度
    int vehicle_state;        // 行车状态
    int pic_count;            // 抓拍图片
    vector<int> pic_type;     // 图片类型
    string plate_pic_url;     // 车牌照片
    string vehicle_pic_1_url; // 第一张过车图片地址
    string vehicle_pic_2_url; // 第二张过车图片地址
    string vehicle_pic_3_url; // 第三张过车图片地址
    string combine_pic_url;   // 合成图片地址
    int alarm_action;         // 违法类型
};

device_status json_parse_device_status(const string& jsonstr);
string json_encode_device_status(const device_status& data);
device_data json_parse_device_data(const string& jsonstr);
string json_encode_device_data(const device_data& data);
time_t json_parse_keepalive(const string& jsonstr);
int check_sum(const string& msg);
string json_encode_ipnc(const std::string& addr, int type, Json::Value message);
std::pair<int, string> json_parse_ipnc_response(const string& response);
Json::Value json_encode_road_gate_control(int cmd, optional<int> keep_sec, int mode);
Json::Value json_encode_capture_control(int mode, int recognize, int pic_num);
Json::Value json_encode_led_control(int action, const string& content, int color,
                                    int size, int mode, optional<int> duration);
Json::Value json_encode_audio_control(const string& content, int volume);
Json::Value json_encode_lock_action(int action);


}
}

#endif /* DEVICE_MESSAGE_H */
