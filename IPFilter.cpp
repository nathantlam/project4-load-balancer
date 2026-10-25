/** @file IPFilter.cpp @brief IPv4 parsing, matching, and full-block detection. */
#include "IPFilter.h"

#include <algorithm>
#include <limits>
#include <sstream>
#include <stdexcept>

IPFilter::IPFilter(const std::vector<std::string>& ranges) {
    for (const auto& range : ranges) addRange(range);
}

std::uint32_t IPFilter::parseAddress(const std::string& address) {
    std::istringstream input(address);
    std::string octet;
    std::uint32_t result = 0;
    for (int index = 0; index < 4; ++index) {
        if (!std::getline(input, octet, '.') || octet.empty() || octet.size() > 3 ||
            octet.find_first_not_of("0123456789") != std::string::npos) {
            throw std::runtime_error("Invalid IPv4 address: " + address);
        }
        const int value = std::stoi(octet);
        if (value > 255) throw std::runtime_error("Invalid IPv4 address: " + address);
        result = (result << 8) | static_cast<std::uint32_t>(value);
    }
    if (!input.eof()) throw std::runtime_error("Invalid IPv4 address: " + address);
    return result;
}

void IPFilter::addRange(const std::string& range) {
    const auto slash = range.find('/');
    const auto address = parseAddress(range.substr(0, slash));
    int prefix = 32;
    if (slash != std::string::npos) {
        const auto text = range.substr(slash + 1);
        if (text.empty() || text.size() > 2 || text.find_first_not_of("0123456789") != std::string::npos) {
            throw std::runtime_error("Invalid CIDR prefix: " + range);
        }
        prefix = std::stoi(text);
        if (prefix > 32) throw std::runtime_error("Invalid CIDR prefix: " + range);
    }
    const std::uint32_t mask = prefix == 0 ? 0 : (0xffffffffU << (32 - prefix));
    const auto first = address & mask;
    blockedRanges.emplace_back(first, first | ~mask);
}

bool IPFilter::isBlocked(const std::string& address) const {
    const auto value = parseAddress(address);
    for (const auto& range : blockedRanges) {
        if (value >= range.first && value <= range.second) return true;
    }
    return false;
}

bool IPFilter::blocksAll() const {
    auto ranges = blockedRanges;
    std::sort(ranges.begin(), ranges.end());
    std::uint64_t covered = 0;
    for (const auto& range : ranges) {
        if (range.first > covered) return false;
        covered = std::max(covered, static_cast<std::uint64_t>(range.second) + 1);
        if (covered > std::numeric_limits<std::uint32_t>::max()) return true;
    }
    return false;
}
