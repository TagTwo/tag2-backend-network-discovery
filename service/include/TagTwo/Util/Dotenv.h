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
namespace TagTwo::Util {


    class DotEnv {
    public:
        // Public accessor for the singleton instance
        static DotEnv& getInstance() {
            static DotEnv instance;
            return instance;
        }

        // Load .env file into the singleton
        void load(const std::string& filepath);
        void require(const std::initializer_list<std::string>& required_keys) const;


        // Retrieve a specific key's value
        std::string get(const std::string& key) const;

    private:
        // Private constructor
        DotEnv() = default;

        // Delete copy constructor and copy assignment operator
        DotEnv(const DotEnv&) = delete;
        DotEnv& operator=(const DotEnv&) = delete;

        // Member variable to store the env_vars
        std::map<std::string, std::string> env_vars;
    };

}

#endif //TAGTWO_NETWORK_DISCOVERY_DOTENV_H
