//
// Created by per on 4/3/23.
//

#ifndef TAGTWO_STREAMING_NUMPYBUFFERPROTOCOL_H
#define TAGTWO_STREAMING_NUMPYBUFFERPROTOCOL_H

#include <cstdint>

namespace TagTwo::Networking{
    struct NumpyBufferData{
        uint64_t service_uid;
        uint64_t uid;
        uint64_t timestamp;
        uint32_t width;
        uint32_t height;
        uint32_t channels;
        uint32_t type;
        uint8_t data[];
    };

}



#endif //TAGTWO_STREAMING_NUMPYBUFFERPROTOCOL_H
