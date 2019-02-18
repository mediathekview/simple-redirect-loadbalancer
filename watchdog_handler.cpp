//
// Created by Christian Franzke on 2019-01-29.
//
#include <iostream>
#include <boost/bind.hpp>

#include <curl/curl.h>
#include <syslog.h>

#include "watchdog_handler.h"
#include "utils.h"

/**
 * Thread pool for all watchdog related activities
 */
static boost::asio::thread_pool watchdog_pool(std::thread::hardware_concurrency());

void check_server(quicktype::server &server_) {
    CURL *curl = curl_easy_init();

    if (curl) {
        log(INFO, "Performing watchdog request for " + server_.url);
        const std::string reqstr = server_.url + "/filmliste.id";
        curl_easy_setopt(curl, CURLOPT_URL, reqstr.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1L);
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // use HEAD request
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, ""); // auto decompress all supported

        const CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            syslog(LOG_CRIT, "watchdog failed for server %s: %s", server_.url.c_str(), curl_easy_strerror(res));
            //error condition, deactivate server
            //server_.active_.store(false);
        } else {
            //when we reach here we are active, so set server status
            //server_.active_.store(true);
        }
    }

    curl_easy_cleanup(curl);
}

void watchdog_timer_handler(const boost::system::error_code & /*e*/,
                            boost::asio::steady_timer *t, std::vector<quicktype::server> *serverList) {

    for (auto &server : *serverList) {
        boost::asio::post(watchdog_pool, [&server]() {
            check_server(server);
        });
    }
    //wait for all watchdog threads to complete
    watchdog_pool.join();

    t->expires_at(t->expiry() + boost::asio::chrono::seconds(60));
    t->async_wait(boost::bind(watchdog_timer_handler, boost::asio::placeholders::error, t, serverList));
}