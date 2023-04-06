#include "TagTwo/Networking/ServiceDiscoveryClient.h"
#include "TagTwo/Util/Dotenv.h"
#include <cstdlib>
#include <ctime>
#include <random>

/**
 * @brief Simulates a service instance that sends heartbeats and goes offline randomly.
 *
 * This function simulates a service instance by connecting to the RabbitMQ server and
 * periodically sending heartbeats as messages with the given service_name and id.
 * The service instance may randomly go offline, during which it stops sending heartbeats,
 * and then come back online after a random duration.
 *
 * @param serviceName The name of the service.
 * @param id The unique identifier of the service instance.
 */
void simulate_service(const std::string& username, const std::string& password, const std::string& host, int port, const std::string& serviceName, int id) {

    TagTwo::Networking::ServiceDiscoveryClient listener(serviceName, "service-discovery", "service-answer", 120, 10, 5, false);
    listener.connect(host, port, username, password);
    listener.add_metadata("connection/game", R"(["localhost:8080", "81.22.15.13:5333"])");

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 5);
    std::uniform_int_distribution<> offline_duration(5, 400);

    bool is_offline = false;
    int offline_counter = 0;

    SPDLOG_INFO("Simulating service {} with id {}", serviceName, id);

    while (true) {
        if (!is_offline) {
            std::string service_instance_name = serviceName + "_" + std::to_string(id);
            auto channel = listener.get_channel();
            //channel->publish("amq.topic", "service-discovery", service_instance_name);
            //SPDLOG_DEBUG("Service {} with id {} sent heartbeat", serviceName, id);

            if (rand() % 10 == 0) { // Randomly make a service go offline
                is_offline = true;
                offline_counter = offline_duration(gen);
                //SPDLOG_INFO("Service {} with id {} goes offline", serviceName, id);
                listener.disable_heartbeat();
            }
        } else {
            if (offline_counter > 0) {
                offline_counter--;
            } else {
                is_offline = false;
                //SPDLOG_INFO("Service {} with id {} comes back online", serviceName, id);
                listener.enable_heartbeat();
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(dis(gen)));
    }
}

std::vector<std::shared_ptr<std::thread>> start_simulation(const std::vector<std::string>& serviceNames, const std::string& username, const std::string& password, const std::string& host, int port){
    std::vector<std::shared_ptr<std::thread>> serviceThreads;

    // Simulate different services
    int num_services_per_name = 500 / (int)serviceNames.size();
    for (const auto& serviceName : serviceNames) {
        for (int i = 0; i < num_services_per_name; ++i) {
            serviceThreads.emplace_back(std::make_shared<std::thread>(simulate_service, username, password, host, port, serviceName, i));
        }
    }
    return serviceThreads;
}



int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    std::filesystem::path env_file = ".env";
    std::map<std::string, std::string> env_vars = TagTwo::Util::DotEnv::read_dotenv(env_file);

    std::vector<std::string> serviceNames = {
            "ServiceA",
            "ServiceB",
            "ServiceC",
            "ServiceD",
            "ServiceE",
            "ServiceF"
    };

    auto serviceThreads = start_simulation(
            serviceNames,
            env_vars.at("RABBITMQ_USERNAME"),
            env_vars.at("RABBITMQ_PASSWORD"),
            env_vars.at("RABBITMQ_HOST"),
            std::stoi(env_vars.at("RABBITMQ_PORT"))
    );



    // Create a listener for service discovery
    TagTwo::Networking::ServiceDiscoveryClient listener("Master", "service-discovery", "service-answer", 120, 10, 5, false);
    listener.connect(
            env_vars.at("RABBITMQ_HOST"),
            std::stoi(env_vars.at("RABBITMQ_PORT")),
            env_vars.at("RABBITMQ_USERNAME"),
            env_vars.at("RABBITMQ_PASSWORD"),
            env_vars.at("RABBITMQ_VHOST")
    );


    while (true) {
        std::vector<std::string> existingServices = listener.get_existing_services();
        SPDLOG_INFO("Existing services ({}):", existingServices.size());
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
