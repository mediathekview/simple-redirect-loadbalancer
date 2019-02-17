//
// Created by Christian Franzke on 2019-01-29.
//

#include "utils.h"

#include <syslog.h>
#include <iostream>
#include <vector>
#include <boost/format.hpp>

extern bool debug; //debug flag from main

/**
 * Thread pool used for keeping syslog off the main exec thread.
 */
static boost::asio::thread_pool syslog_pool(std::thread::hardware_concurrency());

void log(log_level level, std::string msg) {
    if (!debug)
        return;

    boost::asio::post(syslog_pool, [level, msg]() {
        syslog(level, "%s", msg.c_str());
    });
}

void fail(boost::system::error_code ec, char const *what) {
    boost::format formatter("%1: %2");
    formatter % what % ec.message();
    log(ERROR, formatter.str());
}