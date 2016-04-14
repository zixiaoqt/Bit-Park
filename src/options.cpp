#include "options.h"
#include <boost/make_shared.hpp>
#include <boost/program_options.hpp>
#include <fstream>
#include <sstream>
#include "version.h"

namespace parkingserver {

using boost::make_shared;
namespace po = boost::program_options;

options_pointer parse_options(int ac, char* av[], const string& executable_path)
{
    auto ptr = make_shared<options>();

    string config_file;
	// 1、声明一个options_description对象generic，用于添加需要解析的选项名字
    po::options_description generic("Generic options");
	// 2、长选项名（“help”）必须在前，短选项名（“h”）如果有在逗号之后。还需要赋默认值
    generic.add_options()
            ("help,h", "produce help message")
            ("version,v", "version of parking_server")
            ("config,c", po::value<string>(&config_file)->default_value(executable_path + "/settings.txt"),
             "settings file name");

    po::options_description config ("Configuration");
    config.add_options()
            ("http_port", po::value<unsigned short>(&ptr->http_port)->default_value(8088),
             "set http listen port")
            ("dts_port", po::value<unsigned short>(&ptr->dts_port)->default_value(2015),
             "set device tcp server listen port")
            ("pic_path", po::value<string>(&ptr->pic_path)->default_value("./"),
             "set picture save path")
            ("db_server", po::value<string>(&ptr->db_server)->default_value("127.0.0.1:3306"),
             "database server")
            ("db_name", po::value<string>(&ptr->db_name)->default_value("parking"),
             "database name")
            ("db_user", po::value<string>(&ptr->db_user)->default_value("root"),
             "database user")
            ("db_password", po::value<string>(&ptr->db_password)->default_value(""),
             "database password")
            ("db_idle_time", po::value<unsigned int>(&ptr->db_idle_time)->default_value(60),
             "idle time of database connection")
            ("server_ip", po::value<string>(&ptr->server_ip)->default_value("127.0.0.1"),
             "set server ip")
            ("server_port", po::value<unsigned short>(&ptr->server_port)->default_value(9000),
             "set server port")
            ("voucher_pic_path", po::value<string>(&ptr->voucher_pic_path)->default_value("./"),
             "set picture path of voucher")
            ("log_level", po::value<unsigned int>(&ptr->log_level)->default_value(2),
             "level of log")
            ("log_max_size", po::value<unsigned int>(&ptr->log_max_size)->default_value(31457280),
             "max_size of log")
            ("log_max_files", po::value<unsigned int>(&ptr->log_max_files)->default_value(10),
             "max_files of log");

    po::options_description cmdline_options;
    cmdline_options.add(generic).add(config);

    po::options_description config_file_options;
    config_file_options.add(config);

	//	3、解析，结果保存在variables_map对象里。
    po::variables_map vm;
	//	MFC,win32 main（）函数的参数ac, av
    po::store(po::parse_command_line(ac, av, cmdline_options), vm);
    po::notify(vm);

	// c++文件操作std::ifstream 
    std::ifstream ifs(config_file.c_str());
    if (ifs) {
        store(parse_config_file(ifs, config_file_options), vm);
        notify(vm);
    }
	// store()可调用多次，将不同的options_description存入vm
   
	//	4、从vm中获取选项值

	if (vm.count("help")) {
	// 文件写操作 内存写入存储设备  
        std::ostringstream ostr;
        ostr << cmdline_options;
        ptr->help = ostr.str();
    }
    else if (vm.count("version")) {
        ptr->version = VERSION_NUMBER;
    }
    else {
        if (vm.count("http_port")) {
            ptr->http_port = vm["http_port"].as<unsigned short>();
        }
        if (vm.count("dts_port")) {
            ptr->dts_port = vm["dts_port"].as<unsigned short>();
        }
        if (vm.count("pic_path")) {
            ptr->pic_path = vm["pic_path"].as<string>();
        }

        if (vm.count("db_server")) {
            ptr->db_server = vm["db_server"].as<string>();
        }
        if (vm.count("db_name")) {
            ptr->db_name = vm["db_name"].as<string>();
        }
        if (vm.count("db_user")) {
            ptr->db_user = vm["db_user"].as<string>();
        }
        if (vm.count("db_password")) {
            ptr->db_password = vm["db_password"].as<string>();
        }
        if (vm.count("db_idle_time")) {
            ptr->db_idle_time = vm["db_idle_time"].as<unsigned int>();
        }
        if (vm.count("server_ip")) {
            ptr->server_ip = vm["server_ip"].as<string>();
        }
        if (vm.count("server_port")) {
            ptr->server_port = vm["server_port"].as<unsigned short>();
        }
        if (vm.count("voucher_pic_path")) {
            ptr->voucher_pic_path = vm["voucher_pic_path"].as<string>();
        }
        if (vm.count("log_level")) {
            ptr->log_level = vm["log_level"].as<unsigned int>();
        }
        if (vm.count("log_max_size")) {
            ptr->log_max_size = vm["log_max_size"].as<unsigned int>();
        }
        if (vm.count("log_max_files")) {
            ptr->log_max_files = vm["log_max_files"].as<unsigned int>();
        }
    }

    return ptr;
}

string dump_options(options_pointer opts)
{
    std::ostringstream ostr;
	ostr << "dump_options:\n"
		<< "http_port=" << opts->http_port << "\n"
		<< "dts_port=" << opts->dts_port << "\n"
		<< "pic_path=" << opts->pic_path << "\n"
		<< "db_server=" << opts->db_server << "\n"
		<< "db_name=" << opts->db_name << "\n"
		<< "db_user=" << opts->db_user << "\n"
		<< "db_idle_time=" << opts->db_idle_time << "\n"
		<< "server_ip=" << opts->server_ip << "\n"
		<< "server_port=" << opts->server_port << "\n"
		<< "voucher_pic_path=" << opts->voucher_pic_path << "\n"
		<< "log_level=" << opts->log_level << "\n"
		<< "log_max_size=" << opts->log_max_size << "\n"
		<< "log_max_files=" << opts->log_max_files;

    return ostr.str();
}

}
