FROM alpine:latest as build

RUN apk add --no-cache curl-dev boost-dev cmake make g++

WORKDIR /tmp/simple-redirect-loadbalancer

ADD src .

RUN mkdir build
WORKDIR /tmp/simple-redirect-loadbalancer/build
RUN cmake -DCMAKE_BUILD_TYPE=Release ..
RUN make -j 4
