ARG BUILD_IMAGE

FROM ${BUILD_IMAGE} as build

FROM alpine:latest

RUN apk add --no-cache libcurl boost boost-program_options boost-coroutine

COPY --from=build /tmp/simple-redirect-loadbalancer/build/mv_redirect_server /usr/bin/mv_redirect_server
RUN chmod +x /usr/bin/mv_redirect_server

ENTRYPOINT ["/usr/bin/mv_redirect_server"]
CMD ["--help"]