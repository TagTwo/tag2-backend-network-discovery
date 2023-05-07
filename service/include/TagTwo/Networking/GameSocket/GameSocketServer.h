//
// Created by per on 2/25/23.
//

#ifndef TAG2_NETWORKSERVER_H
#define TAG2_NETWORKSERVER_H

#include <boost/asio/ssl/context.hpp>
#include <thread>

#include "GameSocket.h"
#include "GameSocketProtocol.h"
#include "TagTwo/Networking/Util/SSLUtil.h"



namespace this_coro = boost::asio::this_coro;

namespace TagTwo::Networking {


    class GameSocketServer: public std::enable_shared_from_this<GameSocketServer>{

        typedef GameSocket T;

        const bool _ssl_enabled;
        const int num_threads;
        boost::asio::io_context _io_context;
        std::thread _io_context_thread;
        boost::asio::ssl::context _ssl_context;
        boost::asio::ip::tcp::acceptor _acceptor;

        std::vector<std::thread> _thread_pool;

    public:
        boost::signals2::signal<std::shared_ptr<T> ()> eventCreateSession;
        boost::signals2::signal<void (std::shared_ptr<T>)> eventAuthenticationComplete;
        boost::signals2::signal<void (std::shared_ptr<T>)> eventAuthenticationFailed;

        boost::signals2::signal<void (const std::shared_ptr<GameSocket>& client, DataPacket&)> onData;


        // A map to store active clients
        std::unordered_map<std::string, std::shared_ptr<GameSocket>> clients;


        // Constructor
        explicit GameSocketServer(bool ssl, bool ssl_dh=true);
        ~GameSocketServer();

        void start(std::string host, int port, bool threaded = true);
        void stop();
        boost::asio::awaitable<void> listen(boost::asio::ip::tcp::endpoint endpoint);


    };



}







#endif //TAG2_NETWORKSERVER_H
