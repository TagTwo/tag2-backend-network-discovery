//
// Created by per on 4/6/23.
//
#include "TagTwo/Util/Dotenv.h"

std::map<std::string, std::string> TagTwo::Util::DotEnv::read_dotenv(const std::filesystem::path& filepath) {
    std::map<std::string, std::string> env_vars;
    std::ifstream file(filepath);

    if (!file.is_open()) {
        SPDLOG_ERROR("Error: Unable to open .env file at path: {}", filepath.string());
        return env_vars;
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
    return env_vars;
}