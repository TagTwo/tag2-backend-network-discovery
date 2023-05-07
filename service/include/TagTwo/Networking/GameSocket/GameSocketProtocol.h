//
// Created by per on 2/26/23.
//

#ifndef TAG2_DATAPROTOCOL_H
#define TAG2_DATAPROTOCOL_H
#include <vector>
#include <cstdlib>
#include <memory>

namespace TagTwo::Networking {

    constexpr uint8_t PACKET_MAGIC = (uint8_t)0x99414144;
    constexpr uint8_t NETWORK_VERSION = TAGTWO_NETWORKING_VERSION;

    enum DataType {
        NONE=0,
        IMAGE,
        VIDEO,
        AUDIO,
    };

    struct MetaHeader {
        uint16_t frame_rate; // For video data
        uint16_t sample_rate; // For audio data
        uint16_t codec_id; // Identifies the codec used for encoding the data
        uint16_t width; // For video data
        uint16_t height; // For video data
    };


    struct DataHeader {
        uint8_t magic = PACKET_MAGIC;
        uint8_t version = (uint8_t) TAGTWO_NETWORKING_VERSION;
        uint8_t data_type;
        uint32_t data_size;
        uint32_t timestamp;
    };

    struct DataBody {
        std::vector<uint8_t> data;

        DataBody() = default;
        DataBody(DataBody&& other) noexcept = default;
        DataBody(const DataBody& other) = delete;
    };

    struct DataPacket {
        DataHeader header{};
        MetaHeader header_meta{};
        DataBody body{};

    };


    std::vector<uint8_t> DataPacketToBytes(const DataPacket &packet);

}

#endif //TAG2_DATAPROTOCOL_H
