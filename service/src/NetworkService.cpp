//
// Created by per on 4/6/23.
//
#include "TagTwo/Networking/NetworkService.h"

std::string TagTwo::Networking::RabbitMQListener::generateUUID(int n_digits) {
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

void TagTwo::Networking::RabbitMQListener::enable_heartbeat() {
    heartbeat_enabled = true;
}

void TagTwo::Networking::RabbitMQListener::disable_heartbeat() {
    heartbeat_enabled = false;
}

void TagTwo::Networking::RabbitMQListener::add_metadata(const std::string &key, const std::string &data) {
    metadata.set(key, data);
}

void TagTwo::Networking::RabbitMQListener::start_monitor_thread() {
    // Start a new thread that periodically checks for expired services
    monitor_thread = std::thread([this]() {
        while (!stor_monitoring) {
            // Call check_services() to check for expired services
            check_services();
            // Sleep for 1 second between checks (TODO: make this configurable)
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    // Detach the thread from the main thread of execution
    monitor_thread.detach();
}

void TagTwo::Networking::RabbitMQListener::start_heartbeat_thread() {
    // Start a new thread that periodically sends heartbeat messages
    heartbeat_thread = std::thread([this]() {
        while (!stor_monitoring) {
            // Check if channel is connected and heartbeat messages are enabled
            if(channel && channel->connected() && heartbeat_enabled) {
                // Send a presence message to the RabbitMQ server
                send_presence();
            }
            // Sleep for 10 seconds between messages (TODO: make this configurable)
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    });
    // Detach the thread from the main thread of execution
    heartbeat_thread.detach();
}

void TagTwo::Networking::RabbitMQListener::send_presence() {
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

void TagTwo::Networking::RabbitMQListener::check_services() {
    // Lock the services mutex to prevent multiple threads from accessing it at once
    std::lock_guard<std::mutex> lock(services_mutex);

    // Iterate over the services map and remove any expired services
    for (auto it = services.begin(); it != services.end();) {
        if (it->second.is_expired()) {
            // If the service is expired, remove it from the services map
            it = services.erase(it);
        } else {
            // If the service is not expired, move on to the next service
            ++it;
        }
    }
}

void TagTwo::Networking::RabbitMQListener::process_message(const AMQP::Message &message) {


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
                    services.at(msg_service_id).update_heartbeat(msg_last_heartbeat);
                    services.at(msg_service_id).update_metadata(msg_metadata);
                    if(debug){
                        SPDLOG_DEBUG("Processed service discovery message for service: {}", msg_service_id);
                    }

                }else{
                    // If the service ID does not exist, create a new Service object and add it to the services map
                    if(debug){
                        SPDLOG_DEBUG("Service not found: {}", msg_service_id);
                    }

                    services.emplace(std::piecewise_construct,
                                     std::forward_as_tuple(msg_service_id),
                                     std::forward_as_tuple(msg_service_id, service, msg_heartbeat_timeout, msg_last_heartbeat, debug));

                }



            }catch (std::exception& e){
                SPDLOG_ERROR("Invalid service discovery message: {}", msg_raw);
                return;
            }

        }

    }
}

void TagTwo::Networking::RabbitMQListener::connect(std::string host, int port, const std::string &username,
                                                   const std::string &password, const std::string &vhost) {
    // Format the RabbitMQ server address using the input parameters
    auto address = fmt::format("amqp://{}:{}@{}:{}/{}", username, password, host, port, vhost);

    // Print a log message if debug mode is enabled
    if(debug){
        SPDLOG_INFO("Connecting to RabbitMQ server: {}", address);
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

            // Declare and bind queues for each input queue name
            for (const auto& queue : queues) {
                // Declare a new queue with a generated name and make it exclusive to this connection
                channel->declareQueue(fmt::format("service-{}-{}", "TODO", generateUUID(12)), AMQP::exclusive).onSuccess([this, &queue](const std::string &name, uint32_t messagecount, uint32_t consumercount) {
                    // Bind the queue to the "service-discovery" and "service-answer" topics
                    channel->bindQueue("amq.topic", queue, "service-discovery");
                    channel->bindQueue("amq.topic", name, "service-answer");

                    // Start consuming messages from the queue
                    channel->consume(name)
                            .onReceived([this](const AMQP::Message& message, uint64_t deliveryTag, bool redelivered) {
                                // Acknowledge the receipt of the message and process it
                                channel->ack(deliveryTag);
                                process_message(message);

                            });
                });
            }

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

std::vector<std::string> TagTwo::Networking::RabbitMQListener::get_existing_services() {
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

std::shared_ptr<AMQP::TcpChannel> TagTwo::Networking::RabbitMQListener::get_channel() {
    return channel;
}

TagTwo::Networking::RabbitMQListener::~RabbitMQListener() {
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

TagTwo::Networking::RabbitMQListener::RabbitMQListener(std::string serviceName, const std::vector<std::string> &queues,
                                                       int heartbeat_timeout, bool _debug)
        : queues(queues)
        , service_name(std::move(serviceName))
        , heartbeat_timeout(heartbeat_timeout)
        , evbase(event_base_new())
        , service_id(RabbitMQListener::generateUUID())
        , heartbeat_enabled(true)
        , debug(_debug)
{
    start_monitor_thread();
    start_heartbeat_thread();
}

bool TagTwo::Networking::NetworkService::is_expired() {
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
TagTwo::Networking::NetworkService::time_difference(const std::chrono::time_point<std::chrono::system_clock> &now,
                                                    const std::chrono::duration<int64_t> &last) {
    return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch() - last);
}

std::chrono::seconds TagTwo::Networking::NetworkService::last_heartbeat() {
    std::lock_guard<std::mutex> lock(heartbeat_mutex);
    return _last_heartbeat;
}

void TagTwo::Networking::NetworkService::update_heartbeat(int last_heartbeat) {
    std::lock_guard<std::mutex> lock(heartbeat_mutex);
    _last_heartbeat = std::chrono::seconds(last_heartbeat);
    //SPDLOG_INFO("{}: Heartbeat updated for {}", service_uid);
}

void TagTwo::Networking::NetworkService::update_metadata(std::string _metadata) {
    metadata = std::move(_metadata);
}

TagTwo::Networking::NetworkService::NetworkService(std::string _serviceUID, std::string _serviceType,
                                                   int heartbeat_timeout, int last_heartbeat, bool _debug)
        : service_type(std::move(_serviceType))
        , heartbeat_timeout(heartbeat_timeout)
        , service_uid(std::move(_serviceUID))
        , _last_heartbeat(std::chrono::seconds(last_heartbeat))
        , debug(_debug)
{

}
