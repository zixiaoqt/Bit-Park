#ifdef WIN32
#include <winsock2.h>
#endif

#include <iostream>
#include <boost/filesystem.hpp>
#include "log.h"
#include "http_server.h"
#include "userterminal_server.h"
#include <json/json.h>
#include "options.h"
#include "transaction.h"
#include "singleton.h"
#include "mysqlconnectionpool.h"
#include "authentication.h"

#define BOOST_APPLICATION_FEATURE_NS_SELECT_BOOST

#include <boost/program_options.hpp>
#include <boost/application.hpp>
#include <boost/application/auto_handler.hpp>

#if defined(BOOST_WINDOWS_API)
#   include "setup/windows/setup/service_setup.hpp"
#endif

namespace pks = parkingserver;
using boost::format;
// using namespace boost;
namespace po = boost::program_options;
// using boost::application;
namespace application = boost::application;

pks::options_pointer parse_options(application::context& context) {
    boost::strict_lock<application::aspect_map> guard(context);

    auto myargs = context.find<application::args>(guard);
    auto mypath = context.find<application::path>(guard);
    return pks::parse_options(myargs->argc(), myargs->argv(), mypath->executable_path().string());
}

class pks_app
{
  public:
    // void worker(application::context* context) {

    // }

    int operator()(application::context& context) {
        // pks::log()->info("operator() in!");
        std::cout << "operator() in!" << std::endl;

        using namespace std;
        using namespace boost::filesystem;

        auto opts = parse_options(context);
        if (!opts->help.empty()) {
            std::cout << opts->help << std::endl;
            return 0;
        }

        if (!opts->version.empty()) {
            std::cout << opts->version << endl;
            return 0;
        }

        std::cout << dump_options(opts) << std::endl;

        // pks::InitLog(opts->log_level);
		//	创建过车图片文件
        path passvehicle_pic_path(opts->pic_path);
        if (!exists(passvehicle_pic_path)) {
            create_directories(passvehicle_pic_path);
        }
        if (opts->pic_path.back() != '/') {
            opts->pic_path += '/';
        }
		//	创建凭证图片文件
        path voucher_pic_path(opts->voucher_pic_path);
        if (!exists(voucher_pic_path)) {
            create_directories(voucher_pic_path);
        }
        if (opts->voucher_pic_path.back() != '/') {
            opts->voucher_pic_path += '/';
        }
		//	数据库连接池
        pks::Singleton<pks::MysqlConnectionPool>::Instance(opts->db_server,
                                                           opts->db_name,
                                                           opts->db_user,
                                                           opts->db_password,
                                                           opts->db_idle_time);
		//	Websocket服务模块  中心与客户端通信UserTerminalServer
        auto uts = pks::make_user_server(opts->server_ip,
                                         opts->server_port,
                                         opts->pic_path,
                                         opts->voucher_pic_path);
		//	Http服务模块，用于设备上传过车数据和客户端下载图片
        auto svr = pks::make_http_server(opts);

		//	TCP设备通信模块，主要负责控制设备道闸和LED显示；
        auto dts = pks::device_tcp_server::make_server(opts->dts_port);

		//	Transaction类主要负责业务调度，不管是设备的上报信息还是客户端的交互信息都会转发给这个类的对象进行处理
        pks::Transaction transaction(opts, svr, uts, dts);

        uts->run();

		//	 创建线程 启动HTTP服务器

        auto http_server_thd = boost::thread(&pks::http_server_run, svr);
        context.find<application::wait_for_termination_request>()->wait();

		//	关闭HTTP服务器
        pks::http_server_stop(svr);
		//	等待一个线程
        http_server_thd.join();

        std::cout << "operator() out!" << std::endl;
        return 0;
    }

    bool stop(application::context& context) {
        pks::log()->info("stop!");
        // pks::http_server_stop(svr);
        std::cout << "stop!" << std::endl;
        return true;
    }

    // bool pause(application::context& context) {
    //     return false;
    // }

    // bool resume(application::context& context) {
    //     return false;
    // }
  private:
    // pks::http_server_pointer http_svr_ptr;
    // boost::thread http_server_thd;
    // pks::user_server_pointer user_svr_ptr;
};

bool setup(application::context& context)
{
   boost::strict_lock<application::aspect_map> guard(context);

   auto myargs = context.find<application::args>(guard);
   auto mypath = context.find<application::path>(guard);

// provide setup for windows service
#if defined(BOOST_WINDOWS_API)

   // get our executable path name
   boost::filesystem::path executable_path_name = mypath->executable_path_name();

   // define our simple installation schema options
   po::options_description install("service options");
   install.add_options()
      ("help", "produce a help message")
      (",i", "install service")
      (",u", "unistall service")
      ("name", po::value<std::string>()->default_value(mypath->executable_name().stem().string()), "service name")
      ("display", po::value<std::string>()->default_value(""), "service display name (optional, installation only)")
      ("description", po::value<std::string>()->default_value(""), "service description (optional, installation only)")
      ;

      po::variables_map vm;
      po::store(po::parse_command_line(myargs->argc(), myargs->argv(), install), vm);
      boost::system::error_code ec;

      if (vm.count("help"))
      {
         std::cout << install << std::endl;
         return true;
      }

      if (vm.count("-i"))
      {
         application::example::install_windows_service(
         application::setup_arg(vm["name"].as<std::string>()),
         application::setup_arg(vm["display"].as<std::string>()),
         application::setup_arg(vm["description"].as<std::string>()),
         application::setup_arg(executable_path_name)).install(ec);

         std::cout << ec.message() << std::endl;

         return true;
      }

      if (vm.count("-u"))
      {
         application::example::uninstall_windows_service(
            application::setup_arg(vm["name"].as<std::string>()),
            application::setup_arg(executable_path_name)).uninstall(ec);

         std::cout << ec.message() << std::endl;

         return true;
      }

#endif

   return false;
}

int main(int argc, char* argv[])
{
    // pks::InitLog(2);

    // pks::log()->info("main!");
    application::context app_ctx;
    application::auto_handler<pks_app> app(app_ctx);
    // pks_app app(app_ctx);

    app_ctx.insert<application::path>(boost::make_shared<application::path_default_behaviour>(argc, argv));
    app_ctx.insert<application::args>(boost::make_shared<application::args>(argc, argv));

    if(setup(app_ctx)) {
        std::cout << "[I] Setup changed the current configuration." << std::endl;
        return 0;
    }

    // pks::log()->info("check auth...");
	//	解析配置文件
    auto opts = parse_options(app_ctx);
	//	初始化日志
    pks::InitLog(opts->log_level, opts->log_max_size, opts->log_max_files);

    try
    {
        pks::log()->info("run server...");

        // 加密狗
        pks::Authentication auth;
        auth.authenticate();

        boost::system::error_code ec;

#if defined(BOOST_WINDOWS_API)
	   // int result = application::launch<application::common>(app, app_ctx, ec);
       int result = application::launch<application::server>(app, app_ctx, ec);
#else
        int result = application::launch<application::common>(app, app_ctx, ec);
#endif

        pks::log()->info("exit...");

        if(ec) {
            pks::log()->critical("[E] {} <{}>", ec.message(), ec.value());
        }
        return result;
    }
    catch (std::exception& e)
    {
        pks::log()->critical(e.what());
        return 0;
    }
}
