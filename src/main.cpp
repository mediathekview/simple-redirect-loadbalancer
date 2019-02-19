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

#include <curl/curl.h>
#include <fstream>

#include "lambda.h"
#include "watchdog_handler.h"
#include "utils.h"
#include "config.hpp"
#include "Generators.hpp"
#include "ServerPool.h"

#include <nlohmann/json.hpp>


using tcp = boost::asio::ip::tcp;
namespace po = boost::program_options;
namespace http = boost::beast::http;

#define APPLICATION_NAME "mv_redirect_server"
#define CONTENT_TYPE "text/html"

ServerPool serverPool;

template<class Body, class Allocator, class Send>
void handle_request(http::request<Body, http::basic_fields<Allocator>> &&req, Send &&send) {
    // Make sure we can handle the method
    if (!(req.method() == http::verb::get) && !(req.method() == http::verb::head)) {
        log(WARNING, "Illegal request other than GET or HEAD received.");

        auto const bad_request =
                [&req](boost::beast::string_view why) {
                    http::response<http::string_body> res{http::status::bad_request, req.version()};
                    res.set(http::field::server, APPLICATION_NAME);
                    res.set(http::field::content_type, CONTENT_TYPE);
                    res.keep_alive(req.keep_alive());
                    res.body() = why.to_string();
                    res.prepare_payload();
                    return res;
                };

        return send(std::move(bad_request("Unknown HTTP-method")));
    }

    const std::string destination = req.target().to_string();
    const std::string server = serverPool.getNext().url;
    const boost::asio::ip::address addr = send.stream_.remote_endpoint().address();
    const std::string logMessage = "Redirecting Target: " + destination + " for IP " + addr.to_string() + " to server "
                                   + server;
    log(INFO, logMessage);

    auto const redirect =
            [&req, &destination, &server]() {
                http::response<http::string_body> res{http::status::temporary_redirect, req.version()};
                res.set(http::field::server, APPLICATION_NAME);
                res.set(http::field::content_type, CONTENT_TYPE);
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

quicktype::config parse_config_file(std::string &file_name) {
    quicktype::config data;
    try {
        //working sample config
/*        data = R"(
                                     {\"version\": 1,\n"
                                     "  \"port\": 8080,\n"
                                     "  \"address\":\"0.0.0.0\",\n"
                                     "  \"servers\": ["
                                     "      {\"url\": \"https://verteiler1.mediathekview.de/\", \"weight\": 10.0},\n"
                                     "      {\"url\": \"https://verteiler2.mediathekview.de/\", \"weight\": 10.0},\n"
                                     "      {\"url\": \"https://verteiler4.mediathekview.de/\", \"weight\": 10.0},\n"
                                     "      {\"url\": \"https://verteiler6.mediathekview.de/\", \"weight\": 10.0},\n"
                                     "      {\"url\": \"https://verteiler.mediathekviewweb.de/\", \"weight\": 60.0}\n"
                                     "  ]"
                                     "}
        )"_json;*/

        std::ifstream i(file_name);
        nlohmann::json j;
        i >> j;
        data = j;

        if (data.version != 1) {
            std::cerr << "Invalid config file format" << std::endl;
            ::exit(EXIT_FAILURE);
        }

#ifndef NDEBUG
        std::cout << "num servers: " << data.servers.size() << std::endl;
        std::cout << "port: " << data.port << std::endl;
        std::cout << "address: " << data.address << std::endl;
#endif
    }
    catch (nlohmann::json::parse_error &e) {
        std::cerr << "EXCEPTION PARSING CONFIG FILE: " << e.what() << std::endl;
        ::exit(EXIT_FAILURE);
    }

    return data;
}

void wait_for_thread_termination(std::vector<std::thread> &v) {
    log(INFO, "Waiting for thread termination");

    for (auto &t : v) {
        t.join();
    }
}

std::string parse_command_line(int argc, char *argv[],po::variables_map& vm) {
    std::string config_file_name;

    try {
        po::options_description desc("Erlaubte Optionen");
        desc.add_options()
                ("help", "Hilfe anzeigen")
                ("config-file", po::value<std::string>(&config_file_name)->required());

        po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            exit(EXIT_SUCCESS);
        }

        //notify here to prevent program crash when options are missing...
        po::notify(vm);
    }
    catch (po::required_option &e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    return config_file_name;
}

int main(int argc, char *argv[]) {
    po::variables_map vm;
    std::string config_file_name;
    quicktype::config data;

    std::cout << "MediathekView HTTP Redirect Server" << std::endl;
    std::cout << "Version 1.3" << std::endl;

    config_file_name = parse_command_line(argc,argv,vm);

    //here all options are safe to use
    data = parse_config_file(config_file_name);
    //fill server data
    serverPool.init(data.servers);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    openlog(APPLICATION_NAME, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

    log(INFO, "HTTP Redirect Server started");

    auto const threads = std::max<int>(1, std::thread::hardware_concurrency());
    boost::asio::io_context ioc{threads};
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
    boost::asio::steady_timer watchdog_timer(ioc, std::chrono::steady_clock::now() + std::chrono::seconds(10));

    //signal handler
    signals.async_wait([&](boost::system::error_code const &, int) {
        ioc.stop();
    });

    //watchdog...
    watchdog_timer.async_wait(
            boost::bind(watchdog_timer_handler, boost::asio::placeholders::error, &watchdog_timer, &data.servers));

    //http handler
    boost::asio::spawn(ioc, std::bind(&do_listen, std::ref(ioc),
                                      tcp::endpoint{boost::asio::ip::make_address(data.address), data.port},
                                      std::placeholders::_1));

    std::vector<std::thread> ioc_thread_list;
    ioc_thread_list.reserve(static_cast<unsigned long>(threads - 1));
    for (auto i = threads - 1; i > 0; --i)
        ioc_thread_list.emplace_back([&ioc] {
            ioc.run();
        });

    /*
     * RUN LOOP
    */
    ioc.run();

    curl_global_cleanup();

    closelog();

    wait_for_thread_termination(ioc_thread_list);

    log(INFO, "Successful termination");

    return EXIT_SUCCESS;
}