#include "logger.h"
#include <fstream>
#include <mutex>
#include <memory>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

struct Logger::Impl
{
    std::ofstream ofs;
    std::mutex m;
};

Logger::~Logger()
{
    if (pimpl)
    {
        if (pimpl->ofs.is_open())
            pimpl->ofs.close();
        delete pimpl;
    }
}

bool Logger::init(const std::string &path)
{
    if (!pimpl)
        pimpl = new Impl();
    std::lock_guard<std::mutex> lg(pimpl->m);
    pimpl->ofs.open(path, std::ios::app);
    return pimpl->ofs.is_open();
}

static std::string iso_timestamp_now()
{
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm *gmt = std::gmtime(&t); 
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", gmt);
    return std::string(buf);
}

void Logger::log(const std::string &client, const std::string &hostport,
                 const std::string &request_line, const std::string &action,
                 int status, size_t bytes_transferred)
{
    if (!pimpl)
        return;
    std::lock_guard<std::mutex> lg(pimpl->m);
    std::ostringstream oss;
    oss << iso_timestamp_now() << " "
        << client << " \"" << request_line << "\" "
        << hostport << " " << action << " " << status << " " << bytes_transferred << "\n";
    pimpl->ofs << oss.str();
    pimpl->ofs.flush();
}
