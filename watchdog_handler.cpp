//
// Created by Christian Franzke on 2019-01-29.
//
#include <iostream>
#include <boost/bind.hpp>

#include <curl/curl.h>
#include <syslog.h>

#include "watchdog_handler.h"
#include "ServerData.h"
#include "utils.h"

void watchdog_timer_handler(const boost::system::error_code & /*e*/,
                            boost::asio::steady_timer *t, std::vector<ServerData> *serverList) {
    CURL *curl;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    for (auto &server : *serverList) {
        if (curl) {
            syslog(LOG_NOTICE,"Performing watchdog request for %s",server.url_.c_str());
            const std::string reqstr = server.url_ + "/filmliste.id";
            curl_easy_setopt(curl, CURLOPT_URL, reqstr.c_str());
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1L);
            curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // use HEAD request
            curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, ""); // auto decompress all supported

            const CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                syslog(LOG_CRIT, "watchdog failed for server %s: %s", server.url_.c_str(), curl_easy_strerror(res));
                //error condition, deactivate server
                server.active_.store(false);
            }
            else {
                //when we reach here we are active, so set server status
                server.active_.store(true);
            }
        }
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    t->expires_at(t->expiry() + boost::asio::chrono::seconds(20));
    t->async_wait(boost::bind(watchdog_timer_handler, boost::asio::placeholders::error, t, serverList));
}