//
// Created by Christian Franzke on 2019-01-31.
//

#include "ServerPool.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <random>
#include <cmath>
#include <vector>
#include "server.hpp"


void ServerPool::init(std::vector<quicktype::server> serverList) {
    for (auto &s : serverList) {
        totalWeight += s.weight;
        pool.emplace(totalWeight, s);
    }
}

quicktype::server ServerPool::getNext() {
    std::uniform_real_distribution<double> uniform_dist(0.0, totalWeight);

    return pool.lower_bound(uniform_dist(e1))->second;
}