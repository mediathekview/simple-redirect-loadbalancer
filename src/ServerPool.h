//
// Created by Christian Franzke on 2019-01-31.
//

#ifndef MV_REDIRECT_SERVER_SERVERPOOL_H
#define MV_REDIRECT_SERVER_SERVERPOOL_H

#include <map>
#include <random>
#include "server.hpp"

class ServerPool {
public:
    ServerPool() : totalWeight{0.0}, e1(r()) {
    };

    void init(std::vector<quicktype::server> serverList);

    quicktype::server getNext();

private:
    std::map<double, quicktype::server> pool;
    double totalWeight;
    std::random_device r;
    std::default_random_engine e1;
};


#endif //MV_REDIRECT_SERVER_SERVERPOOL_H
