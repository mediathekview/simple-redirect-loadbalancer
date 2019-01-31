//
// Created by Christian Franzke on 2019-01-29.
//

#include "utils.h"

#include <syslog.h>
#include <iostream>
#include <vector>

extern bool debug; //debug flag from main

void log(log_level level, std::string msg) {
#ifdef DEBUG
    if (!debug)
        return;

    syslog(level,"%s", msg.c_str());
#endif
}

void wait_for_thread_termination(std::vector<std::thread> &v) {
    log(INFO, "Waiting for thread termination");

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

void prepare_server_list(boost::program_options::variables_map &vm, std::vector<ServerData> &g_serverList) {
    if (vm.count("server-url")) {
        std::cout << "Server URLs are: " << std::endl;
        to_cout(vm["server-url"].as<std::vector<std::string>>());
        log(INFO, "Using supplied URLs for redirection");

        for (auto const &item : vm["server-url"].as<std::vector<std::string>>())
            g_serverList.emplace_back(ServerData(item));
    } else {
        log(WARNING, "Empty server URLs, using internal default list");

        fill_default_server_data(g_serverList);
    }
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