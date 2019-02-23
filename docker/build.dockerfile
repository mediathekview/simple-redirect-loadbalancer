FROM ubuntu:18.10 as build

RUN apt-get update
RUN apt-get install --no-install-recommends -y \
                      libcurl4-openssl-dev \
                      cmake \
                      make \
                      g++ \
                      libboost-system-dev \
                      libboost-coroutine-dev \
                      libboost-program-options-dev \
                      nlohmann-json3-dev

WORKDIR /tmp/simple-redirect-loadbalancer

ADD src .

RUN mkdir build
WORKDIR /tmp/simple-redirect-loadbalancer/build
RUN cmake -DCMAKE_BUILD_TYPE=Release ..
RUN make -j 4
