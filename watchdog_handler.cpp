//
// Created by Christian Franzke on 2019-01-29.
//
#include <iostream>
#include <boost/bind.hpp>

#include "watchdog_handler.h"

void watchdog_timer_handler(const boost::system::error_code & /*e*/,
                            boost::asio::steady_timer *t) {
    std::cout << "WATCHDOG TIMER called" << std::endl;

    t->expires_at(t->expiry() + boost::asio::chrono::seconds(60));
    t->async_wait(boost::bind(watchdog_timer_handler, boost::asio::placeholders::error, t));
}