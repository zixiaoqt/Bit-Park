#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <string>
#include <boost/shared_ptr.hpp>

using boost::shared_ptr;
using std::string;

namespace parkingserver {

struct options
{
  public:
    string help;
    string version;

    unsigned short http_port;
    unsigned short dts_port;
    string pic_path;
    std::string db_server, db_name, db_user, db_password;
    unsigned int db_idle_time;

    string server_ip;
    unsigned short server_port;

    string voucher_pic_path;
    unsigned int log_level;
    unsigned int log_max_size;
    unsigned int log_max_files;
};

typedef shared_ptr<options> options_pointer;

options_pointer parse_options(int ac, char* av[], const string& executable_path);
string dump_options(options_pointer opts);

}

#endif /* _OPTIONS_H_ */
