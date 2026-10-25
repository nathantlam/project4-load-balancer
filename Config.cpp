/** @file Config.cpp @brief Strict configuration parsing and validation. */
#include "Config.h"

#include <fstream>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>

namespace {
/** @brief Remove surrounding whitespace from a setting. */
std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}

/** @brief Read a complete value with no trailing characters. */
template <typename T>
T parseNumber(const std::string& value) {
    std::istringstream input(value);
    T number{};
    if (!(input >> number)) {
        throw std::runtime_error("Invalid numeric value: " + value);
    }
    input >> std::ws;
    if (!input.eof()) {
        throw std::runtime_error("Invalid numeric value: " + value);
    }
    return number;
}
}

Config Config::load(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Cannot open configuration: " + path);
    }

    Config config;
    std::set<std::string> keys;
    std::string line;
    int lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        line = trim(line.substr(0, line.find('#')));
        if (line.empty()) {
            continue;
        }
        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            throw std::runtime_error("Expected key=value at line " + std::to_string(lineNumber));
        }
        const std::string key = trim(line.substr(0, separator));
        const std::string value = trim(line.substr(separator + 1));
        if (!keys.insert(key).second) {
            throw std::runtime_error("Duplicate configuration key: " + key);
        }
        if (key == "min_task_time") config.minTaskTime = parseNumber<int>(value);
        else if (key == "max_task_time") config.maxTaskTime = parseNumber<int>(value);
        else if (key == "arrival_probability") config.arrivalProbability = parseNumber<double>(value);
        else if (key == "min_arrivals") config.minArrivals = parseNumber<int>(value);
        else if (key == "max_arrivals") config.maxArrivals = parseNumber<int>(value);
        else if (key == "scaling_cooldown") config.scalingCooldown = parseNumber<int>(value);
        else if (key == "min_servers") config.minServers = parseNumber<int>(value);
        else if (key == "max_servers") config.maxServers = parseNumber<int>(value);
        else if (key == "random_seed") {
            const auto seed = parseNumber<long long>(value);
            if (seed < 0 || static_cast<unsigned long long>(seed) > std::numeric_limits<unsigned int>::max()) {
                throw std::runtime_error("random_seed is outside the unsigned integer range");
            }
            config.randomSeed = static_cast<unsigned int>(seed);
        } else if (key == "blocked_ranges") {
            if (!value.empty() && value.back() == ',') {
                throw std::runtime_error("Empty blocked range after trailing comma");
            }
            std::istringstream ranges(value);
            std::string range;
            while (std::getline(ranges, range, ',')) {
                range = trim(range);
                if (range.empty()) throw std::runtime_error("Empty blocked range");
                config.blockedRanges.push_back(range);
            }
        } else {
            throw std::runtime_error("Unknown configuration key: " + key);
        }
    }
    config.validate();
    return config;
}

void Config::validate() const {
    if (minTaskTime < 1 || maxTaskTime < minTaskTime || maxTaskTime > 1000000000) {
        throw std::runtime_error("Task times must satisfy 1 <= min <= max <= 1000000000");
    }
    if (!(arrivalProbability >= 0.0 && arrivalProbability <= 1.0)) {
        throw std::runtime_error("arrival_probability must be between 0 and 1");
    }
    if (minArrivals < 1 || maxArrivals < minArrivals || maxArrivals > 100000) {
        throw std::runtime_error("Arrival batch sizes must satisfy 1 <= min <= max <= 100000");
    }
    if (scalingCooldown < 1 || minServers < 1 || maxServers < minServers || maxServers > 10000) {
        throw std::runtime_error("Use positive cooldown and 1 <= min_servers <= max_servers <= 10000");
    }
}
