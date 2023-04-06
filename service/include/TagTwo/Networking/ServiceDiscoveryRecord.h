//
// Created by per on 4/6/23.
//

#ifndef TAGTWO_NETWORK_DISCOVERY_SERVICEDISCOVERYRECORD_H
#define TAGTWO_NETWORK_DISCOVERY_SERVICEDISCOVERYRECORD_H

#include <string>
#include <chrono>
#include <memory>

namespace TagTwo::Networking {

/**
 * @class NetworkService
 * @brief Represents a network service and manages its metadata, heartbeat, and expiration status.
 */
    class ServiceDiscoveryRecord {
    public:
        /**
         * @brief Constructs a NetworkService object with the given parameters.
         * @param _serviceUID A unique identifier for the service.
         * @param _serviceType The type of the service (e.g. "ServiceA").
         * @param heartbeat_timeout The timeout value for heartbeats.
         * @param last_heartbeat The last recorded heartbeat time.
         * @param _debug A boolean flag to enable or disable debug output.
         */
        ServiceDiscoveryRecord(
                std::string _serviceUID,
                std::string _serviceType,
                int heartbeat_timeout,
                int last_heartbeat,
                bool _debug
        );

        /**
         * @brief Updates the metadata of the service.
         * @param _metadata The new metadata string.
         */
        void update_metadata(std::string _metadata);

        /**
         * @brief Updates the _last_heartbeat to the current time.
         * @param last_heartbeat The new last heartbeat time.
         */
        void update_heartbeat(int last_heartbeat);

        /**
         * @brief Returns the last recorded heartbeat time.
         * @return A std::chrono::seconds object representing the last recorded heartbeat time.
         */
        std::chrono::seconds last_heartbeat();

        /**
         * @brief Calculates the time difference between 'now' and 'last' in seconds.
         * @param now A std::chrono::time_point object representing the current time.
         * @param last A std::chrono::duration object representing the last time.
         * @return A std::chrono::seconds object representing the time difference between 'now' and 'last'.
         */
        static std::chrono::seconds time_difference(
                const std::chrono::time_point <std::chrono::system_clock> &now,
                const std::chrono::duration <int64_t> &last
        );

        /**
         * @brief Checks if the service is expired based on its last heartbeat and the heartbeat_timeout.
         * @return A boolean value indicating whether the service is expired.
         */
        bool is_expired();

    private:
        const std::string service_type; ///< The type of the service (e.g., "ServiceA").
        std::chrono::seconds _last_heartbeat; ///< Time of the last recorded heartbeat.
        int heartbeat_timeout; ///< Timeout value for heartbeats.
        std::mutex heartbeat_mutex; ///< Mutex to protect access to the _last_heartbeat variable.
        const std::string service_uid; ///< Unique identifier for the service.
        std::string metadata; ///< Metadata associated with the service.
        bool debug; ///< Debug flag to enable or disable debug output.

    };

}

#endif //TAGTWO_NETWORK_DISCOVERY_SERVICEDISCOVERYRECORD_H
