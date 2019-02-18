#pragma once

#include <nlohmann/json.hpp>
#include "helper.hpp"

#include "config.hpp"
#include "server.hpp"

namespace nlohmann {
    void from_json(const json &j, quicktype::server &x);

    void to_json(json &j, const quicktype::server &x);

    void from_json(const json &j, quicktype::config &x);

    void to_json(json &j, const quicktype::config &x);

    inline void from_json(const json &j, quicktype::server &x) {
        x.url = j.at("url").get<std::string>();
        x.weight = j.at("weight").get<double>();
    }

    inline void to_json(json &j, const quicktype::server &x) {
        j = json::object();
        j["url"] = x.url;
        j["weight"] = x.weight;
    }

    inline void from_json(const json &j, quicktype::config &x) {
        x.version = j.at("version").get<int64_t>();
        x.port = j.at("port").get<int64_t>();
        x.address = j.at("address").get<std::string>();
        x.servers = j.at("servers").get<std::vector<quicktype::server>>();
    }

    inline void to_json(json &j, const quicktype::config &x) {
        j = json::object();
        j["version"] = x.version;
        j["port"] = x.port;
        j["address"] = x.address;
        j["servers"] = x.servers;
    }
}
