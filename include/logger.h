#ifndef LOGGER_H
#define LOGGER_H

#include <string>

class Logger
{
public:
    Logger() = default;
    ~Logger();

   
    bool init(const std::string &path);

   
    void log(const std::string &client, const std::string &hostport,
             const std::string &request_line, const std::string &action,
             int status, size_t bytes_transferred);

private:
    struct Impl;
    Impl *pimpl = nullptr;
};

#endif 
