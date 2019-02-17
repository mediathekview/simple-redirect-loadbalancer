#pragma once

#include <nlohmann/json.hpp>
#include "helper.hpp"

namespace quicktype {
    struct server;
}

namespace quicktype {
    using nlohmann::json;

    struct config {
        u_int16_t version;
        u_int16_t port;
        std::string address;
        std::vector<server> servers;
    };
}
