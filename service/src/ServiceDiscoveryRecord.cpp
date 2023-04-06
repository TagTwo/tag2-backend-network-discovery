//
// Created by per on 4/6/23.
//
#include <spdlog/spdlog.h>
#include "TagTwo/Networking/ServiceDiscoveryRecord.h"


bool TagTwo::Networking::ServiceDiscoveryRecord::is_expired() {
    // Get the current time
    auto current_time = std::chrono::system_clock::now();

    // Get the last heartbeat time (duration)
    auto last_heartbeat_duration = last_heartbeat();

    // Calculate the time difference between now and the last heartbeat
    auto time_diff = time_difference(current_time, last_heartbeat_duration);

    if (debug) {
        SPDLOG_INFO("Now: {} Last: {} Diff: {}, Timeout: {}, Expired: {}",
                    current_time.time_since_epoch().count(),
                    last_heartbeat_duration.count(),
                    time_diff.count(),
                    heartbeat_timeout,
                    time_diff.count() > heartbeat_timeout
        );
    }

    // Check if the time difference exceeds the heartbeat timeout
    bool expired = time_diff.count() > heartbeat_timeout;
    return expired;
}

std::chrono::seconds
TagTwo::Networking::ServiceDiscoveryRecord::time_difference(const std::chrono::time_point<std::chrono::system_clock> &now,
                                                            const std::chrono::duration<int64_t> &last) {
    return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch() - last);
}

std::chrono::seconds TagTwo::Networking::ServiceDiscoveryRecord::last_heartbeat() {
    std::lock_guard<std::mutex> lock(heartbeat_mutex);
    return _last_heartbeat;
}

void TagTwo::Networking::ServiceDiscoveryRecord::update_heartbeat(int last_heartbeat) {
    std::lock_guard<std::mutex> lock(heartbeat_mutex);
    _last_heartbeat = std::chrono::seconds(last_heartbeat);
    //SPDLOG_INFO("{}: Heartbeat updated for {}", service_uid);
}

void TagTwo::Networking::ServiceDiscoveryRecord::update_metadata(std::string _metadata) {
    metadata = std::move(_metadata);
}

TagTwo::Networking::ServiceDiscoveryRecord::ServiceDiscoveryRecord(std::string _serviceUID, std::string _serviceType,
                                                                   int heartbeat_timeout, int last_heartbeat, bool _debug)
        : service_type(std::move(_serviceType))
        , heartbeat_timeout(heartbeat_timeout)
        , service_uid(std::move(_serviceUID))
        , _last_heartbeat(std::chrono::seconds(last_heartbeat))
        , debug(_debug)
{

}