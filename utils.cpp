//
// Created by Christian Franzke on 2019-01-29.
//

#include "utils.h"

#include <syslog.h>
#include <iostream>
#include <vector>


void wait_for_thread_termination(std::vector<std::thread> &v) {
    syslog(LOG_NOTICE, "Waiting for thread termination.");
    for (auto &t : v)
        t.join();
}

void to_cout(const std::vector<std::string> &v) {
    std::copy(v.begin(), v.end(), std::ostream_iterator<std::string>{std::cout, "\n"});
}

void fail(boost::system::error_code ec, char const *what) {
    syslog(LOG_CRIT, "%s: %s", what, ec.message().c_str());
}

void fill_default_server_data(std::vector<ServerData> &list) {
    list.emplace_back(ServerData("https://verteiler1.mediathekview.de/"));
    list.emplace_back(ServerData("https://verteiler2.mediathekview.de/"));
    list.emplace_back(ServerData("https://verteiler4.mediathekview.de/"));
    list.emplace_back(ServerData("https://verteiler6.mediathekview.de/"));
    list.emplace_back(ServerData("https://verteiler.mediathekviewweb.de/"));
}

std::string findNewServer(std::vector<ServerData> &list, std::mutex &mutex, unsigned long &index) {
    std::lock_guard<std::mutex> lock(mutex);

    ServerData serverData = list.at(index);
    do {
        index++;
        if (index > (list.size() - 1))
            index = 0;
        serverData = list.at(index);
    } while (!serverData.active_.load());

    return std::string(serverData.url_);
}