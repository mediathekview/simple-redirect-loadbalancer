ARG BUILD_IMAGE

FROM ${BUILD_IMAGE} as build

FROM ubuntu:18.10

RUN apt-get update
RUN apt-get install --no-install-recommends -y \
  libcurl4 \
  libboost-system1.67.0 \
  libboost-coroutine1.67.0 \
  libboost-program-options1.67.0

COPY --from=build /tmp/simple-redirect-loadbalancer/build/mv_redirect_server /usr/bin/mv_redirect_server
RUN chmod +x /usr/bin/mv_redirect_server

ENTRYPOINT ["/usr/bin/mv_redirect_server"]
CMD ["--help"]