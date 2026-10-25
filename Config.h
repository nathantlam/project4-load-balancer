/** @file Config.h @brief Simulation settings read from a text file. */
#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

/** @brief Validated settings shared by the simulation components. */
struct Config {
    int minTaskTime = 5; ///< Shortest job in cycles.
    int maxTaskTime = 50; ///< Longest job in cycles.
    double arrivalProbability = 0.45; ///< Chance of an arrival batch each cycle.
    int minArrivals = 1; ///< Smallest batch when arrivals occur.
    int maxArrivals = 6; ///< Largest arrival batch.
    int scalingCooldown = 100; ///< Cycles between actual server changes.
    int minServers = 1; ///< Minimum server count per balancer.
    int maxServers = 1000; ///< Safety limit per balancer.
    unsigned int randomSeed = 412; ///< Repeatable random-number seed.
    std::vector<std::string> blockedRanges; ///< Incoming IPv4 CIDR ranges.

    /** @brief Read key=value settings. @param path File to open. @return Settings.
     * @throws std::runtime_error For missing files, unknown keys, or invalid values. */
    static Config load(const std::string& path);
    /** @brief Validate numeric ranges. @throws std::runtime_error On invalid settings. */
    void validate() const;
};

#endif
