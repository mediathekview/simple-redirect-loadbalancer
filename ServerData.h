//
// Created by Christian Franzke on 2019-01-29.
//

#ifndef MV_REDIRECT_SERVER_SERVERDATA_H
#define MV_REDIRECT_SERVER_SERVERDATA_H

#include <iostream>

struct ServerData {
    std::string url_;
    std::atomic_bool active_{true};

    explicit ServerData(std::string url) : url_(std::move(url)) {}

    // Copy constructor
    ServerData(const ServerData &p2) {
        url_ = p2.url_;
        active_.store(p2.active_.load());
    }

    ServerData &operator=(ServerData other) {
        std::swap(url_, other.url_);
        active_.store(other.active_.load());

        return *this;
    }
};

#endif //MV_REDIRECT_SERVER_SERVERDATA_H
