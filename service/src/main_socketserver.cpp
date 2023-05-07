//
// Created by per on 4/10/23.
//
#include "TagTwo/Networking/GameSocket/GameSocketClientSSL.h"
#include "TagTwo/Networking/GameSocket/GameSocketServer.h"
#include <boost/asio/detached.hpp>
#include <iostream>
#include <vector>
#include <boost/asio/buffers_iterator.hpp>


using TagTwo::Networking::DataHeader;
using TagTwo::Networking::DataPacket;
using TagTwo::Networking::DataType;

// This function simulates creating and processing data of different types.
DataPacket create_test_data(DataType data_type) {
    DataPacket packet;

    // Set common header values
    packet.header.magic = TagTwo::Networking::PACKET_MAGIC;
    packet.header.version = TagTwo::Networking::NETWORK_VERSION;
    packet.header.data_type = data_type;
    packet.header.timestamp = 0; // Set a dummy timestamp
    /*
    switch (data_type) {
        case DataType::IMAGE:
            packet.header.data_size = sizeof(ImageHeader) + 1000;
            packet.header.info_header = true;
            packet.header.info.img.width = 640;
            packet.header.info.img.height = 480;
            packet.header.info.img.codec_id = 1; // Dummy codec ID
            break;
        case DataType::VIDEO:
            packet.header.data_size = sizeof(VideoHeader) + 5000;
            packet.header.info_header = true;
            packet.header.info.vid.frame_rate = 30;
            packet.header.info.vid.sample_rate = 48000;
            packet.header.info.vid.codec_id = 2; // Dummy codec ID
            packet.header.info.vid.width = 1280;
            packet.header.info.vid.height = 720;
            break;
        case DataType::AUDIO:
            packet.header.data_size = sizeof(AudioHeader) + 2000;
            packet.header.info_header = true;
            packet.header.info.aud.sample_rate = 44100;
            packet.header.info.aud.codec_id = 3; // Dummy codec ID
            break;
        default:
            packet.header.data_size = 100;
            packet.header.info_header = false;
            break;
    }

    // Allocate memory for the data body and fill it with dummy data
    packet.body.data.reserve(packet.header.data_size - (packet.header_meta ? sizeof(DataHeader) : 0));*/


    uint8_t fill_value;
    switch (data_type) {
        case DataType::IMAGE:
            fill_value = 0x42;
            break;
        case DataType::VIDEO:
            fill_value = 0x24;
            break;
        case DataType::AUDIO:
            fill_value = 0x11;
            break;
        default:
            fill_value = 0x01;
            break;
    }

    std::fill(packet.body.data.begin(), packet.body.data.end(), fill_value);    // Fill the data body with dummy data

    return packet;
}


#include <chrono>
#include <iterator>
#include <boost/asio/co_spawn.hpp>

class tqdm {
public:
    class iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = int;
        using difference_type = int;
        using pointer = int*;
        using reference = int&;

        iterator(tqdm* parent, int current)
                : parent_(parent), current_(current) {}

        int operator*() const { return current_; }
        iterator& operator++() {
            ++current_;
            parent_->update();
            return *this;
        }

        bool operator!=(const iterator& other) const {
            return current_ != other.current_;
        }

    private:
        tqdm* parent_;
        int current_;
    };

    tqdm(int total_iterations, int report_every = 1000)
            : total_iterations_(total_iterations),
              report_every_(report_every),
              start_time_(std::chrono::steady_clock::now()) {}

    iterator begin() {
        return iterator(this, 0);
    }

    iterator end() {
        return iterator(this, total_iterations_);
    }

    void update() {
        ++current_iteration_;
        if (current_iteration_ % report_every_ == 0) {
            display_progress_bar();
        }
    }

private:
    int total_iterations_;
    int report_every_;
    int current_iteration_ = 0;
    std::chrono::steady_clock::time_point start_time_;

    void display_progress_bar() {
        float progress = float(current_iteration_) / float(total_iterations_);
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time_).count();
        float iterations_per_second = current_iteration_ / (elapsed_time / 1000.0);

        int bar_width = 50;
        std::cout << "[";
        int position = bar_width * progress;
        for (int i = 0; i < bar_width; ++i) {
            if (i < position) std::cout << "=";
            else if (i == position) std::cout << ">";
            else std::cout << " ";
        }

        std::cout << "] " << int(progress * 100.0) << "%  " << iterations_per_second << " iter/s\r";
        std::cout.flush();
    }
};



int main() {
    // Initialize the logger
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%L%$] %v");

    // Set up the server
    std::string host = "127.0.0.1";
    int port = 4444;
    bool use_ssl = false;
    TagTwo::Networking::GameSocketServer server(use_ssl);
    server.start(host, port, false);

    server.onData.connect([](const std::shared_ptr<TagTwo::Networking::GameSocket>& client, DataPacket& packet) {
        //spdlog::info("Received data of type {}", packet.header.data_type);
        client->send_data_sync(packet);
    });


    // Set up the client
    auto client = std::make_shared<TagTwo::Networking::GameSocketClient>(use_ssl);

    // Connect the client to the server
    client->connect_sync(host, port);

    boost::asio::io_context io_context(2);

    boost::asio::co_spawn(io_context, [&]() -> awaitable<void>{
        while(true)
        {
            co_await client->read_data();
        }
    }, boost::asio::detached);


    boost::asio::co_spawn(io_context, [&]() -> awaitable<void>{
        // Test different data types
        for (int i : tqdm(100000000)){
            for (auto data_type : {TagTwo::Networking::IMAGE, TagTwo::Networking::VIDEO, TagTwo::Networking::AUDIO, TagTwo::Networking::NONE}) {
                // Create test data
                auto test_data = create_test_data(data_type);


                // Send the data to the server
                co_await client->send_data(test_data);
            }
        }

        while(true)
        {
            co_await client->read_data();
        }
    }, boost::asio::detached);


    io_context.run();
    client.reset();

    // Clean up
    server.stop();

    return 0;
}
