//
// Created by per on 4/6/23.
//
#include "TagTwo/Util/JsonBuilder.h"


void TagTwo::Util::JsonBuilder::set_multilevel_key(nlohmann::json &json_object, const std::string &key,
                                                    const nlohmann::json &value) {
    // Split the key using '/' as a delimiter
    std::vector<std::string> key_parts = split_string(key, '/');

    nlohmann::json* current_level = &json_object;
    for (size_t i = 0; i < key_parts.size(); ++i) {
        const auto& key_part = key_parts[i];
        if (i == key_parts.size() - 1) { // Last key part
            (*current_level)[key_part] = value;
        } else { // Not the last key part
            if (current_level->find(key_part) == current_level->end()) { // If the key doesn't exist yet, create an empty object
                (*current_level)[key_part] = nlohmann::json::object();
            }
            current_level = &(*current_level)[key_part];
        }
    }
}

std::vector<std::string> TagTwo::Util::JsonBuilder::split_string(const std::string &input, char delimiter) {
    std::vector<std::string> tokens;
    std::istringstream token_stream(input);
    std::string token;

    while (std::getline(token_stream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

std::string TagTwo::Util::JsonBuilder::toJSON() const {
    return m_json_object.dump(4);
}

void TagTwo::Util::JsonBuilder::set(const std::string &key, const nlohmann::json &value) {
    set_multilevel_key(m_json_object, key, value);
}
