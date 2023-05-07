//
// Created by per on 4/6/23.
//
#include "TagTwo/Util/Dotenv.h"


void TagTwo::Util::DotEnv::require(const std::initializer_list<std::string>& required_keys) {
    for (const auto& key : required_keys) {
        try {
            get(key); // Will throw an error if the key is not found
        } catch (const std::runtime_error& e) {
            SPDLOG_ERROR("Error: Required environment variable not found: {}", key);
            throw;
        }
    }
}


void TagTwo::Util::DotEnv::load(const std::string& filepath) {
    auto file_path = std::filesystem::path(filepath);

    std::ifstream file(file_path);

    if (!file.is_open()) {
        SPDLOG_WARN("Error: Unable to open .env file at path: {}", file_path.string());
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string key, value;

        if (std::getline(ss, key, '=')) {
            std::getline(ss, value);
            env_vars[key] = value;
        }
    }

    file.close();
}

std::string TagTwo::Util::DotEnv::get(const std::string& key) const {
    // First, check the env_vars map
    auto it = env_vars.find(key);
    if (it != env_vars.end()) {
        return it->second;
    }

    // Second, check the system environment
    const char* value = std::getenv(key.c_str());
    if (value) {
        return {value};
    }

    // If the key is not found in both locations, throw an error
    throw std::runtime_error("Error: Environment variable not found: " + key);
}
