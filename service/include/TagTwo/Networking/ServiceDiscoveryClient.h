#ifndef TAGTWO_STREAMING_NETWORKSERVICE_H
#define TAGTWO_STREAMING_NETWORKSERVICE_H
#include <memory>
#include <unordered_map>
#include <amqpcpp.h>
#include <amqpcpp/libevent.h>
#include <thread>
#include <mutex>

#include "TagTwo/Util/JsonBuilder.h"
#include "TagTwo/Networking/ServiceDiscoveryRecord.h"

namespace TagTwo::Networking{

    /** Forward declaration of the ServiceDiscoveryClient class. */
    class ServiceDiscoveryRecord;

    /**
     * @brief ServiceDiscoveryClient class that listens to multiple RabbitMQ queues and manages ServiceDiscoveryClient objects.
     */
    class ServiceDiscoveryClient {

    private:
        std::shared_ptr<AMQP::LibEventHandler> connection_handler;
        std::shared_ptr<AMQP::TcpConnection> connection;
        std::shared_ptr<AMQP::TcpChannel> channel;
        const int heartbeat_timeout;
        std::unordered_map<std::string, std::shared_ptr<ServiceDiscoveryRecord>> services;
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
        const std::string report_queue;
        const std::string answer_routing_key;
        const int heartbeat_interval;
        const int service_check_interval;

    public:

        /**
         * @brief Construct a new ServiceDiscoveryClient object.
         * @param queues A vector of queue names to listen to.
         * @param heartbeat_timeout The timeout value for heartbeats.
         */
        ServiceDiscoveryClient(
                std::string serviceName,
                std::string report_queue,
                std::string answer_routing_key,
                int heartbeat_timeout,
                int heartbeat_interval,
                int service_check_interval,
                bool _debug
        );

        /**
         * @brief Destroy the ServiceDiscoveryClient object, stopping the monitoring thread.
         */
        ~ServiceDiscoveryClient();

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

        std::string get_service_id();

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



        std::vector<std::shared_ptr<ServiceDiscoveryRecord>> get_services(){
            std::vector<std::shared_ptr<ServiceDiscoveryRecord>> result;
            result.reserve(services.size());
            std::lock_guard<std::mutex> lock(services_mutex);
            for (auto& service : services){
                result.push_back(service.second);
            }
            return result;
        }


        std::vector<std::shared_ptr<ServiceDiscoveryRecord>> get_services(std::string service_type){
            auto result = get_services();
            result.erase(std::remove_if(result.begin(), result.end(), [&service_type](const std::shared_ptr<ServiceDiscoveryRecord>& service){
                return service->get_service_type() != service_type;
            }), result.end());
            return result;

        }

    };

}


#endif //TAGTWO_STREAMING_NETWORKSERVICE_H
