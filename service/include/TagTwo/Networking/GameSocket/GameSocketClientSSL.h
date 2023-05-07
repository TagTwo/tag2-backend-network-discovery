//
// Created by per on 4/10/23.
//

#ifndef TAGTWO_NETWORK_DISCOVERY_GAMESOCKETCLIENT_H
#define TAGTWO_NETWORK_DISCOVERY_GAMESOCKETCLIENT_H

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <thread>
#include "GameSocket.h"

namespace TagTwo::Networking {

    class GameSocketClient : public GameSocket {
    std::thread _io_context_thread;
    boost::asio::io_context _io_context;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> _work_guard;
    bool _ssl_enabled;
    public:
        explicit GameSocketClient(bool use_ssl);

        ~GameSocketClient();

        awaitable<void> connect(const std::string &host, uint16_t port);

        void connect_sync(const std::string &host, uint16_t port);



        void _start_io_thread();

        void run();


    };


}
#endif //TAGTWO_NETWORK_DISCOVERY_GAMESOCKETCLIENT_H
