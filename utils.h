//
// Created by Christian Franzke on 2019-01-29.
//

#ifndef MV_REDIRECT_SERVER_UTILS_H
#define MV_REDIRECT_SERVER_UTILS_H
#include <thread>
#include <boost/asio.hpp>
#include "ServerData.h"

void wait_for_thread_termination(std::vector<std::thread> &v);
void to_cout(const std::vector<std::string> &v);
void fail(boost::system::error_code ec, char const *what);
void fill_default_server_data(std::vector<ServerData> &list);
#endif //MV_REDIRECT_SERVER_UTILS_H
