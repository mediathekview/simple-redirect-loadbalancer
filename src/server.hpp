#pragma once

#include <nlohmann/json.hpp>
#include "helper.hpp"

namespace quicktype {
    using nlohmann::json;

    struct server {
        std::string url;
        double weight;
    };
}
