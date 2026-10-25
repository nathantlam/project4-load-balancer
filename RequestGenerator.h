/** @file RequestGenerator.h @brief Seeded random requests and arrival batches. */
#ifndef REQUEST_GENERATOR_H
#define REQUEST_GENERATOR_H

#include "Config.h"
#include "Request.h"
#include <random>

/** @brief Uses a single seeded generator for the simulated workload. */
class RequestGenerator {
public:
    /** @brief Set random distributions. @param config Validated settings. */
    explicit RequestGenerator(const Config& config);
    /** @brief Generate a job. @return Random addresses, time, and P/S type. */
    Request generate();
    /** @brief Sample arrivals for one cycle. @return Zero or a configured batch size. */
    int arrivalsThisCycle();
private:
    std::mt19937 engine; ///< Seeded pseudorandom generator.
    std::uniform_int_distribution<int> octet{0, 255}; ///< IPv4 octet.
    std::uniform_int_distribution<int> taskTime; ///< Job duration.
    std::uniform_int_distribution<int> jobType{0, 1}; ///< Job category.
    std::uniform_int_distribution<int> batchSize; ///< Arrival count.
    std::bernoulli_distribution arrival; ///< Arrival event.
    /** @brief Build a dotted IPv4 address. @return Four random octets. */
    std::string randomIP();
};

#endif
