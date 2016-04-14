#define BOOST_APPLICATION_FEATURE_NS_SELECT_BOOST

#include <boost/program_options.hpp>
#include <boost/application.hpp>
#include <boost/application/auto_handler.hpp>

#if defined(BOOST_WINDOWS_API)      
#   include "setup/windows/setup/service_setup.hpp"
#endif

#include "../src/log.h"

// using namespace boost;
namespace pks = parkingserver;
namespace po = boost::program_options;
namespace application = boost::application;

class daemon_app
{
  public:
    void worker(application::context* context) {

        using boost::application::example::windows_scm;

        boost::shared_ptr<application::status> st = context->find<application::status>();
        while(st->state() != application::status::stoped) {
            boost::this_thread::sleep(boost::posix_time::seconds(1));

            boost::system::error_code ec;
            windows_scm scm(SC_MANAGER_ENUMERATE_SERVICE, ec);
            if(!ec) {
                // Open this service for DELETE access
                SC_HANDLE hservice = OpenService(scm.get(), "parking-server", SERVICE_QUERY_STATUS|SERVICE_START);
                if (hservice != NULL) {
                    SERVICE_STATUS st;
                    if (QueryServiceStatus(hservice, &st)) {
                        if (st.dwCurrentState == SERVICE_STOPPED) {
                            pks::log()->warn("service stopped, restart...");
                            if (StartService(hservice, 0, NULL)) {
                                pks::log()->warn("service started.");
                            }
                        }
                        CloseServiceHandle(hservice);             
                    }
                    else {
                        pks::log()->warn("query service status failed: {}", boost::application::last_error());
                    }
                }
                else {
                    pks::log()->warn("open service failed: {}", boost::application::last_error());
                }
            }
            else {
                pks::log()->warn("scm open failed: {}", ec);
            }
        }
    }

    int operator()(application::context& context) {
        pks::log()->info("operator() in!");

        // launch a work thread
        boost::thread thread(&daemon_app::worker, this, &context);
        
        context.find<application::wait_for_termination_request>()->wait();
        pks::log()->info("operator() out!");
        return 0;
    }

    bool stop(application::context& context) {
        pks::log()->info("stop!");
        std::cout << "stop!" << std::endl;
        return true;
    }
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
    pks::InitLog(2, 10*1024*1024, 5, "parking-daemon");

    pks::log()->info("main!");
    application::context app_ctx;
    application::auto_handler<daemon_app> app(app_ctx);
    // pks_app app(app_ctx);

    app_ctx.insert<application::path>(boost::make_shared<application::path_default_behaviour>(argc, argv));
    app_ctx.insert<application::args>(boost::make_shared<application::args>(argc, argv));

    if(setup(app_ctx)) {
        std::cout << "[I] Setup changed the current configuration." << std::endl;
        return 0;
    }

    boost::system::error_code ec;
    int result = application::launch<application::server>(app, app_ctx, ec);

    if(ec) {
        std::cout << "[E] " << ec.message() << " <" << ec.value() << "> " << std::endl;
    }

    return result;
}
