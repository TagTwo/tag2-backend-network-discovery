//
// Created by per on 4/3/23.
//
/**
@file NNGClient.h
@brief Provides an interface for working with NNG sockets in C++.
*/

#ifndef TAGTWO_STREAMING_NNGCLIENT_H
#define TAGTWO_STREAMING_NNGCLIENT_H
#include <nngpp/nngpp.h>
#include <nngpp/protocol/req0.h>
#include <nngpp/protocol/push0.h>
#include <nngpp/protocol/pub0.h>
#include <string>
#include <utility>
#include <spdlog/spdlog.h>

// Including the standard chrono library to work with timestamps
using namespace std::chrono;

// Declare a new namespace TagTwo::Networking to avoid naming conflicts
namespace TagTwo::Networking{

    using milliseconds = std::chrono::milliseconds;
    using system_clock = std::chrono::system_clock;
    using std::chrono::duration_cast;

    /**
    @enum NNGClientType
    @brief Enum class to define the different client types for the NNG library.
    */
    enum NNGClientType{
        PUSH, // Client will only send data
        REQ,  // Client will send requests and receive replies
        PUB   // Client will publish data to subscribers
    };

    /**
    @class NNGClient
    @brief NNGClient class provides an interface for working with NNG sockets.
    */
    class NNGClient {

        NNGClientType type;  // Client type (PUSH, REQ, or PUB)
        bool _connected = false; // Connection status
        const std::string host;  // Host address
        const int port;  // Host port number
        const milliseconds reconnect_interval;  // Reconnection interval (in milliseconds)
        std::chrono::system_clock clock;  // System clock to manage timestamps
        milliseconds nextConnect;  // Timestamp for the next connection attempt

        // Static method to get the current timestamp in milliseconds
        static milliseconds getTimestampMillis(){
            return duration_cast< milliseconds >(
                    system_clock::now().time_since_epoch()
            );
        }

    public:
        // NNGClient constructor
        explicit NNGClient(
                std::string  _host,
                const int _port,
                NNGClientType _type,
                std::size_t _reconnect_interval = 5000,
                std::size_t max_reconnect_failures = 5
        )
                : type(_type)
                , host(std::move(_host))
                , port(_port)
                , reconnect_interval(_reconnect_interval)
                , _fail_counter_max(max_reconnect_failures)
                , nextConnect(getTimestampMillis())
        {
        };

        // NNGClient destructor
        ~NNGClient() = default;

        // Method to check the connection status
        [[nodiscard]] bool isConnected() const{
            return _connected;
        }

        // Method to start listening for incoming connections (only for PUB clients)
        nng::error listen(){
            if(type != NNGClientType::PUB){
                return nng::error::syserr;
            }

            try{
                socket = nng::pub::open();
                socket.listen(host.c_str());
                _connected = true;

            }catch(const nng::exception& e){
                return e.get_error();
            }
        }

        // Method to connect the client to the server
        nng::error connect() {
            if(_connected){
                return nng::error::success;
            }

            if(getTimestampMillis() < nextConnect){
                return nng::error::internal;
            }
            nextConnect = getTimestampMillis() + reconnect_interval;

            auto addr = fmt::format("tcp://{}:{}", host, port);

            try {

                if(type == NNGClientType::REQ){
                    socket = nng::req::open();
                }else if(type == NNGClientType::PUSH){
                    socket = nng::push::open();
                    socket.set_opt_ms(nng::to_name(nng::option::send_timeout), 1000);  // Set a send timeout of 1 second
                }
                SPDLOG_INFO("Connecting to endpoint: {}", addr);
                socket.dial(addr.c_str(), 0);
                SPDLOG_INFO("Connected to endpoint: {}!", addr);
                _connected = true;
                _fail_counter = 0;
            } catch (const nng::exception& e) {
                _fail_counter++;
                SPDLOG_ERROR("Failed to connect to server: {} | Error: {} | Try: {}", addr, e.what(), _fail_counter);
                if (_fail_counter >= _fail_counter_max) {
                    SPDLOG_ERROR("Max reconnect attempts reached. Giving up.");
                }
                return e.get_error();
            }
            return nng::error::success;
        }

        // Method to get the current failure counter
        [[nodiscard]] std::size_t getFailCounter() const{
            return _fail_counter;
        }

        // Method to get the maximum allowed number of reconnect failures
        [[nodiscard]] std::size_t getNngMaxFailCount() const{
            return _fail_counter_max;
        }

        // Method to get the host address
        std::string getHost(){
            return host;
        }

        // Method to get the port number
        [[nodiscard]] int getPort() const{
            return port;
        }

        // Method to send a message using the NNG socket
        nng::error send(nng::msg msg) {
            try {
                socket.send(std::move(msg), nng::flag::nonblock);
            } catch (const nng::exception& e) {
                _connected = false;
                SPDLOG_ERROR("Failed to send message | Error: {}", e.what());
                throw e;
            }
            return nng::error::success;
        }

        // Method to receive a message using the NNG socket
        std::pair<nng::error, std::string> receive() {
            if(type == NNGClientType::PUSH){
                SPDLOG_ERROR("Cannot receive when using NNG::PUSH");
                return std::make_pair(nng::error::canceled, "Cannot receive when using NNG::PUSH");
            }
            std::string result;
            try {
                nng::buffer buf = socket.recv();
                result.assign(reinterpret_cast<char*>(buf.data()), buf.size() - 1);

            } catch (const nng::exception& e) {
                SPDLOG_ERROR("Failed to receive message | Error: {}", e.what());
                return std::make_pair(e.get_error(), result);
            }
            return std::make_pair(nng::error::success, result);
        }

    private:
        nng::socket socket;  // NNG socket
        std::size_t _fail_counter = 0;  // Current reconnect failure counter
        std::size_t _fail_counter_max;  // Maximum allowed reconnect failures
    };


}
#endif //TAGTWO_STREAMING_NNGCLIENT_H
