//
// Created by per on 4/6/23.
//

#ifndef TAGTWO_NETWORK_DISCOVERY_DOTENV_H
#define TAGTWO_NETWORK_DISCOVERY_DOTENV_H
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <sstream>
#include <filesystem>
#include "spdlog/spdlog.h"
namespace TagTwo::Util::DotEnv {
    std::map<std::string, std::string> read_dotenv(const std::filesystem::path& filepath);
}

#endif //TAGTWO_NETWORK_DISCOVERY_DOTENV_H
