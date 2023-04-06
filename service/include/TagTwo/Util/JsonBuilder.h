//
// Created by per on 4/6/23.
//

#ifndef TAGTWO_NETWORK_DISCOVERY_JSONBUILDER_H
#define TAGTWO_NETWORK_DISCOVERY_JSONBUILDER_H
#include <nlohmann/json.hpp>

namespace TagTwo::Util{
/**
 * @class JsonBuilder
 * @brief A utility class for building JSON objects with multilevel keys.
 */
    class JsonBuilder {
    public:
        /**
         * @brief Sets a value for a multilevel key in the JSON object.
         * @param key A string representing the multilevel key (e.g. "a/b/c").
         * @param value The value to be set at the specified key.
         */
        void set(const std::string& key, const nlohmann::json& value);

        /**
         * @brief Returns the JSON object as a formatted string.
         * @return A string containing the JSON object in a human-readable format.
         */
        [[nodiscard]] std::string toJSON() const;

    private:
        /**
         * @brief Splits a string using a specified delimiter.
         * @param input The string to be split.
         * @param delimiter The character used as a delimiter for splitting the string.
         * @return A vector of strings representing the split input string.
         */
        [[nodiscard]] static std::vector<std::string> split_string(const std::string& input, char delimiter);

        /**
         * @brief Sets a value for a multilevel key in a JSON object.
         * @param json_object A reference to the JSON object where the value will be set.
         * @param key A string representing the multilevel key (e.g. "a/b/c").
         * @param value The value to be set at the specified key.
         */
        static void set_multilevel_key(nlohmann::json& json_object, const std::string& key, const nlohmann::json& value);

        /// The JSON object being built.
        nlohmann::json m_json_object;
    };


}
#endif //TAGTWO_NETWORK_DISCOVERY_JSONBUILDER_H
