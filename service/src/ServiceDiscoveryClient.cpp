//
// Created by per on 4/6/23.
//
#include "TagTwo/Networking/ServiceDiscoveryClient.h"
#include "TagTwo/Networking/ServiceDiscoveryRecord.h"
#include <utility>
#include <random>
#include <spdlog/spdlog.h>


std::string TagTwo::Networking::ServiceDiscoveryClient::generateUUID(int n_digits) {
    // Seed a random number generator using a hardware-based random number generator, if available.
    std::random_device rd;
    // Use the Mersenne Twister 19937 algorithm for the random number generator.
    std::mt19937 gen(rd());
    // Generate random numbers between 0 and 15 (inclusive).
    std::uniform_int_distribution<> dis(0, 15);

    // Create a string stream to build the UUID string.
    std::stringstream ss;
    // Generate 32 hexadecimal digits (with hyphens added at positions 8, 12, 16, and 20).
    for (int i = 0; i < n_digits; i++) {
        if (i == 8 || i == 12 || i == 16 || i == 20) {
            ss << "-";
        }
        ss << std::hex << dis(gen);
    }

    // Return the generated UUID as a string.
    return ss.str();
}

void TagTwo::Networking::ServiceDiscoveryClient::enable_heartbeat() {
    heartbeat_enabled = true;
}

void TagTwo::Networking::ServiceDiscoveryClient::disable_heartbeat() {
    heartbeat_enabled = false;
}

void TagTwo::Networking::ServiceDiscoveryClient::add_metadata(const std::string &key, const std::string &data) {
    metadata.set(key, data);
}

void TagTwo::Networking::ServiceDiscoveryClient::start_monitor_thread() {
    // Start a new thread that periodically checks for expired services
    monitor_thread = std::thread([this]() {
        while (!stor_monitoring) {
            // Call check_services() to check for expired services
            check_services();
            // Sleep for X second between checks
            std::this_thread::sleep_for(std::chrono::seconds(service_check_interval));
        }
    });
    // Detach the thread from the main thread of execution
    monitor_thread.detach();
}

void TagTwo::Networking::ServiceDiscoveryClient::start_heartbeat_thread() {
    // Start a new thread that periodically sends heartbeat messages
    heartbeat_thread = std::thread([this]() {
        while (!stor_monitoring) {
            // Check if channel is connected and heartbeat messages are enabled
            if(channel && channel->connected() && heartbeat_enabled) {
                // Send a presence message to the RabbitMQ server
                send_presence();
            }
            // Sleep for 10 seconds between messages
            std::this_thread::sleep_for(std::chrono::seconds(heartbeat_interval));
        }
    });
    // Detach the thread from the main thread of execution
    heartbeat_thread.detach();
}

void TagTwo::Networking::ServiceDiscoveryClient::send_presence() {
    // Get a channel object for sending the presence message
    auto _channel = get_channel();

    // Create a JSON object representing the service presence data
    nlohmann::json data_json = {
            {"service",           service_name},
            {"service_id",        service_id},
            {"heartbeat_timeout", heartbeat_timeout},
            {"timestamp",         (std::chrono::system_clock::now().time_since_epoch() / std::chrono::seconds (1))},
            {"metadata",          metadata.toJSON()}
    };
    // Convert the JSON object to a string
    auto data_string = data_json.dump();

    // Create an AMQP envelope with the presence message data
    AMQP::Envelope envelope(data_string.c_str(), data_string.size());

    // Publish the presence message to the RabbitMQ server
    _channel->publish("amq.topic", "service-discovery", data_string);
}

void TagTwo::Networking::ServiceDiscoveryClient::check_services() {
    // Lock the services mutex to prevent multiple threads from accessing it at once
    std::lock_guard<std::mutex> lock(services_mutex);

    // Iterate over the services map and remove any expired services
    for (auto it = services.begin(); it != services.end();) {
        if (it->second->is_expired()) {
            // If the service is expired, remove it from the services map
            it = services.erase(it);
        } else {
            // If the service is not expired, move on to the next service
            ++it;
        }
    }
}

void TagTwo::Networking::ServiceDiscoveryClient::process_message(const AMQP::Message &message) {


    // Check if the message is a service discovery message
    if (message.routingkey() == "service-answer") {
        // Convert the message body to a JSON object
        auto msg_raw = std::string(message.body(), message.bodySize());
        auto msg = nlohmann::json::parse(msg_raw);

        // Check that the message is a JSON array
        if(!msg.is_array()){
            SPDLOG_ERROR("Invalid service discovery message: {}", msg_raw);
            return;
        }

        // Process each service data object in the message
        if(debug){
            SPDLOG_INFO("Received service discovery message: {}", msg.size());
        }
        for(auto& service_data : msg){
            // Extract the service ID, name, heartbeat timeout, and last heartbeat time
            try{
                auto service = service_data["service"].get<std::string>();
                auto msg_service_id = service_data["service_id"].get<std::string>();
                auto msg_heartbeat_timeout = service_data["heartbeat_timeout"].get<int>();
                auto msg_last_heartbeat= service_data["last_heartbeat"].get<int>();
                auto msg_metadata = service_data["metadata"].get<std::string>();


                // Check if the service ID already exists in the services map
                if(services.count(msg_service_id) > 0){
                    // If the service ID exists, update its last heartbeat time
                    services.at(msg_service_id)->update_heartbeat(msg_last_heartbeat);
                    services.at(msg_service_id)->update_metadata(msg_metadata);
                    if(debug){
                        SPDLOG_DEBUG("Processed service discovery message for service: {}", msg_service_id);
                    }

                }else{
                    // If the service ID does not exist, create a new Service object and add it to the services map
                    if(debug){
                        SPDLOG_DEBUG("Service not found: {}", msg_service_id);
                    }

                    services.emplace(
                            msg_service_id,
                            std::make_shared<ServiceDiscoveryRecord>(
                                    msg_service_id,
                                    service,
                                    msg_heartbeat_timeout,
                                    msg_last_heartbeat,
                                    debug
                            )
                    );

                }



            }catch (std::exception& e){
                SPDLOG_ERROR("Invalid service discovery message: {}", msg_raw);
                return;
            }

        }

    }
}

void TagTwo::Networking::ServiceDiscoveryClient::connect(std::string host, int port, const std::string &username,
                                                         const std::string &password, const std::string &vhost) {
    // Format the RabbitMQ server address using the input parameters
    auto address = fmt::format("amqp://{}:{}@{}:{}/{}", username, password, host, port, vhost);

    // Print a log message if debug mode is enabled
    if(debug){
        SPDLOG_INFO("Connecting to RabbitMQ server: {}:{}/{}", host, port, vhost);
    }

    // Try to connect to the RabbitMQ server, retrying until successful
    while (!connected) {
        try {
            // Create a new event handler using libevent
            connection_handler = std::make_shared<AMQP::LibEventHandler>(evbase);

            // Create a new TCP connection to the RabbitMQ server
            connection = std::make_shared<AMQP::TcpConnection>(
                    connection_handler.get(),
                    address
            );

            // Create a new TCP channel within the connection
            channel = std::make_shared<AMQP::TcpChannel>(connection.get());

            // Set up an error handler for the channel
            channel->onError([](const char* message) {
                SPDLOG_ERROR("Channel error: {}", message);
            });


            // Declare a new queue with a generated name and make it exclusive to this connection
            channel->declareQueue(fmt::format("service-{}-{}", service_name, generateUUID(12)), AMQP::exclusive).onSuccess([this](const std::string &name, uint32_t messagecount, uint32_t consumercount) {
                // Bind the queue to the "service-discovery" and "service-answer" topics
                channel->bindQueue("amq.topic", report_queue, report_queue);
                channel->bindQueue("amq.topic", name, answer_routing_key);

                // Start consuming messages from the queue
                channel->consume(name)
                        .onReceived([this](const AMQP::Message& message, uint64_t deliveryTag, bool redelivered) {
                            // Acknowledge the receipt of the message and process it
                            channel->ack(deliveryTag);
                            process_message(message);

                        });
            });


            // Set the connected flag to true and print a log message if debug mode is enabled
            connected = true;
            if(debug){
                SPDLOG_INFO("Connected to RabbitMQ server: {}", address);
            }
        } catch (const std::exception& e) {
            // Print an error message and wait for 5 seconds before retrying
            SPDLOG_ERROR("Failed to connect to RabbitMQ server: {}. Retrying in 5 seconds...", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    // Start the libevent thread to handle the connection in the background
    libeven_thread = std::thread([this]() {
        event_base_dispatch(evbase);
    });
}

std::vector<std::string> TagTwo::Networking::ServiceDiscoveryClient::get_existing_services() {
    // Lock the services mutex to prevent multiple threads from accessing it at once
    std::lock_guard<std::mutex> lock(services_mutex);

    // Create a new vector to hold the service names and reserve space for them
    std::vector<std::string> serviceNames;
    serviceNames.reserve(services.size());

    // Iterate over the services map and add each service name to the vector
    for (const auto& service : services) {
        serviceNames.push_back(service.first);
    }

    // Return the vector of service names
    return serviceNames;
}

std::shared_ptr<AMQP::TcpChannel> TagTwo::Networking::ServiceDiscoveryClient::get_channel() {
    return channel;
}

TagTwo::Networking::ServiceDiscoveryClient::~ServiceDiscoveryClient() {
    // Set the stor_monitoring flag to true to signal the monitoring thread to exit
    stor_monitoring = true;

    // Break out of the event loop and free the event base
    event_base_loopbreak(evbase);
    event_base_free(evbase);

    // Join the monitoring thread and libevent thread (if they are joinable)
    if (monitor_thread.joinable()) {
        monitor_thread.join();
    }
    if (libeven_thread.joinable()) {
        libeven_thread.join();
    }
}


TagTwo::Networking::ServiceDiscoveryClient::ServiceDiscoveryClient(
        std::string _serviceName,
        std::string _report_queue,
        std::string _answer_routing_key="service-answer",
        int _heartbeat_timeout=120,
        int _heartbeat_interval=10,
        int _service_check_interval=5,
        bool _debug=false
)
        : report_queue(std::move(_report_queue))
        , answer_routing_key(std::move(_answer_routing_key))
        , service_name(std::move(_serviceName))
        , heartbeat_timeout(_heartbeat_timeout)
        , heartbeat_interval(_heartbeat_interval)
        , service_check_interval(_service_check_interval)
        , evbase(event_base_new())
        , service_id(ServiceDiscoveryClient::generateUUID())
        , heartbeat_enabled(true)
        , debug(_debug)
{
    start_monitor_thread();
    start_heartbeat_thread();
}

std::string TagTwo::Networking::ServiceDiscoveryClient::get_service_id() {
    return service_id;
}

std::vector<std::shared_ptr<ServiceDiscoveryRecord>>
TagTwo::Networking::ServiceDiscoveryClient::get_services(std::string service_type) {
    auto result = get_services();
    result.erase(std::remove_if(result.begin(), result.end(), [&service_type](const std::shared_ptr<ServiceDiscoveryRecord>& service){
        return service->get_service_type() != service_type;
    }), result.end());
    return result;

}

std::vector<std::shared_ptr<ServiceDiscoveryRecord>> TagTwo::Networking::ServiceDiscoveryClient::get_services() {
    std::vector<std::shared_ptr<ServiceDiscoveryRecord>> result;
    result.reserve(services.size());
    std::lock_guard<std::mutex> lock(services_mutex);
    for (auto& service : services){
        result.push_back(service.second);
    }
    return result;
}
