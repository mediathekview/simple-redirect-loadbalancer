FROM alpine:latest as build

RUN apk add --no-cache curl-dev boost-dev cmake make g++

WORKDIR /tmp/simple-redirect-loadbalancer

ADD . .

RUN mkdir build
WORKDIR /tmp/simple-redirect-loadbalancer/build
RUN cmake -DCMAKE_BUILD_TYPE=Release ..
RUN make -j 4


FROM alpine:latest

RUN apk add --no-cache libcurl boost boost-program_options boost-coroutine

COPY --from=build /tmp/simple-redirect-loadbalancer/build/mv_redirect_server /usr/bin/mv_redirect_server
RUN chmod +x /usr/bin/mv_redirect_server

ENTRYPOINT /usr/bin/mv_redirect_server --help
