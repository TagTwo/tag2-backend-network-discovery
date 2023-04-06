#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <map>
#include <amqpcpp.h>
#include <amqpcpp/libevent.h>
#include <mutex>
#include <utility>
#include <vector>
#include <memory>
#include <atomic>
#include <spdlog/spdlog.h>
#include <iostream>
#include <random>
#include <sstream>
#include <nlohmann/json.hpp>
#include "TagTwo/Util/JsonBuilder.h"

#ifndef TAGTWO_STREAMING_NETWORKSERVICE_H
#define TAGTWO_STREAMING_NETWORKSERVICE_H



namespace TagTwo::Networking{


/**
 * @class NetworkService
 * @brief Represents a network service and manages its metadata, heartbeat, and expiration status.
 */
    class NetworkService {
    public:
        /**
         * @brief Constructs a NetworkService object with the given parameters.
         * @param _serviceUID A unique identifier for the service.
         * @param _serviceType The type of the service (e.g. "ServiceA").
         * @param heartbeat_timeout The timeout value for heartbeats.
         * @param last_heartbeat The last recorded heartbeat time.
         * @param _debug A boolean flag to enable or disable debug output.
         */
        NetworkService(
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
                const std::chrono::time_point<std::chrono::system_clock>& now,
                const std::chrono::duration<int64_t>& last
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


/**
 * @brief RabbitMQListener class that listens to multiple RabbitMQ queues and manages NetworkService objects.
 */
    class RabbitMQListener {

    private:
        std::shared_ptr<AMQP::LibEventHandler> connection_handler;
        std::shared_ptr<AMQP::TcpConnection> connection;
        std::shared_ptr<AMQP::TcpChannel> channel;
        std::vector<std::string> queues;
        int heartbeat_timeout;
        std::unordered_map<std::string, NetworkService> services;
        std::mutex services_mutex;
        std::thread monitor_thread;
        event_base* evbase;
        std::atomic<bool> stor_monitoring{false};
        std::thread libeven_thread;
        const std::string service_name;
        const std::string service_id;
        bool connected = false;
        std::thread heartbeat_thread;
        bool heartbeat_enabled;
        bool debug;
        TagTwo::Util::JsonBuilder metadata;

    public:

        /**
         * @brief Construct a new RabbitMQListener object.
         * @param queues A vector of queue names to listen to.
         * @param heartbeat_timeout The timeout value for heartbeats.
         */
        RabbitMQListener(
                std::string serviceName,
                const std::vector<std::string>& queues,
                int heartbeat_timeout,
                bool _debug
        );

        /**
         * @brief Destroy the RabbitMQListener object, stopping the monitoring thread.
         */
        ~RabbitMQListener();

        /**
         * @brief Get the AMQP::TcpChannel shared pointer.
         * @return A shared pointer to the AMQP::TcpChannel object.
         */
        std::shared_ptr<AMQP::TcpChannel> get_channel();

        /**
         * @brief Get a vector of existing service names.
         * @return A vector containing the names of existing services.
         */
        std::vector<std::string> get_existing_services();

        /**
         * @brief Connects to the RabbitMQ server and sets up the listener.
         *
         * This function will attempt to connect to the specified RabbitMQ server using the provided
         * host, port, username, password, and vhost. If the connection fails, it will continue
         * retrying until a successful connection is established.
         *
         * @param host The hostname or IP address of the RabbitMQ server.
         * @param port The port number to connect to.
         * @param username The username for RabbitMQ authentication.
         * @param password The password for RabbitMQ authentication.
         * @param vhost The virtual host to use (default is "/").
         */
        void connect(std::string host, int port, const std::string& username, const std::string& password, const std::string& vhost="");

        /**
         * Generates a version 4 UUID (Universally Unique Identifier) using random numbers.
         *
         * @return The generated UUID as a string.
         */
        static std::string generateUUID(int n_digits=32);

        /**
         * @brief Enable the heartbeat feature of the class.
         *
         * This function sets a flag to enable the heartbeat feature of the class. When the
         * heartbeat feature is enabled, the class will periodically send heartbeat messages
         * to the connected RabbitMQ server to indicate that it is still alive.
         */
        void enable_heartbeat();

        /**
         * @brief Disable the heartbeat feature of the class.
         *
         * This function sets a flag to disable the heartbeat feature of the class. When the
         * heartbeat feature is disabled, the class will not send any heartbeat messages to
         * the connected RabbitMQ server.
         */
        // This function sets a flag to disable the heartbeat feature.
        void disable_heartbeat();

        /**
         * @brief Enable the monitoring feature of the class.
         *
         * This function sets a flag to enable the monitoring feature of the class. When the
         * monitoring feature is enabled, the class will periodically check for expired
         * services and remove them from the list of available services.
         */
        void add_metadata(const std::string& key, const std::string& data);

    private:

        /**
         * @brief Start a thread to monitor the services.
         */
        void start_monitor_thread();


        /**
         * @brief Start a thread to send heartbeat messages.
         */
        void start_heartbeat_thread();


        /**
         * @brief Send a presence message to the RabbitMQ exchange.
         * @param serviceName The name of the service.
         */
        void send_presence();


        /**
         * @brief Check for expired services and remove them.
         */
        void check_services();

        /**
         * @brief Process the incoming messages from the RabbitMQ queues.
         * @param message The received AMQP::Message.
         * @param body The body of the received AMQP::Message.
         */
        void process_message(const AMQP::Message& message);


    };

}


#endif //TAGTWO_STREAMING_NETWORKSERVICE_H
