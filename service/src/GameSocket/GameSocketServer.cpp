//
// Created by per on 4/10/23.
//
#include <boost/asio/detached.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/ssl/stream.hpp>
#include "TagTwo/Networking/GameSocket/GameSocketServer.h"
#include "TagTwo/Networking/Util/ErrorUtil.h"

TagTwo::Networking::GameSocketServer::GameSocketServer(bool ssl, bool ssl_dh)
        : _ssl_enabled(ssl)
        , num_threads(static_cast<int32_t>(std::thread::hardware_concurrency()))
        , _io_context(num_threads)
        , _ssl_context(boost::asio::ssl::context::tls)
        , _acceptor(_io_context)
        {
    if (ssl) {
        Util::SSLUtil::init_ssl(_ssl_context);
    }


}
void TagTwo::Networking::GameSocketServer::start(std::string host, const int port, bool blocking) {
    try {
        // Log server start information
        SPDLOG_INFO("Starting GameSocketServer on {}:{} with {} io threads.", host, port, std::thread::hardware_concurrency());

        // Set up signal handling
        //boost::asio::signal_set signals(_io_context, SIGINT, SIGTERM);
       // signals.async_wait([&](auto, auto) { stop(); });

        // Resolve the endpoint
        boost::asio::ip::tcp::resolver resolver(_io_context);
        boost::asio::ip::tcp::resolver::query query(host, std::to_string(port));
        boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
        boost::asio::ip::tcp::endpoint endpoint = iter->endpoint();

        // Start the listener
        boost::asio::co_spawn(_io_context, listen(endpoint), boost::asio::detached);

        // Create a thread pool for the worker threads
        _thread_pool.reserve(blocking ? num_threads - 1 : num_threads);

        // Run the io_context on the worker threads
        for (unsigned int i = 0; i < _thread_pool.capacity(); ++i) {
            _thread_pool.emplace_back([&, i] {
                SPDLOG_DEBUG("IOContext Thread {} started", i);
                _io_context.run();
                SPDLOG_DEBUG("IOContext Thread {} finished", i);
            });
            _thread_pool.at(i).detach();
        }

        if (blocking) {
             // Run the io_context on the main thread
            _io_context.run();

        }
    } catch (std::exception& exception) {
        // Log any errors that occur while starting the server
        SPDLOG_ERROR("Could not start ASIOServer: {}", exception.what());
    }
}



boost::asio::awaitable<void> TagTwo::Networking::GameSocketServer::listen(boost::asio::ip::tcp::endpoint endpoint) {
    try{
        auto executor = co_await this_coro::executor;

        // Open and bind the acceptor to the endpoint
        _acceptor = boost::asio::ip::tcp::acceptor(_io_context, endpoint);
        _acceptor.set_option(boost::asio::socket_base::reuse_address(true));
        _acceptor.listen();

        SPDLOG_DEBUG("Server is ready for new clients");

        for (;;)
        {


            std::shared_ptr<tcp::socket> socket = std::make_shared<tcp::socket>(_acceptor.get_executor());

            auto ssl_socket = std::make_shared<boost::asio::ssl::stream<tcp::socket&>>(*socket, _ssl_context);
            co_await _acceptor.async_accept(*socket, boost::asio::use_awaitable);



            if (_ssl_enabled) {
                // Wrap the socket in an SSL stream and perform SSL handshake
                co_await ssl_socket->async_handshake(boost::asio::ssl::stream_base::server, boost::asio::use_awaitable);

            }

            // Create new session object with SSL stream
            std::shared_ptr<GameSocket> client = std::make_shared<GameSocket>();
            client->set_socket(socket);

            if(_ssl_enabled){
                client->set_ssl_socket(ssl_socket);
            }


            // Register client in map so that object is not destroyed.
            clients.emplace(client->get_remote_address(), client);


            client->onData.connect([this](const std::shared_ptr<GameSocket>& client, DataPacket& packet){
                onData(client, packet);
            });

            // Signal to remove client when disconnected.
            client->signalOnClose.connect([this](auto c){
                clients.erase(c->get_remote_address());
            });

            // Start the client session
            boost::asio::co_spawn(executor, client->start(), boost::asio::detached);
        }
    }catch(std::error_code& exception){
        // Log any errors that occur while starting the listener
        if(!Util::ErrorUtil::handle_async_error(exception)){
            throw exception;
        }

    }

    co_return ;
}

void TagTwo::Networking::GameSocketServer::stop() {

    // Close all active connections
    for (auto& client : clients) {
        client.second->get_socket().close();
    }

    // Close the acceptor to stop accepting new connections
    _acceptor.close();


    // Stop the I/O context
    _io_context.post([this]() {
        _io_context.stop();
    });

    // Join the threads
    for (std::thread& t : _thread_pool) {
        if (t.joinable()) {
            t.join();
        }
    }

    // Join the main I/O context thread if it was detached
    if (_io_context_thread.joinable()) {
        _io_context_thread.join();
    }
}

TagTwo::Networking::GameSocketServer::~GameSocketServer() {
    stop();
}
