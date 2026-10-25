/** @file IPFilter.h @brief IPv4 CIDR firewall interface. */
#ifndef IP_FILTER_H
#define IP_FILTER_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

/** @brief Rejects incoming addresses within configured IPv4 CIDR ranges. */
class IPFilter {
public:
    /** @brief Create the firewall. @param ranges CIDR strings; empty allows all. */
    explicit IPFilter(const std::vector<std::string>& ranges = {});
    /** @brief Add a CIDR range. @param range IPv4/prefix or a single IPv4 address. */
    void addRange(const std::string& range);
    /** @brief Test a source address. @param address Dotted IPv4. @return True if blocked. */
    bool isBlocked(const std::string& address) const;
    /** @brief Check full-space blocking. @return True when no IPv4 address is allowed. */
    bool blocksAll() const;
private:
    std::vector<std::pair<std::uint32_t, std::uint32_t>> blockedRanges; ///< Inclusive ranges.
    /** @brief Validate and pack an IPv4 string. @param address Dotted IPv4. @return Packed address. */
    static std::uint32_t parseAddress(const std::string& address);
};

#endif
