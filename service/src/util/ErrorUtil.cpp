//
// Created by per on 4/11/23.
//

#include <spdlog/spdlog.h>
#include "TagTwo/Networking/Util/ErrorUtil.h"

bool TagTwo::Networking::Util::ErrorUtil::handle_async_error(boost::system::error_code error) {

    switch (error.value()) {
        case boost::asio::error::eof:
            SPDLOG_DEBUG("End of file (normal client shutdown): {}", error.message());
            return true;
        case boost::asio::error::operation_aborted:
            SPDLOG_DEBUG("Operation canceled (normal server shutdown): {}", error.message());
            return true;
        case boost::asio::error::connection_aborted:
        default:
            SPDLOG_ERROR("Error in async operation: {}", error.message());
            return false;
    }
}
