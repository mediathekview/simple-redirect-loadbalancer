//
// Created by Christian Franzke on 2019-01-29.
//

#ifndef MV_REDIRECT_SERVER_UTILS_H
#define MV_REDIRECT_SERVER_UTILS_H
#include <thread>
#include <boost/asio.hpp>
#include <syslog.h>

#include "ServerData.h"

void fail(boost::system::error_code ec, char const *what);

typedef enum {
    INFO = LOG_NOTICE,
    WARNING = LOG_WARNING,
    ERROR = LOG_ERR
} log_level;

void log(log_level level, std::string msg);
#endif //MV_REDIRECT_SERVER_UTILS_H
