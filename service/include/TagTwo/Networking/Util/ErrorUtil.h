//
// Created by per on 4/11/23.
//

#ifndef TAGTWO_NETWORK_DISCOVERY_ERRORUTIL_H
#define TAGTWO_NETWORK_DISCOVERY_ERRORUTIL_H
#include <boost/asio/error.hpp>
namespace TagTwo::Networking::Util::ErrorUtil {

    bool handle_async_error(boost::system::error_code error);

}

#endif //TAGTWO_NETWORK_DISCOVERY_ERRORUTIL_H
