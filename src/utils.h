//
// Created by Christian Franzke on 2019-01-29.
//

#ifndef MV_REDIRECT_SERVER_UTILS_H
#define MV_REDIRECT_SERVER_UTILS_H
#include <thread>
#include <boost/asio.hpp>
#include <syslog.h>

void fail(boost::system::error_code ec, char const *what);

typedef enum {
    INFO = LOG_NOTICE,
    WARNING = LOG_WARNING,
    ERROR = LOG_ERR
} log_level;

void log(log_level level, std::string msg);
bool destination_is_invalid(const std::string& destination);
bool ip_is_blocked ( const boost::asio::ip::address& addr );
#endif //MV_REDIRECT_SERVER_UTILS_H
