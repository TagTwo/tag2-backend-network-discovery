//
// Created by per on 4/10/23.
//
#include <boost/asio/co_spawn.hpp>
#include "TagTwo/Networking/GameSocket/GameSocketClientSSL.h"
#include "TagTwo/Networking/Util/SSLUtil.h"


TagTwo::Networking::GameSocketClient::GameSocketClient(bool use_ssl)
        : GameSocket(65536, true),
          _ssl_enabled(use_ssl),
          _work_guard(boost::asio::make_work_guard(_io_context)) {
    // Constructor
    if(_ssl_enabled){

        Util::SSLUtil::init_ssl(_ssl_context);
    }
}

TagTwo::Networking::GameSocketClient::~GameSocketClient() {
    // Destructor

    // Notify the io_context that it can stop when it runs out of work
    _work_guard.reset();
    _io_context.stop();
    _io_context_thread.join();
}

awaitable<void> TagTwo::Networking::GameSocketClient::connect(const std::string &host, uint16_t port) {
    // Connect to the specified host and port
    auto socket = std::make_shared<tcp::socket>(_io_context);

    tcp::resolver resolver(socket->get_executor());
    co_await socket->async_connect(*resolver.resolve(host, std::to_string(port)), use_awaitable);
    SPDLOG_INFO("Connected to {}:{}, Open={}", host, port, socket->is_open());

    auto ssl_socket = std::make_shared<boost::asio::ssl::stream<tcp::socket&>>(*socket, _ssl_context);

    if (_ssl_enabled) {
        co_await ssl_socket->async_handshake(boost::asio::ssl::stream_base::client, use_awaitable);
        set_ssl_socket(ssl_socket);
        SPDLOG_INFO("SSL handshake completed with {}:{}", host, port);
    }

    // Set the socket in the base class
    set_socket(socket);
}

void TagTwo::Networking::GameSocketClient::connect_sync(const std::string &host, uint16_t port) {
    // Connect to the host synchronously
    std::future<void> connect_future = boost::asio::co_spawn(_io_context, connect(host, port), boost::asio::use_future);

    _start_io_thread();

    connect_future.get();
}

void TagTwo::Networking::GameSocketClient::_start_io_thread() {
    // Start the IO context thread
    _io_context_thread = std::thread([this]() {
        _io_context.run();
        SPDLOG_INFO("IO context stopped");
    });
}

void TagTwo::Networking::GameSocketClient::run() {
    // Run the IO context
    _io_context.run();
}
