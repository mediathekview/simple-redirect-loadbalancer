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

#include "lambda.h"
#include "ServerData.h"

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

static const std::string APPLICATION_NAME = "mv_redirect_server";
static std::mutex g_serverMutex;
static unsigned long g_indexServer = 0;
static std::vector<ServerData> g_serverList;

std::string findNewServer() {
    //FIXME da ist noch der Wurm drin
    std::lock_guard<std::mutex> lock(g_serverMutex);
    g_indexServer++;
    if (g_indexServer > (g_serverList.size() - 1))
        g_indexServer = 0;

    ServerData serverData = g_serverList.at(g_indexServer);
    while (!serverData.active_.load()) {
        g_indexServer++;
        if (g_indexServer > (g_serverList.size() - 1))
            g_indexServer = 0;
        serverData = g_serverList.at(g_indexServer);
    }

    return std::string(serverData.url_);
}

template<class Body, class Allocator, class Send>
void handle_request(http::request<Body, http::basic_fields<Allocator>> &&req, Send &&send) {
    // Make sure we can handle the method
    if (!(req.method() == http::verb::get) && !(req.method() == http::verb::head)) {
        syslog(LOG_WARNING, "Illegal request other than GET or HEAD received.");
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

    const tcp::endpoint endpoint = send.stream_.remote_endpoint();
    const boost::asio::ip::address addr = endpoint.address();
    const std::string destination = req.target().to_string();
    const std::string server = findNewServer();
    const std::string logMessage = "Redirecting Target: " + destination + " for IP " + addr.to_string() + " to server "
                                   + server;
    syslog(LOG_NOTICE, "%s", logMessage.c_str());

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

void fail(boost::system::error_code ec, char const *what) {
    syslog(LOG_CRIT, "%s: %s", what, ec.message().c_str());
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

void waitForThreadTermination(std::vector<std::thread> &v) {
    syslog(LOG_NOTICE, "Waiting for thread termination.");
    for (auto &t : v)
        t.join();
}

void watchdog_timer_handler(const boost::system::error_code & /*e*/,
                            boost::asio::steady_timer *t) {
    std::cout << "WATCHDOG TIMER called" << std::endl;

    t->expires_at(t->expiry() + boost::asio::chrono::seconds(60));
    t->async_wait(boost::bind(watchdog_timer_handler, boost::asio::placeholders::error, t));
}

void to_cout(const std::vector<std::string> &v) {
    std::copy(v.begin(), v.end(), std::ostream_iterator<std::string>{std::cout, "\n"});
}

int main(int argc, char *argv[]) {
    unsigned short port;
    std::string addr;

    std::cout << "MediathekView HTTP Redirect Server" << std::endl;
    std::cout << "Version 0.6" << std::endl;

    boost::program_options::options_description desc("Erlaubte Optionen");
    desc.add_options()
            ("help", "Hilfe anzeigen")
            ("port", boost::program_options::value<unsigned short>(&port)->default_value(8080), "HTTP listener port")
            ("bind-addr", boost::program_options::value<std::string>(&addr)->default_value("0.0.0.0"),
             "HTTP bind address")
            ("server-url", boost::program_options::value<std::vector<std::string>>(), "Verteiler Server URL");
    //server-url kann ohne Parameter einfach angegeben werden
    boost::program_options::positional_options_description p;
    p.add("server-url", -1);

    boost::program_options::variables_map vm;
    boost::program_options::store(
            boost::program_options::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
    boost::program_options::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        exit(EXIT_SUCCESS);
    }

    if (vm.count("server-url")) {
        std::cout << "Server URLs are: " << std::endl;
        to_cout(vm["server-url"].as<std::vector<std::string>>());
        syslog(LOG_NOTICE, "Using supplied URLs for redirection");

        for (auto const &item : vm["server-url"].as<std::vector<std::string>>())
            g_serverList.emplace_back(ServerData(item));
    } else {
        syslog(LOG_WARNING, "Empty server URLs, using internal default list");
        g_serverList.emplace_back(ServerData("https://verteiler1.mediathekview.de/"));
        g_serverList.emplace_back(ServerData("https://verteiler2.mediathekview.de/"));
        g_serverList.emplace_back(ServerData("https://verteiler4.mediathekview.de/"));
        g_serverList.emplace_back(ServerData("https://verteiler6.mediathekview.de/"));
        g_serverList.emplace_back(ServerData("https://verteiler.mediathekviewweb.de/"));
    }

    std::cout << "Verteiler server list size: " << g_serverList.size() << std::endl;


    auto const address = boost::asio::ip::make_address(addr);

    openlog(APPLICATION_NAME.c_str(), LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    syslog(LOG_NOTICE, "HTTP Redirect Server started");

    auto const threads = std::max<int>(1, std::thread::hardware_concurrency());
    boost::asio::io_context ioc{threads};
    syslog(LOG_INFO, "Using number of threads: %d", threads);

    syslog(LOG_NOTICE, "Listening for HTTP GET requests on port %d", port);

    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&](boost::system::error_code const &, int) {
        ioc.stop();
    });

    //watchdog...
    boost::asio::steady_timer watchdog_timer(ioc, std::chrono::steady_clock::now() + std::chrono::seconds(10));
    watchdog_timer.async_wait(boost::bind(watchdog_timer_handler, boost::asio::placeholders::error, &watchdog_timer));

    //http handler
    boost::asio::spawn(ioc, std::bind(&do_listen, std::ref(ioc), tcp::endpoint{address, port}, std::placeholders::_1));

    std::vector<std::thread> v;
    v.reserve(static_cast<unsigned long>(threads - 1));
    for (auto i = threads - 1; i > 0; --i)
        v.emplace_back([&ioc] {
            ioc.run();
        });

    ioc.run();

    closelog();

    waitForThreadTermination(v);

    syslog(LOG_NOTICE, "Program exited normally");

    return EXIT_SUCCESS;
}