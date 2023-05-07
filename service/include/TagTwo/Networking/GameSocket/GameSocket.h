//
// Created by per on 4/10/23.
//

#ifndef TAGTWO_NETWORK_DISCOVERY_GAMESOCKET_H
#define TAGTWO_NETWORK_DISCOVERY_GAMESOCKET_H

#include <boost/signals2/signal.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_awaitable.hpp>
#include "GameSocketProtocol.h"


#include <spdlog/spdlog.h>
#include <future>
#include <boost/asio/use_future.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/read.hpp>


using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::use_awaitable;
namespace this_coro = boost::asio::this_coro;

namespace TagTwo::Networking {



    class GameSocket : public std::enable_shared_from_this<GameSocket> {
        const uint32_t _max_packet_size;
        bool _tcp_nodelay;
        bool stopped = false;

    protected:
        std::shared_ptr<boost::asio::ssl::stream<tcp::socket&>> _ssl_socket;
        std::shared_ptr<tcp::socket> _socket;

        boost::asio::ssl::context _ssl_context;
    public:
        boost::signals2::signal<void(std::shared_ptr<GameSocket>)> signalOnClose;
        boost::signals2::signal<void(const std::shared_ptr<GameSocket>& client, DataPacket&)> onData;

        GameSocket(int max_packet_size = 65536, bool tcp_nodelay = true);

        ~GameSocket();

        std::string get_remote_address();

        void _init_socket();

        tcp::socket& get_socket();
        std::shared_ptr<boost::asio::ssl::stream<tcp::socket&>>& get_ssl_socket();
        awaitable<void> start();
        awaitable<void> send_data(const DataPacket& data, boost::asio::const_buffer info = boost::asio::const_buffer());

        awaitable<void> read_data();

        void send_data_sync(const DataPacket& data, boost::asio::const_buffer info = boost::asio::const_buffer());

        void read_data_sync();

        void set_socket(std::shared_ptr<tcp::socket> socket);

        void set_ssl_socket(std::shared_ptr<boost::asio::ssl::stream<tcp::socket&>> socket);

        awaitable<size_t> _async_write(std::vector<boost::asio::const_buffer>& buffer);

        awaitable<size_t> _async_read(std::vector<boost::asio::mutable_buffer>& buffer);
    };
}
#endif //TAGTWO_NETWORK_DISCOVERY_GAMESOCKET_H
