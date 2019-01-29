//
// Created by Christian Franzke on 2019-01-29.
//

#ifndef MV_REDIRECT_SERVER_WATCHDOG_HANDLER_H
#define MV_REDIRECT_SERVER_WATCHDOG_HANDLER_H
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio.hpp>

void watchdog_timer_handler(const boost::system::error_code & /*e*/, boost::asio::steady_timer *t);
#endif //MV_REDIRECT_SERVER_WATCHDOG_HANDLER_H
