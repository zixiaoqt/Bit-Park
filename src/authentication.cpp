#include "authentication.h"
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <chrono>
#include <future>
#include <fstream>
#include "elecrypt.h"
#include "elitee.h"
#include "log.h"
#include "scopeguard.h"
#include "util.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;

namespace parkingserver {
static UCHAR deskey1[16] = { 0x46, 0x5a, 0x8c, 0x91, 0xfc, 0xe2, 0x6c, 0x9a,
                             0x3a, 0x98, 0x93, 0x8e, 0x1d, 0xaa, 0x97, 0x1e };
static UCHAR deskey2[16] = { 0x57, 0x4a, 0x80, 0xe1, 0xfc, 0xa2, 0x6c, 0x9a,
                             0x3a, 0x98, 0x93, 0x4e, 0x1d, 0xaa, 0x97, 0x2e };

static const int trial_time{ 604800 };
static const int detect_interval{ 30 };
static const std::string company{ "BITCOM" };
static const std::string module_name{ "dongle" };

Authentication::Authentication()
        : work_(ios_)
        , timer_(make_shared<boost::asio::deadline_timer>(ios_)){
    thread_ = std::thread(boost::bind(&boost::asio::io_service::run, &ios_));
}

Authentication::~Authentication() {
    ios_.stop();
    thread_.join();
}

void Authentication::authenticate() {
#if defined WIN32 || defined _WIN32 || defined _WIN64
    auto temp_path = temp_directory_path();
    temp_path += "/505C8F6665368D398F6F4EF6";
    if (exists(temp_path)) {
        ifstream ifs;
        auto file_gud = makeGuard([&](){
                if (ifs) {
                    ifs.close();
                }
            });
        ifs.open(temp_path.c_str(), ios::binary);
        if (ifs) {
            UCHAR text[64] = { 0 };
            UCHAR ciphertext[64] = { 0 };
            ifs.read(reinterpret_cast<char*>(ciphertext), 64);
            auto ret = aes_ecb(deskey1, ciphertext, text, 64, AES_CRYPT_DEC);
            if (ret != SES_SUCCESS) {
                auto errinfo = str(format("Decrypt failed %1%") % ret);
                throw runtime_error(errinfo);
            }
            auto start_t = atoi(reinterpret_cast<char*>(text));
            log()->info("The first installed time: {}",
                        util::FormatLocalTime(start_t));
            auto current_t = util::GetCurrentTime();
            if (current_t - start_t >= trial_time) {
                log()->info("The trial time expires");
                detect_dongle();
                // detect dongle on timer
                timer_->expires_from_now(boost::posix_time::seconds(detect_interval));
                timer_->async_wait(boost::bind(&Authentication::handle_timer, this,
                                               boost::asio::placeholders::error, "It is time to detect dongle"));
            }
            else {
                // start a expiring timer
                auto interval = trial_time - current_t + start_t;
                log()->info("The trial time expires after {} seconds", interval);
                timer_->expires_from_now(boost::posix_time::seconds(interval));
                timer_->async_wait(boost::bind(&Authentication::handle_timer, this,
                                               boost::asio::placeholders::error, "The trial time expires"));
            }
        }
        else {
            throw runtime_error("Read failed");
        }
    }
    else {
        log()->info("First start the program");
        ofstream ofs;
        auto file_gud = makeGuard([&](){
                if (ofs) {
                    ofs.close();
                }
            });
        ofs.open(temp_path.c_str(), ios::binary);
        if (ofs) {
            auto current_time = std::chrono::system_clock::now();
            auto current_t = std::chrono::system_clock::to_time_t(current_time);
            UCHAR ciphertext[64] = { 0 };
            UCHAR text[64] = { 0 };
            auto strtime = std::to_string(current_t);
            memcpy(text, strtime.c_str(), strtime.length());
            auto ret = aes_ecb(deskey1, text, ciphertext, 64, AES_CRYPT_ENC);
            if (ret != SES_SUCCESS) {
                remove(temp_path.c_str());
                auto errinfo = str(format("Encrypt failed %1%") % ret);
                throw runtime_error(errinfo);
            }
            ofs.write(reinterpret_cast<char*>(ciphertext), 64);
        }
        else {
            remove(temp_path.c_str());
            throw runtime_error("Write failed");
        }
        // start timer about trial time
        timer_->expires_from_now(boost::posix_time::seconds(trial_time));
        timer_->async_wait(boost::bind(&Authentication::handle_timer, this,
                                       boost::asio::placeholders::error, "The trial time expires"));
    }
#endif
}

void Authentication::detect_dongle() {
#if defined WIN32 || defined _WIN32 || defined _WIN64
    log()->debug("Detecting dongle");
    //open first device
    ELE_DEVICE_CONTEXT ele_ctx = { 0 };
    ele_ctx.ulSize = sizeof(ELE_DEVICE_CONTEXT);
    UCHAR puc_desp[32] = { 0 };
    memcpy(puc_desp, company.c_str(), company.length());
    auto rt = EleOpenFirstDevice(nullptr, puc_desp, nullptr, ELE_SHARE_MODE, &ele_ctx);
    if (!rt) {
        if (EleGetLastError() == ELE_NO_MORE_DEVICE) {
            throw runtime_error("Not plug in dongle");
        }
        else {
            auto errinfo = str(format("Open dongle failed %1%") % EleGetLastError());
            throw runtime_error(errinfo);
        }
    }
    auto file_gud = makeGuard([&](){
            EleClose(&ele_ctx);
        });
    srand((unsigned)time(0));
    UCHAR8_T input[32] = { 0 };
    for (auto i = 0; i < 32; ++i) {
        input[i] = rand() % 32 ;
    }
    UCHAR8_T cipher_input[32] = { 0 };
    auto ret = aes_ecb(deskey2, input, cipher_input, 32, AES_CRYPT_ENC);
    if (ret != SES_SUCCESS) {
        auto errinfo = str(format("Encrypt input failed %1%") % ret);
        throw runtime_error(errinfo);
    }
    UCHAR8_T cipher_output[32] = { 0 };
    ULONG32_T real_len = 0;
    char mod_name[32] = { 0 };
    memcpy(mod_name, module_name.c_str(), module_name.length());
    rt = EleExecute(&ele_ctx, mod_name, cipher_input, 32, cipher_output, 32, &real_len);
    if (!rt) {
        auto errinfo = str(format("Execute failed %1%") % EleGetLastError());
        throw runtime_error(errinfo);
    }
    UCHAR output[32] = { 0 };
    ret = aes_ecb(deskey2, cipher_output, output, 32, AES_CRYPT_DEC);
    if (ret != SES_SUCCESS) {
        auto errinfo = str(format("Decrypt failed %1%") % ret);
        throw runtime_error(errinfo);
    }
    for (auto i = 0; i < 32; ++i) {
        if (output[i] != input[i] + i) {
            throw runtime_error("Verify content error");
        }
    }
#endif
}

void Authentication::handle_timer(const boost::system::error_code& e,
                                  const std::string& loginfo) {
    try
    {
        log()->info(loginfo);
        detect_dongle();
        timer_->expires_from_now(boost::posix_time::seconds(detect_interval));
        timer_->async_wait(boost::bind(&Authentication::handle_timer, this,
                                       boost::asio::placeholders::error, "It is time to detect dongle"));
    }
    catch (std::exception& e)
    {
        log()->critical(e.what());
        exit(0);
    }
}

}
