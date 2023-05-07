//
// Created by per on 4/10/23.
//
#include <cstdint>
#include <cstddef>
#include "TagTwo/Networking/GameSocket/GameSocketProtocol.h"

std::vector<uint8_t> TagTwo::Networking::DataPacketToBytes(const TagTwo::Networking::DataPacket &packet) {
    // Calculate the size of the info header

    // Calculate the total size and reserve space in the vector
    size_t totalSize = sizeof(packet.header) - sizeof(packet.header_meta)  + packet.header.data_size;
    std::vector<uint8_t> bytes;
    bytes.reserve(totalSize);

    // Serialize the header, info, and body using memcpy and updating the pointer
    const auto* dataPtr = reinterpret_cast<const uint8_t*>(&packet.header);
    bytes.insert(bytes.end(), dataPtr, dataPtr + sizeof(packet.header) - sizeof(packet.header_meta));

    dataPtr = reinterpret_cast<const uint8_t*>(&packet.header_meta);
    bytes.insert(bytes.end(), dataPtr, dataPtr);

    const std::vector<uint8_t>& dataVec = packet.body.data;
    bytes.insert(bytes.end(), dataVec.begin(), dataVec.end());

    return bytes;
}

