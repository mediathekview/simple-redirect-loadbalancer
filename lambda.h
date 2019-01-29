//
// Created by Christian Franzke on 2019-01-28.
//

#ifndef MV_REDIRECT_SERVER_LAMBDA_H
#define MV_REDIRECT_SERVER_LAMBDA_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

template<class Stream>
struct send_lambda {
    Stream &stream_;
    bool &close_;
    boost::system::error_code &ec_;
    boost::asio::yield_context yield_;

    explicit send_lambda(
            Stream &stream,
            bool &close,
            boost::system::error_code &ec,
            boost::asio::yield_context yield)
            : stream_(stream), close_(close), ec_(ec), yield_(yield) {
    }

    template<bool isRequest, class Body, class Fields>
    void operator()(boost::beast::http::message<isRequest, Body, Fields> &&msg) const {
        // Determine if we should close the connection after
        close_ = msg.need_eof();

        // We need the serializer here because the serializer requires
        // a non-const file_body, and the message oriented version of
        // http::write only works with const messages.
        boost::beast::http::serializer<isRequest, Body, Fields> sr{msg};
        boost::beast::http::async_write(stream_, sr, yield_[ec_]);
    }
};

#endif //MV_REDIRECT_SERVER_LAMBDA_H
