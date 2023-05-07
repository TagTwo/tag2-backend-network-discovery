//
// Created by per on 4/10/23.
//
#include "TagTwo/Networking/GameSocket/GameSocket.h"
#include "TagTwo/Networking/Util/ErrorUtil.h"
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/co_spawn.hpp>
#include <utility>


TagTwo::Networking::GameSocket::GameSocket(int max_packet_size, bool tcp_nodelay)
        : _tcp_nodelay(tcp_nodelay)
        , _ssl_context(boost::asio::ssl::context::tls)
        , _ssl_socket(nullptr)
        , _max_packet_size(max_packet_size){}






TagTwo::Networking::GameSocket::~GameSocket() {
    stopped = true;
    _socket->close();
}


awaitable<std::size_t> TagTwo::Networking::GameSocket::_async_read(std::vector<boost::asio::mutable_buffer>& buffer) {
    if(_ssl_socket != nullptr){
        co_return co_await boost::asio::async_read(*_ssl_socket, buffer, use_awaitable);
    }
    co_return co_await boost::asio::async_read(*_socket, buffer, use_awaitable);
}

awaitable<std::size_t> TagTwo::Networking::GameSocket::_async_write(std::vector<boost::asio::const_buffer>& buffer) {
    if(_ssl_socket != nullptr){
        co_return co_await boost::asio::async_write(*_ssl_socket, buffer, use_awaitable);
    }
    co_return co_await boost::asio::async_write(*_socket, buffer, use_awaitable);
}

awaitable<void> TagTwo::Networking::GameSocket::read_data() {
    try {
        DataPacket packet;

        // Create a buffer sequence for header, info header, and body
        std::vector<boost::asio::mutable_buffer> header_buffer = {boost::asio::buffer(&packet.header, sizeof(DataHeader))};
        std::vector<boost::asio::mutable_buffer> read_buffers = {
                boost::asio::buffer(&packet.header_meta, sizeof(MetaHeader)),
                boost::asio::buffer(packet.body.data)
        };

        // Read the header
        std::size_t header_bytes_read = co_await _async_read(header_buffer);

        // Check if the magic field matches
        if (packet.header.magic != PACKET_MAGIC) {
            SPDLOG_ERROR("Invalid magic number: {}", packet.header.magic);
            co_return;
        }

        // Read the info header and body
        packet.body.data.resize(packet.header.data_size);
        std::size_t bytes_read = co_await _async_read(read_buffers);

        onData(this->shared_from_this(), packet); // SLOWS DOWN.

        SPDLOG_DEBUG("[{}] - Read {} bytes (Header: {} ({} bytes), Info header: {} ({} bytes), Data body ({} bytes))",
                     get_remote_address(),
                     header_bytes_read + bytes_read,
                     packet.header.magic, header_bytes_read,
                     packet.header.data_type, sizeof(CombinedInfoHeader),
                     bytes_read - sizeof(CombinedInfoHeader));

    } catch (const boost::system::system_error& error) {
        Util::ErrorUtil::handle_async_error(error.code());
        co_return;
    }
}



awaitable<void> TagTwo::Networking::GameSocket::send_data(const DataPacket& packet, boost::asio::const_buffer info) {
    try {
        // Create a buffer sequence for header, info header, and body
        std::vector<boost::asio::const_buffer> write_buffers = {
                boost::asio::buffer(&packet.header, sizeof(DataHeader)),
                boost::asio::buffer(&packet.header_meta, sizeof(MetaHeader)),
                boost::asio::buffer(packet.body.data)
        };

        // Calculate the total number of bytes to send
        std::size_t total_bytes_to_send = sizeof(DataHeader) + (packet.header.data_type != DataType::NONE ? sizeof(MetaHeader) : 0) + packet.body.data.size();

        // Send the data using scatter-gather I/O
        std::size_t total_bytes_sent = co_await _async_write(write_buffers);

        SPDLOG_DEBUG("[{}] - Sent {} bytes of data ({} header, {} info, {} body)", get_remote_address(), total_bytes_sent, sizeof(DataHeader), sizeof(CombinedInfoHeader), packet.body.data.size());
    } catch (const boost::system::system_error& error) {
        Util::ErrorUtil::handle_async_error(error.code());
        co_return;
    }
}


std::string TagTwo::Networking::GameSocket::get_remote_address() {
    return fmt::format("{}:{}", _socket->remote_endpoint().address().to_string(),
                       _socket->remote_endpoint().port());
}

tcp::socket &TagTwo::Networking::GameSocket::get_socket() {
    return _ssl_socket->next_layer();
}

std::shared_ptr<boost::asio::ssl::stream<tcp::socket&>> &TagTwo::Networking::GameSocket::get_ssl_socket() {
    return _ssl_socket;
}


awaitable<void> TagTwo::Networking::GameSocket::start() {
    try {
        for (;;) {
            co_await read_data();
        }
    }
    catch (std::exception &e) {
        std::printf("echo Exception: %s\n", e.what());
    }
}


void TagTwo::Networking::GameSocket::_init_socket() {

    if(_tcp_nodelay){
        // Enable TCP_NODELAY
        boost::asio::ip::tcp::no_delay option(_tcp_nodelay);
        _socket->set_option(option);
    }

    // Adjust send and receive buffer sizes
    boost::asio::socket_base::send_buffer_size send_buf_size(_max_packet_size); // 64 KB
    boost::asio::socket_base::receive_buffer_size recv_buf_size(_max_packet_size); // 64 KB
    _socket->set_option(send_buf_size);
    _socket->set_option(recv_buf_size);
}



void TagTwo::Networking::GameSocket::send_data_sync(const DataPacket&data, boost::asio::const_buffer info) {

    auto& context = static_cast<boost::asio::io_context&>(_socket->get_executor().context());
    auto promise = std::make_shared<std::promise<void>>();
    std::future<void> future = promise->get_future();

    boost::asio::co_spawn(context, [&, promise]() -> awaitable<void> {
        //co_await boost::asio::post(context, use_awaitable);
        co_await send_data(data, info);
        promise->set_value();
    }, boost::asio::detached);

    future.get();
}

void TagTwo::Networking::GameSocket::read_data_sync(){
    auto& context = static_cast<boost::asio::io_context&>(_socket->get_executor().context());
    std::future<void> send_future = boost::asio::co_spawn(context, read_data(), boost::asio::use_future);
    send_future.get();
}


void TagTwo::Networking::GameSocket::set_socket(std::shared_ptr<tcp::socket> socket){
    _socket = std::move(socket);
    _init_socket();
}

void TagTwo::Networking::GameSocket::set_ssl_socket(std::shared_ptr<boost::asio::ssl::stream<tcp::socket&>> socket){
    _ssl_socket = std::move(socket);
}

/*
boost::asio::awaitable<void> flush_receive_buffer(boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& socket) {
    boost::system::error_code ec;
    std::array<char, 4096> buffer;

    while (true) {
        std::size_t bytes_read = co_await socket.async_read_some(boost::asio::buffer(buffer), boost::asio::redirect_error(boost::asio::use_awaitable, ec));

        if (ec == boost::asio::error::would_block || ec == boost::asio::error::try_again || bytes_read == 0) {
            // No more data to read, exit the loop
            break;
        }

        if (ec) {
            // An error occurred while reading, handle it or log it if necessary
            // ...
            break;
        }
    }
}
*/