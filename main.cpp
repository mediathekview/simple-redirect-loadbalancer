#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <thread>
#include <mutex>

#include <syslog.h>
#include <curl/curl.h>


#include "lambda.h"
#include "ServerData.h"
#include "watchdog_handler.h"
#include "utils.h"

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

const std::string APPLICATION_NAME{"mv_redirect_server"};
std::mutex g_serverMutex;
unsigned long g_serverIndex = 0;
std::vector<ServerData> g_serverList;
bool debug;

template<class Body, class Allocator, class Send>
void handle_request(http::request<Body, http::basic_fields<Allocator>> &&req, Send &&send) {
    // Make sure we can handle the method
    if (!(req.method() == http::verb::get) && !(req.method() == http::verb::head)) {
        log(WARNING, "Illegal request other than GET or HEAD received.");

        auto const bad_request =
                [&req](boost::beast::string_view why) {
                    http::response<http::string_body> res{http::status::bad_request, req.version()};
                    res.set(http::field::server, APPLICATION_NAME);
                    res.set(http::field::content_type, "text/html");
                    res.keep_alive(req.keep_alive());
                    res.body() = why.to_string();
                    res.prepare_payload();
                    return res;
                };

        return send(std::move(bad_request("Unknown HTTP-method")));
    }

    const std::string destination = req.target().to_string();
    const std::string server = findNewServer(std::ref(g_serverList), std::ref(g_serverMutex), std::ref(g_serverIndex));
#ifdef DEBUG
    const boost::asio::ip::address addr = send.stream_.remote_endpoint().address();
    const std::string logMessage = "Redirecting Target: " + destination + " for IP " + addr.to_string() + " to server "
                                   + server;
    syslog(LOG_NOTICE, "%s", logMessage.c_str());
#endif

    auto const redirect =
            [&req, &destination, &server]() {
                http::response<http::string_body> res{http::status::temporary_redirect, req.version()};
                res.set(http::field::server, APPLICATION_NAME);
                res.set(http::field::content_type, "text/html");
                res.set(http::field::location, server + destination);
                res.keep_alive(req.keep_alive());
                res.body() = "";
                res.prepare_payload();
                return res;
            };

    return send(std::move(redirect()));
}

void do_session(tcp::socket &socket, const boost::asio::yield_context &yield) {
    bool close = false;
    boost::system::error_code ec;
    boost::beast::flat_buffer buffer;
    // This lambda is used to send messages
    send_lambda<tcp::socket> lambda{socket, close, ec, yield};

    for (;;) {
        http::request<http::string_body> req;
        http::async_read(socket, buffer, req, yield[ec]);
        if (ec == http::error::end_of_stream)
            break;
        if (ec)
            return fail(ec, "read");

        handle_request(std::move(req), lambda);
        if (ec)
            return fail(ec, "write");
        if (close) {
            break;
        }
    }

    socket.shutdown(tcp::socket::shutdown_send, ec);
}

void do_listen(boost::asio::io_context &ioc, const tcp::endpoint &endpoint, const boost::asio::yield_context &yield) {
    boost::system::error_code ec;
    tcp::acceptor acceptor(ioc);

    acceptor.open(endpoint.protocol(), ec);
    if (ec)
        return fail(ec, "open");

    acceptor.bind(endpoint, ec);
    if (ec)
        return fail(ec, "bind");

    acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec)
        return fail(ec, "listen");

    for (;;) {
        tcp::socket socket(ioc);
        acceptor.async_accept(socket, yield[ec]);
        if (ec)
            fail(ec, "accept");
        else
            boost::asio::spawn(acceptor.get_executor().context(),
                               std::bind(&do_session, std::move(socket), std::placeholders::_1));
    }
}

int main(int argc, char *argv[]) {
    unsigned short port;
    std::string addr;
    boost::program_options::variables_map vm;

    std::cout << "MediathekView HTTP Redirect Server" << std::endl;
    std::cout << "Version 1.0" << std::endl;

    boost::program_options::options_description desc("Erlaubte Optionen");
    desc.add_options()
            ("help", "Hilfe anzeigen")
            ("port", boost::program_options::value<unsigned short>(&port)->default_value(8080), "HTTP listener port")
            ("bind-addr", boost::program_options::value<std::string>(&addr)->default_value("0.0.0.0"),
             "HTTP bind address")
            ("debug", boost::program_options::value<bool>(&debug)->default_value(false), "enable debug mode")
            ("server-url", boost::program_options::value<std::vector<std::string>>(), "Verteiler Server URL");
    //server-url kann ohne Parameter einfach angegeben werden
    boost::program_options::positional_options_description pod;
    pod.add("server-url", -1);

    boost::program_options::store(
            boost::program_options::command_line_parser(argc, argv).options(desc).positional(pod).run(), vm);
    boost::program_options::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        exit(EXIT_SUCCESS);
    }

    prepare_server_list(vm, g_serverList);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    auto const address = boost::asio::ip::make_address(addr);

    openlog(APPLICATION_NAME.c_str(), LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

    log(INFO, "HTTP Redirect Server started");

    auto const threads = std::max<int>(1, std::thread::hardware_concurrency());
    boost::asio::io_context ioc{threads};

    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&](boost::system::error_code const &, int) {
        ioc.stop();
    });

    //watchdog...
    boost::asio::steady_timer watchdog_timer(ioc, std::chrono::steady_clock::now() + std::chrono::seconds(10));
    watchdog_timer.async_wait(boost::bind(watchdog_timer_handler, boost::asio::placeholders::error, &watchdog_timer, &g_serverList));

    //http handler
    boost::asio::spawn(ioc, std::bind(&do_listen, std::ref(ioc), tcp::endpoint{address, port}, std::placeholders::_1));

    std::vector<std::thread> ioc_thread_list;
    ioc_thread_list.reserve(static_cast<unsigned long>(threads - 1));
    for (auto i = threads - 1; i > 0; --i)
        ioc_thread_list.emplace_back([&ioc] {
            ioc.run();
        });

    ioc.run();

    curl_global_cleanup();

    closelog();

    wait_for_thread_termination(ioc_thread_list);

    return EXIT_SUCCESS;
}